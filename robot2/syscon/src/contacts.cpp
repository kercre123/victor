#include <string.h>

#include "common.h"
#include "hardware.h"

#include "contacts.h"

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
              // This has to be disabled while we are transmitting
              //| USART_CR1_RE
              | USART_CR1_RXNEIE
              ;

  NVIC_SetPriority(USART2_IRQn, PRIORITY_CONTACTS_COMMS);
  NVIC_EnableIRQ(USART2_IRQn);
}

void Contacts::stop(void) {
}

void Contacts::tick(void) {
}

extern "C" void USART2_IRQHandler(void) {
}
