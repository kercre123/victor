#include <string.h>

#include "common.h"
#include "hardware.h"

#include "comms.h"
#include "motors.h"
#include "power.h"
#include "opto.h"
#include "lights.h"
#include "mics.h"
#include "touch.h"
#include "contacts.h"

#include "messages.h"

static struct {
  // Body 2 Head data
  SpineMessageHeader headerB2H;
  BodyToHead payloadB2H;
  SpineMessageFooter footerB2H;
  
  // Contact data
  SpineMessageHeader headerCD;
  ContactData payloadCD;
  SpineMessageFooter footerCD;
} outboundPacket;

static struct {
  union {
    SpineMessageHeader header;
    uint8_t header_raw[];
  };
  union {
    HeadToBody headToBody;
    ContactData contactData;
    uint8_t raw[];
  };
  SpineMessageFooter _footer;
} inboundPacket;

static const uint16_t BYTE_TIMER_PRESCALE = SYSTEM_CLOCK / COMMS_BAUDRATE;
static const uint16_t BYTE_TIMER_PERIOD = 14; // 8 bits, 1 start / stop + 3 bits of "slop"
static const int MAX_MISSED_FRAMES = 200; // 1 second of missed frames
static int missed_frames;

static uint32_t crc(const void* ptr, int length) {
  const uint32_t* data = (const uint32_t*) ptr;

  CRC->CR = CRC_CR_RESET | CRC_CR_REV_IN | CRC_CR_REV_OUT;
  while (length-- > 0) {
    CRC->DR = *(data++);
  }
  return CRC->DR;
}

