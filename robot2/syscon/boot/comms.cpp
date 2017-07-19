#include <string.h>

#include "comms.h"
#include "vectors.h"
#include "flash.h"
#include "common.h"
#include "hardware.h"

#include "stm32f0xx.h"

#include "schema/messages.h"

using namespace Anki::Cozmo::Spine;

extern bool validate(void);

extern "C" {
  extern const uint32_t  __HW_REVISION[];
  extern const uint32_t  __HW_MODEL[];
  extern const uint8_t   __EIN[16];
}

static struct {
  SpineMessageHeader header;
  union {
    struct {
      AckMessage payload;
      SpineMessageFooter footer;
    } ack;
    struct {
      VersionInfo payload;
      SpineMessageFooter footer;
    } version;
  };
} outbound;

static struct {
  union {
    SpineMessageHeader header;
    uint8_t raw[];
  };
  union {
    struct {
      WriteDFU payload;
      SpineMessageFooter footer;
    } write;
    struct {
      SpineMessageFooter footer;
    } null;
  };
} inbound;

static const int EXTRA_BYTES = sizeof(SpineMessageHeader) + sizeof(SpineMessageFooter);
static volatile bool g_exitRuntime = false;

static uint32_t crc(const void* ptr, int bytes) {
  int length = bytes / sizeof(uint32_t);
  const uint32_t* data = (const uint32_t*) ptr;

  CRC->CR = CRC_CR_RESET | CRC_CR_REV_IN | CRC_CR_REV_OUT;
  while (length-- > 0) {
    CRC->DR = *(data++);
  }
  return CRC->DR;
}

static void sendPayload() {
  // Fire out ourbound data
  DMA1_Channel2->CCR = 0;
  DMA1_Channel2->CPAR = (uint32_t)&USART1->TDR;
  DMA1_Channel2->CMAR = (uint32_t)&outbound;
  DMA1_Channel2->CNDTR = outbound.header.bytes_to_follow + EXTRA_BYTES;

  DMA1_Channel2->CCR = DMA_CCR_MINC
                     | DMA_CCR_DIR
                     | DMA_CCR_EN;
}

static void sendVersion() {
  outbound.header.payload_type = PAYLOAD_VERSION;
  outbound.header.bytes_to_follow = sizeof(outbound.version.payload);

  outbound.version.payload.hw_revision = __HW_REVISION[0];
  outbound.version.payload.hw_model = __HW_MODEL[0];
  memcpy(&outbound.version.payload.ein, __EIN, sizeof(outbound.version.payload.ein));
  memcpy(&outbound.version.payload.app_version, APP->applicationVersion, sizeof(outbound.version.payload.app_version));
  outbound.version.footer.checksum = crc(&outbound.version.payload, sizeof(outbound.version.payload));

  sendPayload();
}

static void sendAck(Ack c) {

  // Frame a dummy packet
  outbound.header.payload_type = PAYLOAD_ACK;
  outbound.header.bytes_to_follow = sizeof(outbound.ack.payload);
  outbound.ack.payload.status = c;
  outbound.ack.footer.checksum = crc(&outbound.ack.payload, sizeof(outbound.ack.payload));

  sendPayload();
}

void Comms::run(void) {
  // CRC Poly
  CRC->INIT = ~0;

  // Configure our GPIO to be wired into USART1
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
  USART1->CR3 = USART_CR3_DMAT | USART_CR3_OVRDIS;
  USART1->CR2 = (SYNC_HEAD_TO_BODY & 0xFF) << 24;
  USART1->CR1 = USART_CR1_RE
              | USART_CR1_TE
              | USART_CR1_CMIE
              | USART_CR1_UE;


  // Setup our constants for outbound data
  outbound.header.sync_bytes = SYNC_BODY_TO_HEAD;

  // Configure our interrupts
  for (int i = 100; i; i--) __asm("nop");
  sendAck(ACK_PAYLOAD);
  NVIC_SetPriority(USART1_IRQn, 2);
  NVIC_EnableIRQ(USART1_IRQn);

  while (!g_exitRuntime) {
    if (APP->fingerPrint == COZMO_APPLICATION_FINGERPRINT) {
      APP->visitorTick();
    }

    __asm("WFI");
  }

  NVIC_DisableIRQ(USART1_IRQn);
}

