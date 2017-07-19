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

  USART2->CR1 = USART_CR1_UE
              | USART_CR1_TE
              ;
}

void Contacts::stop(void) {
}

void Contacts::putc(uint8_t ch) {
  while (~USART2->ISR & USART_ISR_TXE) ;
  USART2->TDR = ch;
}

void Contacts::tick(void) {
}
