#include <string.h>

#include "comms.h"
#include "vectors.h"
#include "flash.h"
#include "common.h"
#include "hardware.h"
#include "analog.h"
#include "contacts.h"

#include "stm32f0xx.h"

#include "messages.h"

extern bool validate(void);

extern "C" {
  extern const uint32_t  __HW_REVISION[];
  extern const uint32_t  __HW_MODEL[];
  extern const uint8_t   __EIN[16];
}

static struct {
  union {
    SpineMessageHeader header;
    uint8_t raw[];
  };
  union {
    ContactData contacts;
    WriteDFU write;
    uint8_t payload[];
  };
  SpineMessageFooter _footer;    
} inbound;

static const int EXTRA_BYTES = sizeof(SpineMessageHeader) + sizeof(SpineMessageFooter);
static volatile bool g_exitRuntime = false;

static volatile uint8_t tx_read_index;
static volatile uint8_t tx_write_index;
static uint8_t tx_out_buffer[0x100];

static uint32_t crc(const void* ptr, int bytes) {
  int length = bytes / sizeof(uint32_t);
  const uint32_t* data = (const uint32_t*) ptr;

  CRC->CR = CRC_CR_RESET | CRC_CR_REV_IN | CRC_CR_REV_OUT;
  while (length-- > 0) {
    CRC->DR = *(data++);
  }
  return CRC->DR;
}

static void enqueue(const void* data, int size) {
  const uint8_t* index = (const uint8_t*) data;
  
  while (size-- > 0) {
    tx_out_buffer[tx_write_index++] = *(index++);
  }
}

static void sendPayload(PayloadId id, const void* data, int size) {
  {
    SpineMessageHeader header;
    
    header.sync_bytes = SYNC_BODY_TO_HEAD;
    header.bytes_to_follow = size;
    header.payload_type = id;
    enqueue(&header, sizeof(header));
  }
  
  enqueue(data, size);

  {
    SpineMessageFooter footer;
    footer.checksum = crc(data, size);
    enqueue(&footer, sizeof(footer));
  }

  // Start sending out data
  USART1->CR1 |= USART_CR1_TXEIE;
}

static void sendVersion() {
  VersionInfo version;

  version.hw_revision = __HW_REVISION[0];
  version.hw_model = __HW_MODEL[0];
  memcpy(&version.ein, __EIN, sizeof(version.ein));
  memcpy(&version.app_version, APP->applicationVersion, sizeof(version.app_version));

  sendPayload(PAYLOAD_VERSION, &version, sizeof(version));
}

static void sendAck(Ack c) {
  AckMessage ack;

  // Frame a dummy packet
  ack.status = c;

  sendPayload(PAYLOAD_ACK, &ack, sizeof(ack));
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

  tx_read_index = 0;
  tx_write_index = 0;

  // Hold high for a random time
  for (int i = 100; i; i--) __asm("nop");
  sendAck(ACK_PAYLOAD);
  
  // Configure our interrupts
  NVIC_SetPriority(USART1_IRQn, PRIORITY_CONTACTS_COMMS); // Match priority so interrupts don't preempt each other
  NVIC_EnableIRQ(USART1_IRQn);

  while (!g_exitRuntime) {
    ContactData outbound;

    if (Contacts::transmit(outbound)) {
      NVIC_DisableIRQ(USART1_IRQn);
      sendPayload(PAYLOAD_CONT_DATA, &outbound, sizeof(outbound));
      NVIC_EnableIRQ(USART1_IRQn);
    }

    __asm("WFI");
  }

  NVIC_DisableIRQ(USART1_IRQn);
}

extern "C" void USART1_IRQHandler(void) {
  // Start dequeueing fifo data
  if (USART1->ISR & USART_ISR_TXE) {
    USART1->TDR = tx_out_buffer[tx_read_index++];
    if (tx_read_index == tx_write_index) {
      USART1->CR1 &= ~USART_CR1_TXEIE;
    }
  }
  
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
  static uint16_t writeIndex = 0;
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

  uint32_t expected_crc = crc(inbound.payload, inbound.header.bytes_to_follow);
  SpineMessageFooter* footer = (SpineMessageFooter*)&inbound.payload[inbound.header.bytes_to_follow];

  if (expected_crc != footer->checksum) {
    sendAck(NACK_CRC_FAILED);
    return ;
  }

  switch (inbound.header.payload_type) {
  case PAYLOAD_VERSION:
    sendVersion();
    return ;

  case PAYLOAD_CONT_DATA:
    Contacts::forward(inbound.contacts);
    return ;

  case PAYLOAD_MODE_CHANGE:
    if (APP->fingerPrint != COZMO_APPLICATION_FINGERPRINT) {
      sendAck(NACK_NOT_VALID);
      return ;
    }

    g_exitRuntime = true;
    return ;

  case PAYLOAD_ERASE:
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
    }
    break ;

  case PAYLOAD_DFU_PACKET:
    {
      const uint32_t word_size = inbound.write.wordCount * sizeof(inbound.write.data[0]);
      const uint32_t* dst = (uint32_t*)&APP->certificate[inbound.write.address];
      const uint32_t* src = inbound.write.data;

      // We can only work with aligned addresses
      if (inbound.write.address & 3) {
        sendAck(NACK_SIZE_ALIGN);
        return;
      }

      // Address is out of bounds
      if ((inbound.write.address + word_size + 8) > COZMO_APPLICATION_SIZE) {
        sendAck(NACK_BAD_ADDRESS);
        return ;
      }

      // Check if the space is erased
      for (int i = 0; i < inbound.write.wordCount; i++) {
        if (dst[i] != 0xFFFFFFFF) {
          sendAck(NACK_NOT_ERASED);
          return ;
        }
      }

      // Write flash
      Flash::writeFlash(dst, src, word_size);

      // Verify flash
      for (int i = 0; i < inbound.write.wordCount; i++) {
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
