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
#include "vectors.h"
#include "flash.h"

#include "messages.h"

struct TransmitFifoPayload {
  SpineMessageHeader header;
  union {
    ContactData contactData;
    AckMessage ack;
    VersionInfo version;
    uint8_t raw[];
  };
  SpineMessageFooter _footer;
};

struct TransmitFifo {
  uint8_t size;
  TransmitFifoPayload payload;
};

static struct {
  struct {
    // Body 2 Head data
    SpineMessageHeader header;
    BodyToHead payload;
    SpineMessageFooter footer;
  } sync;
  
  // Additional data (after sync)
  uint8_t tail[sizeof(TransmitFifoPayload) * 3];
} outboundPacket;

static struct {
  union {
    SpineMessageHeader header;
    uint8_t header_raw[];
  };
  union {
    HeadToBody headToBody;
    ContactData contactData;
    AckMessage ack;
    VersionInfo version;
    uint8_t raw[];
  };
  SpineMessageFooter _footer;
} inboundPacket;

static const uint16_t BYTE_TIMER_PRESCALE = SYSTEM_CLOCK / COMMS_BAUDRATE;
static const uint16_t BYTE_TIMER_PERIOD = 14; // 8 bits, 1 start / stop + 3 bits of "slop"
static const int MAX_MISSED_FRAMES = 200; // 1 second of missed frames
static const int MAX_FIFO_SLOTS = 4;

static TransmitFifo txFifo[MAX_FIFO_SLOTS];
static int txFifo_read = 0;
static int txFifo_write = 0;