void Comms::init(void) {
  // Initial values for CRC32
  CRC->INIT = ~0;

  // Configure our GPIO to be wired into USART1's
  BODY_TX::alternate(0);  // USART1_TX
  BODY_TX::speed(SPEED_HIGH);
  BODY_TX::pull(PULL_NONE);
  BODY_TX::mode(MODE_ALTERNATE);

  BODY_RX::alternate(0);  // USART1_RX
  BODY_RX::speed(SPEED_HIGH);
  BODY_RX::pull(PULL_NONE);
  BODY_RX::mode(MODE_ALTERNATE);

  // Configure our USART1 (Start interrupt)
  USART1->BRR = SYSTEM_CLOCK / COMMS_BAUDRATE;
  USART1->CR3 = USART_CR3_DMAR | USART_CR3_OVRDIS;
  USART1->CR2 = (SYNC_HEAD_TO_BODY & 0xFF) << 24;
  USART1->CR1 = USART_CR1_RE
              | USART_CR1_TE
              | USART_CR1_CMIE
              | USART_CR1_UE;

  // Timer6 is used to fire off the DMA for outbound UART data
  TIM6->PSC = BYTE_TIMER_PRESCALE - 1;
  TIM6->ARR = BYTE_TIMER_PERIOD - 1;
  TIM6->DIER = TIM_DIER_UDE;
  TIM6->CR1 = TIM_CR1_CEN;

  // Setup our constants for outbound data
  outboundPacket.headerB2H.sync_bytes = SYNC_BODY_TO_HEAD;
  outboundPacket.headerB2H.bytes_to_follow = sizeof(outboundPacket.payloadB2H);
  outboundPacket.headerB2H.payload_type = PAYLOAD_DATA_FRAME;
  
  outboundPacket.headerCD.sync_bytes = SYNC_BODY_TO_HEAD;
  outboundPacket.headerCD.bytes_to_follow = sizeof(outboundPacket.payloadCD);
  outboundPacket.headerCD.payload_type = PAYLOAD_CONT_DATA;

  // Configure our interrupts
  NVIC_SetPriority(USART1_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(USART1_IRQn);
  NVIC_SetPriority(DMA1_Channel4_5_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

  missed_frames = 0;
}

void Comms::tick(void) {
  Motors::transmit(&outboundPacket.payloadB2H);
  Opto::transmit(&outboundPacket.payloadB2H);
  Mics::transmit(outboundPacket.payloadB2H.audio);
  Touch::transmit(outboundPacket.payloadB2H.touchLevel);

  if (missed_frames++ >= MAX_MISSED_FRAMES) {
    // Reset but don't pull power
    Power::softReset();
  }

  // Finalize the packet
  outboundPacket.payloadB2H.framecounter++;
  outboundPacket.footerB2H.checksum = crc(&outboundPacket.payloadB2H, sizeof(outboundPacket.payloadB2H) / sizeof(uint32_t));

  // Always send contact data (regardless if it's used or not)
  Contacts::transmit(outboundPacket.payloadCD);
  outboundPacket.footerCD.checksum = crc(&outboundPacket.payloadB2H, sizeof(outboundPacket.payloadCD) / sizeof(uint32_t));

  // Fire out ourbound data
  DMA1_Channel3->CCR = 0;
  DMA1_Channel3->CPAR = (uint32_t)&USART1->TDR;
  DMA1_Channel3->CMAR = (uint32_t)&outboundPacket;
  DMA1_Channel3->CNDTR = sizeof(outboundPacket);

  DMA1_Channel3->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR
                     | DMA_CCR_EN;
}

static int sizeOfInboundPayload(PayloadId id) {
  switch (id) {
    case PAYLOAD_DATA_FRAME:
      return sizeof(HeadToBody);
    case PAYLOAD_CONT_DATA:
      return sizeof(ContactData);
    default:
      return -1;
  }
}

extern "C" void USART1_IRQHandler(void) {
  static int header_rx_index;
  
  if (USART1->ISR & USART_ISR_CMF) {
    // Switch USART to DMA mode
    USART1->CR1 = (USART1->CR1 & ~USART_CR1_CMIE) | USART_CR1_RXNEIE;
    USART1->ICR = USART_ICR_CMCF;
    header_rx_index = 0;
  }

  if (USART1->ISR & USART_ISR_RXNE) {
    inboundPacket.header_raw[header_rx_index++] = USART1->RDR;
  }

  if (header_rx_index < sizeof(SpineMessageHeader)) {
    return ;
  }

  // Payload is an incorrect size (ignore)
  if (inboundPacket.header.bytes_to_follow != sizeOfInboundPayload(inboundPacket.header.payload_type)) {
    USART1->CR1 |= USART_CR1_CMIE;
    return ;
  }

  // Configure out receive buffer
  USART1->CR1 &= ~USART_CR1_RXNEIE;
  
  DMA1_Channel5->CPAR = (uint32_t)&USART1->RDR;
  DMA1_Channel5->CMAR = (uint32_t)inboundPacket.raw;
  DMA1_Channel5->CNDTR = sizeof(SpineMessageFooter) + inboundPacket.header.bytes_to_follow;
  DMA1_Channel5->CCR = DMA_CCR_MINC
                     | DMA_CCR_TCIE
                     | DMA_CCR_CIRC
                     | DMA_CCR_EN
                     ;
}

extern "C" void DMA1_Channel4_5_IRQHandler(void) {
  // Clear our flag
  DMA1->IFCR = DMA_IFCR_CGIF5;

  // Header is invalid
  if (inboundPacket.header.sync_bytes != SYNC_HEAD_TO_BODY) {
    return ;
  }

  // Check the CRC
  static uint32_t foundCRC = crc(&inboundPacket.raw, inboundPacket.header.bytes_to_follow / sizeof (uint32_t));
  SpineMessageFooter* footer = (SpineMessageFooter*) &inboundPacket.raw[inboundPacket.header.bytes_to_follow];
  
  if (foundCRC != footer->checksum) {
    return ;
  }

  switch (inboundPacket.header.payload_type) {
    case PAYLOAD_DATA_FRAME:
      missed_frames = 0;
      Motors::receive(&inboundPacket.headToBody);
      Lights::receive(inboundPacket.headToBody.ledColors);
      break ;
    case PAYLOAD_CONT_DATA:
      Contacts::forward(inboundPacket.contactData);
      break ;
    default:
      break ;
  }

  // Switch back to interrupt mode
  DMA1_Channel5->CCR = 0;
  USART1->ICR = USART_ICR_CMCF;
  USART1->CR1 |= USART_CR1_CMIE;
}
