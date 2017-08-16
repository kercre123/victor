#include <string.h>

#include "common.h"
#include "hardware.h"

#include "contacts.h"

static ContactData rxData;
static int rxDataIndex;

static ContactData txData;
static int txDataIndex;

void Contacts::init(void) {
  VEXT_SENSE::alternate(1);  // USART2_TX
  VEXT_SENSE::speed(SPEED_HIGH);
  VEXT_SENSE::pull(PULL_NONE);
  VEXT_SENSE::mode(MODE_ALTERNATE);

  // Configure our USART2
  USART2->BRR = SYSTEM_CLOCK / CONTACT_BAUDRATE;

  USART2->CR3 = USART_CR3_HDSEL;
  USART2->CR1 = USART_CR1_UE
              | USART_CR1_TE
              | USART_CR1_RE
              | USART_CR1_RXNEIE
              ;

  NVIC_SetPriority(USART2_IRQn, PRIORITY_CONTACTS_COMMS);
  NVIC_EnableIRQ(USART2_IRQn);
  
  memset(rxData.data, 0, sizeof(ContactData));
  rxDataIndex = 0;
  txDataIndex = 0;
}

static void txNextByte() {
  const uint8_t byte = txData.data[txDataIndex++];

  if (byte > 0) {
    USART2->TDR = byte;
  }
  
  if (!byte || txDataIndex >= sizeof(txData)) {
    USART2->CR1 &= ~USART_CR1_TXEIE;
  }
}

void Contacts::forward(const ContactData& pkt) {
  memcpy(txData.data, pkt.data, sizeof(txData.data));
  txDataIndex = 0;
  
  USART2->CR1 |= ~USART_CR1_TXEIE;
  txNextByte();
}

bool Contacts::transmit(ContactData& pkt) {
  if (rxDataIndex <= 0) {
    return false;
  }
  
  memcpy(pkt.data, rxData.data, rxDataIndex);
  memset(rxData.data, 0, sizeof(pkt.data));
  rxDataIndex = 0;
  return true;
}

extern "C" void USART2_IRQHandler(void) {
  if (USART2->ISR & USART_ISR_TXE) {
    txNextByte();
  }

  // Receive data
  if (USART2->ISR & USART_ISR_RXNE) {
    volatile uint8_t rxd = USART2->RDR;

    if (rxDataIndex < sizeof(rxData.data)) {
      rxData.data[rxDataIndex++] = rxd;
    }
  }
}