static int missed_frames = 0;
static bool applicationRunning = false;

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
              | USART_CR1_RXNEIE
              | USART_CR1_UE;

  // Timer6 is used to fire off the DMA for outbound UART data
  TIM6->PSC = BYTE_TIMER_PRESCALE - 1;
  TIM6->ARR = BYTE_TIMER_PERIOD - 1;
  TIM6->DIER = TIM_DIER_UDE;
  TIM6->CR1 = TIM_CR1_CEN;

  // Setup our constants for outbound data
  outboundPacket.sync.header.sync_bytes = SYNC_BODY_TO_HEAD;
  outboundPacket.sync.header.payload_type = PAYLOAD_DATA_FRAME;
  outboundPacket.sync.header.bytes_to_follow = sizeof(outboundPacket.sync.payload);

  applicationRunning = false;

  static const AckMessage ack = { ACK_APPLICATION };
  enqueue(PAYLOAD_ACK, &ack, sizeof(ack));

  // Configure our interrupts
  NVIC_SetPriority(USART1_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(USART1_IRQn);
  NVIC_SetPriority(DMA1_Channel4_5_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
}

void Comms::enqueue(PayloadId kind, const void* packet, int size) {
  int next = (txFifo_write + 1) % MAX_FIFO_SLOTS;
  
  // Drop payload (should probably do something)
  if (next == txFifo_read) {
    return ;
  }
  
  TransmitFifo* target = &txFifo[txFifo_write];
  SpineMessageFooter* footer = (SpineMessageFooter*)&target->payload.raw[size];
    
  target->payload.header.sync_bytes = SYNC_BODY_TO_HEAD;
  target->payload.header.payload_type = kind;
  target->payload.header.bytes_to_follow = size;
  memcpy(&target->payload.raw, packet, size);
  footer->checksum = crc(target->payload.raw, size / sizeof(uint32_t));

  target->size = sizeof(SpineMessageHeader) + sizeof(SpineMessageFooter) + size;
  
  txFifo_write = next;
}

static int dequeue(void* out, int size) {
  uint8_t* output = (uint8_t*) out;
  int transmitted = 0;

  while (txFifo_read != txFifo_write) {
    TransmitFifo* target = &txFifo[txFifo_read];
    if (target->size > size) {
      break ;
    }
    
    memcpy(output, &target->payload, target->size);
    transmitted += target->size;
    size -= target->size;
    output += target->size;
    
    txFifo_read = (txFifo_read + 1) % MAX_FIFO_SLOTS;
  }

  return transmitted;
}

void Comms::tick(void) {
  // Soft-watchdog (This is lame)
  if (missed_frames++ >= MAX_MISSED_FRAMES) {
    applicationRunning = false;
  }

  // Stop our DMA
  DMA1_Channel3->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR;
  
  // Finalize the packet
  int count;

  if (applicationRunning) {
    Motors::transmit(&outboundPacket.sync.payload);
    Opto::transmit(&outboundPacket.sync.payload);
    Mics::transmit(outboundPacket.sync.payload.audio);
    Touch::transmit(outboundPacket.sync.payload.touchLevel);

    outboundPacket.sync.payload.framecounter++;
    outboundPacket.sync.footer.checksum = crc(&outboundPacket.sync.payload, sizeof(outboundPacket.sync.payload) / sizeof(uint32_t));

    DMA1_Channel3->CMAR = (uint32_t)&outboundPacket;
    count = sizeof(outboundPacket.sync);
  } else {
    DMA1_Channel3->CMAR = (uint32_t)&outboundPacket.tail;
    count = 0;
  }

  count += dequeue(&outboundPacket.tail, sizeof(outboundPacket.tail));

  // Fire out ourbound data
  if (count > 0) {
    DMA1_Channel3->CPAR = (uint32_t)&USART1->TDR;
    DMA1_Channel3->CNDTR = count;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
  }
}

static int sizeOfInboundPayload(PayloadId id) {
  switch (id) {
    case PAYLOAD_MODE_CHANGE:
    case PAYLOAD_ERASE:
    case PAYLOAD_VERSION:
      return 0;
    case PAYLOAD_DATA_FRAME:
      return sizeof(HeadToBody);
    case PAYLOAD_CONT_DATA:
      return sizeof(ContactData);
    default:
      return -1;
  }
}

extern "C" void USART1_IRQHandler(void) {
  static int header_rx_index = 0;

  // Flush overflows
  if (USART1->ISR & USART_ISR_ORE) {
    USART1->ICR = USART_ICR_ORECF;
  }

  // No data available
  if (~USART1->ISR & USART_ISR_RXNE) {
    return ;
  }

  inboundPacket.header_raw[header_rx_index++] = USART1->RDR;

  const uint32_t mask = ~(~0 << (header_rx_index * 8));
  const uint32_t expect = SYNC_HEAD_TO_BODY & mask;
  const uint32_t receive = inboundPacket.header.sync_bytes & mask;

  // Header mismatch
  if (expect != receive) {
    header_rx_index = 0;
    return ;
  }

  if (header_rx_index < sizeof(inboundPacket.header)) {
    return ;
  }

  // Clear header for matching next time
  header_rx_index = 0;

  // Payload is an incorrect size (ignore)
  if (inboundPacket.header.bytes_to_follow != sizeOfInboundPayload(inboundPacket.header.payload_type)) {
    return ;
  }

  // Configure out receive buffer
  NVIC_DisableIRQ(USART1_IRQn);
  
  DMA1_Channel5->CPAR = (uint32_t)&USART1->RDR;
  DMA1_Channel5->CMAR = (uint32_t)inboundPacket.raw;
  DMA1_Channel5->CNDTR = sizeof(SpineMessageFooter) + inboundPacket.header.bytes_to_follow;
  DMA1_Channel5->CCR = DMA_CCR_MINC
                     | DMA_CCR_TCIE
                     | DMA_CCR_EN
                     ;
}

void Comms::sendVersion(void) {
  VersionInfo ver;

  ver.hw_revision = COZMO_HWINFO->hw_revision;
  ver.hw_model = COZMO_HWINFO->hw_model;
  memcpy(ver.ein, COZMO_HWINFO->ein, sizeof(ver.ein));
  memcpy(ver.app_version, APP->applicationVersion, sizeof(ver.app_version));

  Comms::enqueue(PAYLOAD_VERSION, &ver, sizeof(ver));
}

extern "C" void DMA1_Channel4_5_IRQHandler(void) {
  // Clear our flag
  DMA1->IFCR = DMA_IFCR_CGIF5;

  // Check the CRC
  uint32_t foundCRC = crc(inboundPacket.raw, inboundPacket.header.bytes_to_follow / sizeof (uint32_t));
  SpineMessageFooter* footer = (SpineMessageFooter*) &inboundPacket.raw[inboundPacket.header.bytes_to_follow];

  if (foundCRC == footer->checksum) {
    switch (inboundPacket.header.payload_type) {
      case PAYLOAD_MODE_CHANGE:
        missed_frames = 0;
        applicationRunning = true;
        break ;
      case PAYLOAD_VERSION:
        Comms::sendVersion();
        break ;
      case PAYLOAD_ERASE:
        Flash::markForWipe();
        break ;
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
  }

  // Switch back to interrupt mode
  DMA1_Channel5->CCR = 0;
  NVIC_EnableIRQ(USART1_IRQn);
}
