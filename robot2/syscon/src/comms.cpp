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

#include "messages.h"

using namespace Anki::Cozmo::Spine;

static struct {
  SpineMessageHeader header;
  BodyToHead payload;
  SpineMessageFooter footer;
} outboundPacket;

static struct {
  SpineMessageHeader header;
  HeadToBody payload;
  SpineMessageFooter footer;
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
  outboundPacket.header.sync_bytes = SYNC_BODY_TO_HEAD;
  outboundPacket.header.payload_type = PAYLOAD_DATA_FRAME;

  // Configure our interrupts
  NVIC_SetPriority(USART1_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(USART1_IRQn);
  NVIC_SetPriority(DMA1_Channel4_5_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

  missed_frames = 0;
}

void Comms::tick(void) {
  Motors::transmit(&outboundPacket.payload);
  Opto::transmit(&outboundPacket.payload);
  //Mics::transmit(outboundPacket.payload.audio);
  Touch::transmit(outboundPacket.payload.touchLevel);

  if (missed_frames++ >= MAX_MISSED_FRAMES) {
    // Reset but don't pull power
    Power::softReset();
  }

  // Finalize the packet
  outboundPacket.header.bytes_to_follow = sizeof(outboundPacket.payload);
  outboundPacket.payload.framecounter++;

  outboundPacket.footer.checksum = crc(&outboundPacket.payload, sizeof(outboundPacket.payload) / sizeof(uint32_t));

  // Fire out ourbound data
  DMA1_Channel3->CCR = 0;
  DMA1_Channel3->CPAR = (uint32_t)&USART1->TDR;
  DMA1_Channel3->CMAR = (uint32_t)&outboundPacket;
  DMA1_Channel3->CNDTR = sizeof(outboundPacket);

  DMA1_Channel3->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR
                     | DMA_CCR_EN;
}

extern "C" void USART1_IRQHandler(void) {
  // Configure out receive buffer
  DMA1_Channel5->CPAR = (uint32_t)&USART1->RDR;
  DMA1_Channel5->CMAR = (uint32_t)&inboundPacket;
  DMA1_Channel5->CNDTR = sizeof(inboundPacket);
  DMA1_Channel5->CCR = DMA_CCR_MINC
                     | DMA_CCR_TCIE
                     | DMA_CCR_CIRC
                     | DMA_CCR_EN
                     ;

  // Switch USART to DMA mode
  USART1->ICR = USART_ICR_CMCF;
  NVIC_DisableIRQ(USART1_IRQn);
}

extern "C" void DMA1_Channel4_5_IRQHandler(void) {
  // Clear our flag
  DMA1->IFCR = DMA_IFCR_CGIF5;

  const uint32_t crc_payload = crc(&inboundPacket.payload, sizeof(inboundPacket.payload) / sizeof (uint32_t));

  // Verify that our packet was valid
  if (inboundPacket.header.sync_bytes != SYNC_HEAD_TO_BODY ||
      inboundPacket.header.bytes_to_follow != sizeof(inboundPacket.payload) ||
      crc_payload != inboundPacket.footer.checksum) {

    // Switch back to interrupt mode
    DMA1_Channel5->CCR = 0;
    USART1->ICR = USART_ICR_CMCF;
    NVIC_EnableIRQ(USART1_IRQn);
    return ;
  }

  switch (inboundPacket.header.payload_type) {
    case PAYLOAD_DATA_FRAME:
      missed_frames = 0;
      Motors::receive(&inboundPacket.payload);
      Lights::receive(inboundPacket.payload.ledColors);
      break ;
    default:
      break ;
  }
}
