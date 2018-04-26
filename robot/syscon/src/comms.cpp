#include <string.h>

#include "common.h"
#include "hardware.h"

#include "analog.h"
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

enum CommsState {
  COMM_STATE_SYNC,
  COMM_STATE_VALIDATE,
  COMM_STATE_PAYLOAD
};

struct InboundPacket {
  union {
    SpineMessageHeader header;
    uint8_t raw[];
  };
  union {
    HeadToBody headToBody;
    ContactData contactData;
    AckMessage ack;
    VersionInfo version;
    uint8_t raw_payload[];
  };
  SpineMessageFooter _footer;
};

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

static const uint16_t BYTE_TIMER_PRESCALE = SYSTEM_CLOCK / COMMS_BAUDRATE;
static const uint16_t BYTE_TIMER_PERIOD = 14; // 8 bits, 1 start / stop + 3 bits of "slop"
static const int MAX_MISSED_FRAMES = 200; // 1 second of missed frames
static const int MAX_FIFO_SLOTS = 4;
static const int MAX_INBOUND_SIZE = 0x100;  // Must be a power of two, and at least twice as big as InboundPacket
static const int EXTRA_MESSAGE_DATA = sizeof(SpineMessageHeader) + sizeof(SpineMessageFooter);

static TransmitFifo txFifo[MAX_FIFO_SLOTS];
static int txFifo_read = 0;
static int txFifo_write = 0;
static uint8_t inbound_raw[MAX_INBOUND_SIZE];

static int missed_frames = 0;

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

  // Configure our USART1 (Using double buffered DMA)
  USART1->BRR = SYSTEM_CLOCK / COMMS_BAUDRATE;
  USART1->CR3 = USART_CR3_DMAR | USART_CR3_OVRDIS;
  USART1->CR1 = USART_CR1_RE
              | USART_CR1_TE
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

  // Configure our interrupts
  NVIC_SetPriority(DMA1_Channel4_5_IRQn, PRIORITY_SPINE_COMMS);
  NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

  // Start reading in a circular buffer
  DMA1_Channel5->CPAR = (uint32_t)&USART1->RDR;
  DMA1_Channel5->CMAR = (uint32_t)inbound_raw;
  DMA1_Channel5->CNDTR = sizeof(inbound_raw);
  DMA1_Channel5->CCR = DMA_CCR_MINC
                     | DMA_CCR_CIRC
                     | DMA_CCR_TCIE
                     | DMA_CCR_HTIE
                     | DMA_CCR_EN
                     ;

  static const AckMessage ack = { ACK_APPLICATION };
  enqueue(PAYLOAD_ACK, &ack, sizeof(ack));
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
  // Soft-watchdog (Disable power to sensors when robot process is idle)
  if (missed_frames < MAX_MISSED_FRAMES) {
    missed_frames++;
  } else if (Opto::sensorsValid()) {
    Power::setMode(POWER_CALM);
  }

  // Stop our DMA
  DMA1_Channel3->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR;

  // Finalize the packet
  int count = sizeof(outboundPacket.sync);

  Analog::transmit(&outboundPacket.sync.payload);
  Motors::transmit(&outboundPacket.sync.payload);
  Opto::transmit(&outboundPacket.sync.payload);
#if MICDATA_ENABLED
  Mics::transmit(outboundPacket.sync.payload.audio);
  Mics::errorCode(outboundPacket.sync.payload.micError);
#endif
  Touch::transmit(outboundPacket.sync.payload.touchLevel);

  outboundPacket.sync.payload.framecounter++;
  outboundPacket.sync.footer.checksum = crc(&outboundPacket.sync.payload, sizeof(outboundPacket.sync.payload) / sizeof(uint32_t));

  DMA1_Channel3->CMAR = (uint32_t)&outboundPacket;

  count += dequeue(&outboundPacket.tail, sizeof(outboundPacket.tail));

  // Fire out ourbound data
  DMA1_Channel3->CPAR = (uint32_t)&USART1->TDR;
  DMA1_Channel3->CNDTR = count;
  DMA1_Channel3->CCR |= DMA_CCR_EN;

  NVIC_SetPendingIRQ(DMA1_Channel4_5_IRQn);
}

