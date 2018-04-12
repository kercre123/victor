#include <string.h>

#include "common.h"
#include "hardware.h"
#include "analog.h"

#include "contacts.h"
#include "comms.h"
#include "timer.h"

static ContactData rxData;
static int rxDataIndex;

static uint8_t txData[64];
static int txReadIndex;
static int txWriteIndex;
static bool transmitting;

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

  memset(&rxData, 0, sizeof(rxData));
  rxDataIndex = 0;
  txReadIndex = 0;
  txWriteIndex = 0;
  transmitting = false;

  Contacts::forward( boot_msg );
}

void Contacts::forward(const ContactData& pkt) {
  NVIC_DisableIRQ(USART2_IRQn);

  for (int i = 0; i < sizeof(pkt.data); i++) {
    uint8_t byte = pkt.data[i];
    if (!byte) continue ;

    txData[txWriteIndex++] = byte;

    if (txWriteIndex >= sizeof(txData)) txWriteIndex = 0;

    if (!transmitting) {
      transmitting = true;
      Analog::delayCharge();
      USART2->CR1 &= ~USART_CR1_RE;
      USART2->TDR = 0xFF;
      VEXT_TX::pull(PULL_UP);
      USART2->CR1 |= USART_CR1_TXEIE;
    }
  }

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
    if (txReadIndex != txWriteIndex) {
      USART2->TDR = txData[txReadIndex++];
      if (txReadIndex >= sizeof(txData)) txReadIndex = 0;
    } else {
      USART2->CR1 &= ~USART_CR1_TXEIE;
      USART2->CR1 |= USART_CR1_TCIE;
    }
  }

  if (USART2->ISR & USART_ISR_TC) {
    if (txReadIndex == txWriteIndex) {
      VEXT_TX::pull(PULL_NONE);
      USART2->CR1 &= ~USART_CR1_TCIE;
      USART2->CR1 |= USART_CR1_RE;
      transmitting = false;
    }
  }
  
  // Receive data
  if (USART2->ISR & USART_ISR_RXNE) {
    uint32_t status = USART2->ISR;
    volatile uint8_t rxd = USART2->RDR;
    
    Analog::delayCharge();
    
    if( status & (USART_ISR_ORE | USART_ISR_FE) ) { //framing and/or overrun error
      //rxd = USART2->RDR; //flush the rdr & shift register
      //rxd = USART2->RDR;
      USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF; //clear flags
    } else if (rxDataIndex < sizeof(rxData.data)) {
      rxData.data[rxDataIndex++] = rxd;
    }
  }
}
