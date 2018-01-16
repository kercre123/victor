#include <string.h>

#include "common.h"
#include "hardware.h"
#include "analog.h"

#include "contacts.h"
#include "comms.h"

static ContactData rxData;
static int rxDataIndex;

static ContactData txData;
static int txDataIndex;

static const ContactData boot_msg = {"\xFF\xFF\xFF\xFF\nbooted\n"};

void Contacts::init(void) {
  VEXT_TX::alternate(1);  // USART2_TX
  VEXT_TX::speed(SPEED_HIGH);
  VEXT_TX::pull(PULL_NONE);
  VEXT_TX::mode(MODE_ALTERNATE);

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
  
  memset(&rxData, 0, sizeof(ContactData));
  memset(&txData, 0, sizeof(ContactData));
  rxDataIndex = 0;
  txDataIndex = 0;
  
  Contacts::forward( boot_msg );
}

void Contacts::forward(const ContactData& pkt) {
  Analog::delayCharge();

  NVIC_DisableIRQ(USART2_IRQn);
  memcpy(&txData, &pkt, sizeof(ContactData));
  txDataIndex = 0;

  USART2->CR1 |= USART_CR1_TXEIE;
  NVIC_EnableIRQ(USART2_IRQn);
}

void Contacts::tick(void) {
  if (rxDataIndex > 0) {
    ContactData pkt;

    NVIC_DisableIRQ(USART2_IRQn);
    memcpy(&pkt, &rxData, sizeof(ContactData));  
    memset(&rxData, 0, sizeof(ContactData));
    rxDataIndex = 0;
    NVIC_EnableIRQ(USART2_IRQn);

    Comms::enqueue(PAYLOAD_CONT_DATA, &pkt, sizeof(pkt));
  }
}

extern "C" void USART2_IRQHandler(void) {
  // Transmit data
  if (USART2->ISR & USART_ISR_TXE) {
    if (txDataIndex < sizeof(txData)) {
      uint8_t byte = txData.data[txDataIndex++];

      if (byte > 0) { 
        USART2->TDR = byte;
      } else {
        txDataIndex = sizeof(txData);
      }
    }

    if (txDataIndex >= sizeof(txData)) {
      USART2->CR1 &= ~USART_CR1_TXEIE;
    }
  }

  // Receive data
  if (USART2->ISR & USART_ISR_RXNE) {
    volatile uint8_t rxd = USART2->RDR;

    Analog::delayCharge();
    if (rxDataIndex < sizeof(rxData.data)) {
      rxData.data[rxDataIndex++] = rxd;
    }
  }
}
  