static int sizeOfInboundPayload(PayloadId id) {
  switch (id) {
    case PAYLOAD_SHUT_DOWN:
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

void Comms::sendVersion(void) {
  VersionInfo ver;

  ver.hw_revision = COZMO_HWINFO->hw_revision;
  ver.hw_model = COZMO_HWINFO->hw_model;
  memcpy(ver.ein, COZMO_HWINFO->ein, sizeof(ver.ein));
  memcpy(ver.app_version, APP->applicationVersion, sizeof(ver.app_version));

  Comms::enqueue(PAYLOAD_VERSION, &ver, sizeof(ver));
}

static void ProcessMessage(InboundPacket& packet) {
  using namespace Comms;
  // Check the CRC
  uint32_t foundCRC = crc(packet.raw_payload, packet.header.bytes_to_follow / sizeof (uint32_t));
  SpineMessageFooter* footer = (SpineMessageFooter*) &packet.raw_payload[packet.header.bytes_to_follow];

  // Process our packet
  if (foundCRC == footer->checksum) {
    // Emergency eject in case of recovery mode
    BODY_TX::set();
    BODY_TX::mode(MODE_ALTERNATE);

    switch (packet.header.payload_type) {
      case PAYLOAD_SHUT_DOWN:
        Power::setMode(POWER_STOP);
        break ;
      case PAYLOAD_MODE_CHANGE:
        missed_frames = 0;
        Power::setMode(POWER_ACTIVE);
        break ;
      case PAYLOAD_VERSION:
        Comms::sendVersion();
        break ;
      case PAYLOAD_ERASE:
        Power::setMode(POWER_ERASE);
        break ;
      case PAYLOAD_DATA_FRAME:
        missed_frames = 0;
        Motors::receive(&packet.headToBody);
        Lights::receive(packet.headToBody.ledColors);
        break ;
      case PAYLOAD_CONT_DATA:
        Contacts::forward(packet.contactData);
        break ;
      default:
        static const AckMessage ack = { NACK_BAD_COMMAND };
        enqueue(PAYLOAD_ACK, &ack, sizeof(ack));
        break ;
    }
  } else {
    static const AckMessage ack = { NACK_CRC_FAILED };
    enqueue(PAYLOAD_ACK, &ack, sizeof(ack));
  }
}

extern "C" void DMA1_Channel4_5_IRQHandler(void) {
  // Find number of words transfered
  static int previousIndex = 0;
  static int receivedWords = 0;
  static CommsState state = COMM_STATE_SYNC;

  int currentIndex = MAX_INBOUND_SIZE - DMA1_Channel5->CNDTR;

  static InboundPacket packet;
  static int packetLength;

  while (currentIndex != previousIndex) {
    int copy;

    if (currentIndex < previousIndex) {
      // Copy wrap around
      copy = MAX_INBOUND_SIZE - previousIndex;
    } else {
      // Up to cursor copy
      copy = currentIndex - previousIndex;
    }

    // Clamp to buffer length
    if (copy > sizeof(InboundPacket) - receivedWords) {
      copy = sizeof(InboundPacket) - receivedWords;
    }

    // Shift in the data from the circular buffer
    if (copy > 0) {
      memcpy(&packet.raw[receivedWords], &inbound_raw[previousIndex], copy);
      receivedWords += copy;
      previousIndex = (previousIndex + copy) & (MAX_INBOUND_SIZE - 1);
    }

    // Sync to packet header
    switch (state) {
      case COMM_STATE_SYNC:
        int offset;

        // We don't have enough data to test the header
        if (receivedWords < sizeof(uint32_t)) {
          continue ;
        }

        // Locate our sync header
        for (offset = 0; offset <= receivedWords - sizeof(uint32_t); offset++) {
          __packed uint32_t* sync_word = (__packed uint32_t*) &packet.raw[offset];

          if (*sync_word == SYNC_HEAD_TO_BODY) {
            state = COMM_STATE_VALIDATE;
            break ;
          }
        }

        // Trim off the erranious words
        if (offset) {
          memcpy(&packet.raw[0], &packet.raw[offset], receivedWords - offset);
          receivedWords -= offset;
        }

      case COMM_STATE_VALIDATE:
        // We don't have enough data to test the header
        if (receivedWords < sizeof(SpineMessageHeader)) {
          continue ;
        }

        // Verify that the message is of the expected type
        // Discard everything if the data is bunk (can cause additional packet loss)
        if (packet.header.bytes_to_follow != sizeOfInboundPayload(packet.header.payload_type)) {
          state = COMM_STATE_SYNC;
          receivedWords = 0;
          continue ;
        }

        // Header was valid, start receiving the payload
        state = COMM_STATE_PAYLOAD;
        packetLength = packet.header.bytes_to_follow + EXTRA_MESSAGE_DATA;

      case COMM_STATE_PAYLOAD:
        // Have not received data
        if (receivedWords < packetLength) {
          continue ;
        }

        // Process the message
        ProcessMessage(packet);

        // Clear out our payload
        memcpy(&packet.raw[0], &packet.raw[packetLength], receivedWords - packetLength);
        receivedWords -= packetLength;
        state = COMM_STATE_SYNC;
        continue ;
    }
  }

  // Clear any pending interrupts
  DMA1->IFCR = DMA_IFCR_CGIF5;
}
