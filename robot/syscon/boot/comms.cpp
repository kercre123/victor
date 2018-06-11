#include <string.h>

#include "comms.h"
#include "vectors.h"
#include "flash.h"
#include "common.h"
#include "hardware.h"
#include "power.h"
#include "analog.h"

#include "stm32f0xx.h"

#include "messages.h"

extern bool validate(void);

static struct {
  SpineMessageHeader header;
  union {
    MicroBodyToHead sync;
    AckMessage ack;
  } payload;
  SpineMessageFooter footer;
} outbound;

static struct {
  union {
    SpineMessageHeader header;
    uint8_t raw[];
  };
  union {
    WriteDFU writeDFU;
    uint8_t raw_payload[];
  };
  SpineMessageFooter _footer;
} inbound;

static const int EXTRA_BYTES = sizeof(SpineMessageHeader) + sizeof(SpineMessageFooter);
static volatile bool g_exitRuntime = false;
static Ack next_ack = ACK_UNUSED;
static void check_buffer();

static uint32_t crc(const void* ptr, int bytes) {
  int length = bytes / sizeof(uint32_t);
  const uint32_t* data = (const uint32_t*) ptr;

  CRC->CR = CRC_CR_RESET | CRC_CR_REV_IN | CRC_CR_REV_OUT;
  while (length-- > 0) {
    CRC->DR = *(data++);
  }
  return CRC->DR;
}

static void reset_dma() {
  // Turn off dma
  DMA1_Channel3->CCR = 0;

  // Reconfigure dma
  DMA1_Channel3->CPAR = (uint32_t)&USART1->RDR;
  DMA1_Channel3->CMAR = (uint32_t)&inbound;
  DMA1_Channel3->CNDTR = sizeof(inbound);
  DMA1_Channel3->CCR = DMA_CCR_MINC
                     | DMA_CCR_EN
                     ;
}

void Comms::run(void) {
  // CRC Poly
  CRC->INIT = ~0;

  // Configure our USART1 (Start interrupt)
  USART1->BRR = SYSTEM_CLOCK / COMMS_BAUDRATE;
  USART1->CR3 = USART_CR3_DMAR | USART_CR3_DMAT | USART_CR3_OVRDIS;
  USART1->CR2 = (SYNC_HEAD_TO_BODY & 0xFF) << 24;
  USART1->CR1 = USART_CR1_RE
              | USART_CR1_TE
              | USART_CR1_UE;

  // Setup our constants for outbound data
  outbound.header.sync_bytes = SYNC_BODY_TO_HEAD;
  outbound.header.bytes_to_follow = sizeof(outbound.payload);

  next_ack = ACK_BOOTED;

  // Start reading into a DMA buffer
  reset_dma();
  
  while (!g_exitRuntime) {
    __wfi();
    check_buffer();
  }

  DMA1_Channel3->CCR = 0;
  USART1->CR1 = 0;
}

void Comms::tick() {
  // Fire out ourbound data
  DMA1_Channel2->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR;

  DMA1_Channel2->CPAR = (uint32_t)&USART1->TDR;
  DMA1_Channel2->CMAR = (uint32_t)&outbound;
  DMA1_Channel2->CNDTR = sizeof(outbound);

  // Process sync / ack period
  if (next_ack == ACK_UNUSED) {
    // Frame a dummy packet
    outbound.header.payload_type = PAYLOAD_BOOT_FRAME;
    outbound.payload.sync.buttonPressed = Analog::button_pressed ? 1 : 0;
  } else {
    // Frame a dummy packet
    outbound.header.payload_type = PAYLOAD_ACK;
    outbound.payload.ack.status = next_ack;
    next_ack = ACK_UNUSED;
  }

  outbound.footer.checksum = crc(&outbound.payload, sizeof(outbound.payload));
  DMA1_Channel2->CCR |= DMA_CCR_EN;
}

static void check_buffer() {
  // Find where we are in our DMA buffer
  uint16_t writeIndex = sizeof(inbound) - DMA1_Channel3->CNDTR;

  // We've not finished receiving a header
  if (writeIndex < sizeof(SpineMessageHeader)) {
    return ;
  }

  // Sync failure
  if (inbound.header.sync_bytes != SYNC_HEAD_TO_BODY) {
    reset_dma();
    return ;
  }

  int inboundSize = inbound.header.bytes_to_follow + EXTRA_BYTES;

  // This packet is too large, ignore it
  if (inboundSize > sizeof(inbound)) {
    reset_dma();
    next_ack = NACK_BAD_COMMAND;
    return ;
  }

  // We've not finished receiving whole packet
  if (writeIndex < inboundSize) {
    return ;
  }

  //Ready to receive another packet
  reset_dma();

  uint32_t expected_crc = crc(&inbound.raw_payload, inbound.header.bytes_to_follow);
  SpineMessageFooter* message_end = (SpineMessageFooter*)(&inbound.raw_payload[inbound.header.bytes_to_follow]);

  if (expected_crc != message_end->checksum) {
    next_ack = NACK_CRC_FAILED;
    return ;
  }

  // Emergency eject in case of recovery mode
  BODY_TX::set();
  BODY_TX::mode(MODE_ALTERNATE);

  switch (inbound.header.payload_type) {
  case PAYLOAD_SHUT_DOWN:
    Power::setMode(POWER_STOP);
    break ;

  case PAYLOAD_ERASE:
    Flash::eraseApplication();
    break ;

  case PAYLOAD_VALIDATE:
    if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
      // If signature fails, wipe application
      if (!validate()) {
        Flash::eraseApplication();
        next_ack = NACK_CERT_FAILED;
        return ;
      }

      // Write fingerprint to signify that write completed
      Flash::writeFlash(&APP->fingerPrint, &COZMO_APPLICATION_FINGERPRINT, sizeof(APP->fingerPrint));
      g_exitRuntime = true;
    }
    break ;

  case PAYLOAD_DFU_PACKET:
    {
      const uint32_t word_size = inbound.writeDFU.wordCount * sizeof(inbound.writeDFU.data[0]);
      const uint32_t* dst = (uint32_t*)&APP->certificate[inbound.writeDFU.address];
      const uint32_t* src = inbound.writeDFU.data;

      // We can only work with aligned addresses
      if (inbound.writeDFU.address & 3) {
        next_ack = NACK_SIZE_ALIGN;
        return;
      }

      // Address is out of bounds
      if ((inbound.writeDFU.address + word_size + 8) > COZMO_APPLICATION_SIZE) {
        next_ack = NACK_BAD_ADDRESS;
        return ;
      }

      // Check if the space is erased
      for (int i = 0; i < inbound.writeDFU.wordCount; i++) {
        if (dst[i] != 0xFFFFFFFF) {
          next_ack = NACK_NOT_ERASED;
          return ;
        }
      }

      // Write flash
      Flash::writeFlash(dst, src, word_size);

      // Verify flash
      for (int i = 0; i < inbound.writeDFU.wordCount; i++) {
        if (dst[i] != src[i]) {
          next_ack = NACK_FLASH_FAILED;
          return ;
        }
      }
    }

    break ;

  default:
    next_ack = NACK_BAD_COMMAND;
    return ;
  }

  next_ack = ACK_PAYLOAD;
}
