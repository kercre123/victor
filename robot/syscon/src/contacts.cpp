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

static inline void set_rx() {
  USART2->CR1 = 0;
  __nop(); __nop(); __nop(); __nop(); __nop();

  USART2->RQR = USART_RQR_RXFRQ;
  USART2->BRR = SYSTEM_CLOCK / CONTACT_BAUDRATE;
  USART2->CR3 = USART_CR3_HDSEL;
  USART2->CR1 = USART_CR1_UE
              | USART_CR1_RE
              | USART_CR1_TE
              | USART_CR1_RXNEIE
              ;

  USART2->TDR = 0xFF;

  transmitting = false;
}

static inline void set_tx() {
  USART2->CR1 = 0;
  __nop(); __nop(); __nop(); __nop(); __nop();

  USART2->BRR = SYSTEM_CLOCK / CONTACT_BAUDRATE;
  USART2->CR3 = 0;
  USART2->CR1 = USART_CR1_UE
              | USART_CR1_TE
              | USART_CR1_TXEIE
              ;

  USART2->TDR = 0xFF;

  transmitting = true;
}

void Contacts::init(void) {
  VEXT_TX::alternate(1);  // USART2_TX
  VEXT_TX::pull(PULL_NONE);

  // Configure our USART2
  set_rx();
  
  NVIC_SetPriority(USART2_IRQn, PRIORITY_CONTACTS_COMMS);
  NVIC_EnableIRQ(USART2_IRQn);

  memset(&rxData, 0, sizeof(rxData));
  rxDataIndex = 0;
  txReadIndex = 0;
  txWriteIndex = 0;

  Contacts::forward( boot_msg );
}

#define DEBUG_TX_OVF_DETECT 0
void Contacts::forward(const ContactData& pkt) {
  NVIC_DisableIRQ(USART2_IRQn);

  Analog::delayCharge();

  for (int i = 0; i < sizeof(pkt.data); i++) {
    uint8_t byte = pkt.data[i];
    if (!byte) continue ;

    //prevent buffer wrap by dropping chars
    int nextWriteIndex = txWriteIndex+1 >= sizeof(txData) ? 0 : txWriteIndex+1;
    if( nextWriteIndex == txReadIndex ) continue;

    #if DEBUG_TX_OVF_DETECT > 0
    #warning contact overflow detection is active
      int lastReadIndex = txReadIndex-1 < 0 ? sizeof(txData)-1 : txReadIndex-1;
      if( nextWriteIndex == lastReadIndex ) //'overflow' 1 char early so we can insert debug value
        byte = 0x88; //out of ascii range
    #endif

    txData[txWriteIndex] = byte;
    txWriteIndex = nextWriteIndex;

    if (!transmitting) {
      set_tx();
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
  if (transmitting)
  {
    if( USART2->CR1 & USART_CR1_TCIE ) {  //STATE: transmit-complete
      if (txReadIndex != txWriteIndex) {  //corner-case: more data arrived between TXE and TC ints
        USART2->CR1 = USART_CR1_UE
                    | USART_CR1_TE
                    | USART_CR1_TXEIE
                    ;
      } else {
        set_rx();
      }
    }
    
    if( USART2->CR1 & USART_CR1_TXEIE ) { //STATE: transmit-active
      if (txReadIndex != txWriteIndex) {
        if (USART2->ISR & USART_ISR_TXE) {
          USART2->TDR = txData[txReadIndex++];
          if (txReadIndex >= sizeof(txData)) txReadIndex = 0;
        } else {
          //unhandled error
        }
      } else { //final byte. wait for transmit complete
        USART2->CR1 = USART_CR1_UE
                    | USART_CR1_TE
                    | USART_CR1_TCIE
                    ;
      }
    }
    
    return ;
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