extern "C" void USART1_IRQHandler(void) {
  static uint16_t writeIndex = 0;

  // Character match (used to mark the start of a packet)
  if (USART1->ISR & USART_ISR_CMF) {
    USART1->ICR = USART_ICR_CMCF;
    USART1->CR1 |= USART_CR1_RXNEIE;
  }

  // Flush overflows
  if (USART1->ISR & USART_ISR_ORE) {
    USART1->ICR = USART_ICR_ORECF;
  }

  // No data available
  if (~USART1->ISR & USART_ISR_RXNE) {
    return ;
  }

  // Receive data
  inbound.raw[writeIndex++] = USART1->RDR;

  // We've not finished receiving a header
  if (writeIndex < sizeof(SpineMessageHeader)) {
    return ;
  }

  // Sync failure
  if (inbound.header.sync_bytes != SYNC_HEAD_TO_BODY) {
    USART1->CR1 &= ~USART_CR1_RXNEIE;
    USART1->RQR = USART_RQR_RXFRQ;
    writeIndex = 0;
    return ;
  }

  int inboundSize = inbound.header.bytes_to_follow + EXTRA_BYTES;

  // This packet is too large, ignore it
  if (inboundSize > sizeof(inbound)) {
    USART1->CR1 &= ~USART_CR1_RXNEIE;
    USART1->RQR = USART_RQR_RXFRQ;
    writeIndex = 0;
    return ;
  }

  // We've not finished receiving whole packet
  if (writeIndex < inboundSize) {
    return ;
  }

  //Ready to receive another packet
  USART1->CR1 &= ~USART_CR1_RXNEIE;
  USART1->RQR = USART_RQR_RXFRQ;
  writeIndex = 0;

  uint32_t expected_crc = crc(&inbound.null, inbound.header.bytes_to_follow);
  uint8_t* message_end = (__packed uint8_t*)(&inbound.null) + inbound.header.bytes_to_follow;
  uint32_t received_crc = *(__packed uint32_t*)message_end;

  if (expected_crc != received_crc) {
    sendAck(NACK_CRC_FAILED);
    return ;
  }

  switch (inbound.header.payload_type) {
  case PAYLOAD_VERSION:
    sendVersion();
    return ;

  case PAYLOAD_MODE_CHANGE:
    if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
      sendAck(NACK_NOT_VALID);
      return ;
    }

    g_exitRuntime = true;
    return ;

  case PAYLOAD_ERASE:
    if (APP->fingerPrint == COZMO_APPLICATION_FINGERPRINT) {
      APP->visitorStop();
    }

    Flash::eraseApplication();
    break ;

  case PAYLOAD_VALIDATE:
    if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
      // If signature fails, wipe application
      if (!validate()) {
        Flash::eraseApplication();
        sendAck(NACK_CERT_FAILED);
        return ;
      }

      // Write fingerprint to signify that write completed
      Flash::writeFlash(&APP->fingerPrint, &COZMO_APPLICATION_FINGERPRINT, sizeof(APP->fingerPrint));
      APP->visitorInit();
    }
    break ;

  case PAYLOAD_DFU_PACKET:
    {
      const uint32_t word_size = inbound.write.payload.wordCount * sizeof(inbound.write.payload.data[0]);
      const uint32_t* dst = (uint32_t*)&APP->certificate[inbound.write.payload.address];
      const uint32_t* src = inbound.write.payload.data;

      // We can only work with aligned addresses
      if (inbound.write.payload.address & 3) {
        sendAck(NACK_SIZE_ALIGN);
        return;
      }

      // Address is out of bounds
      if ((inbound.write.payload.address + word_size + 8) > COZMO_APPLICATION_SIZE) {
        sendAck(NACK_BAD_ADDRESS);
        return ;
      }

      // Check if the space is erased
      for (int i = 0; i < inbound.write.payload.wordCount; i++) {
        if (dst[i] != 0xFFFFFFFF) {
          sendAck(NACK_NOT_ERASED);
          return ;
        }
      }

      // Write flash
      Flash::writeFlash(dst, src, word_size);

      // Verify flash
      for (int i = 0; i < inbound.write.payload.wordCount; i++) {
        if (dst[i] != src[i]) {
          sendAck(NACK_FLASH_FAILED);
          return ;
        }
      }
    }

    break ;

  default:
    sendAck(NACK_BAD_COMMAND);
    return ;
  }

  sendAck(ACK_PAYLOAD);
}
