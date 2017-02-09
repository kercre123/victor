#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"
#include "anki/cozmo/robot/spineData.h"

#include "uart.h"
#include "timer.h"

#include "portable.h"
#include "../hal/hardware.h"

enum TRANSFER_MODE {
  TRANSMIT_UNKNOWN,
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

static const int UART_CORE_CLOCK = 32768*2560;

#define BAUD_SBR(baud)  (UART_CORE_CLOCK * 2 / (baud) / 32)
#define BAUD_BRFA(baud) (UART_CORE_CLOCK * 2 / (baud) % 32)

static const int uart_fifo_size = 8;
static TRANSFER_MODE current_mode = TRANSMIT_UNKNOWN;

inline void transmit_mode(TRANSFER_MODE mode);

void Anki::Cozmo::HAL::UART::init(void) {
  // Enable clocking to the UART and PORTD
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
  SIM_SOPT5 |= SIM_SOPT5_UART0TXSRC(0) | SIM_SOPT5_UART0RXSRC(0);

  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;

  // Enable UART for this shiz
  UART0_BDL = UART_BDL_SBR(BAUD_SBR(spine_baud_rate));
  UART0_BDH = 0;

  UART0_C1 = 0; // 8 bit, 1 bit stop no parity (single wire)
  UART0_S2 |= UART_S2_RXINV_MASK; // Invert RX - blame the level converter
  UART0_C3 = UART_C3_TXINV_MASK;  // Invert TX - that means idle is 'low'
  UART0_C4 = UART_C4_BRFA(BAUD_BRFA(spine_baud_rate));

  UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(2) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(2) ;
  UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK ;

  SOURCE_SETUP(BODY_UART_RX, SourceAlt3);
  SOURCE_SETUP(BODY_UART_TX, SourceAlt3 | SourcePullDown);
}

void Anki::Cozmo::HAL::UART::shutdown(void) {
  // Disable our uart (So it will not lock up while the K02 is booting the application layer
  SIM_SCGC4 &= ~SIM_SCGC4_UART0_MASK;
  SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
}

inline void transmit_mode(TRANSFER_MODE mode) { 
  if (mode == current_mode) {
    return ;
  }
  current_mode = mode;

  switch (mode) {
    case TRANSMIT_SEND:
      UART0_C2 = UART_C2_TE_MASK;
      break ;
    case TRANSMIT_RECEIVE:
      UART0_C2 = UART_C2_RE_MASK;
      break ;
    default:
      break ;
  }
}

void Anki::Cozmo::HAL::UART::read(void* p, int length) {
  transmit_mode(TRANSMIT_RECEIVE);

  uint8_t* data = (uint8_t*) p;
  while (length-- > 0) {
    while (!UART0_RCFIFO) {
      MicroWait(1);
    }
    
    *(data++) = UART0_D;
  }
}

void Anki::Cozmo::HAL::UART::write(const void* p, int length) {
  transmit_mode(TRANSMIT_SEND);

  const uint8_t* data = (const uint8_t*) p;
  while (length-- > 0) {
    while (UART0_TCFIFO >= uart_fifo_size) ;
    UART0_D = *(data++);
  }

  while (~UART0_S1 & UART_S1_TC_MASK) ;
}

void Anki::Cozmo::HAL::UART::flush(void) {
  UART0->CFIFO |= UART_CFIFO_RXFLUSH_MASK;
  UART0_S1 = UART0_S1;
}

void Anki::Cozmo::HAL::UART::receive(void) {
  UART::flush();
  transmit_mode(TRANSMIT_RECEIVE);
}

bool Anki::Cozmo::HAL::UART::rx_avail(void){
  return UART0_RCFIFO > 0;
}

void Anki::Cozmo::HAL::UART::writeByte(uint8_t byte) {
  write(&byte, sizeof(byte));
}

uint8_t Anki::Cozmo::HAL::UART::readByte(void) {
  uint8_t word;
  read(&word, sizeof(word));

  return word;
}
