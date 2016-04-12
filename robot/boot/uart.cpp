#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"
#include "anki/cozmo/robot/spineData.h"

#include "uart.h"
#include "timer.h"

enum TRANSFER_MODE {
  TRANSMIT_RECEIVE,
  TRANSMIT_SEND
};

#define BAUD_SBR(baud)  (CORE_CLOCK * 2 / baud / 32)
#define BAUD_BRFA(baud) (CORE_CLOCK * 2 / baud % 32)

static const int uart_fifo_size = 8;
static TRANSFER_MODE current_mode;

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
	UART0_S2 |= UART_S2_RXINV_MASK;
	UART0_C3 = UART_C3_TXINV_MASK;
	UART0_C4 = UART_C4_BRFA(BAUD_BRFA(spine_baud_rate));

	UART0_PFIFO = UART_PFIFO_TXFE_MASK | UART_PFIFO_TXFIFOSIZE(2) | UART_PFIFO_RXFE_MASK | UART_PFIFO_RXFIFOSIZE(2) ;
	UART0_CFIFO = UART_CFIFO_TXFLUSH_MASK | UART_CFIFO_RXFLUSH_MASK ;

	transmit_mode(TRANSMIT_RECEIVE);
}

void Anki::Cozmo::HAL::UART::shutdown(void) {
	// Disable our uart (So it will not lock up while the K02 is booting the application layer
	SIM_SCGC4 &= ~SIM_SCGC4_UART0_MASK;
	SIM_SOPT5 &= ~(SIM_SOPT5_UART0TXSRC_MASK | SIM_SOPT5_UART0RXSRC_MASK);
}


/*
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"


#include "spi.h"
#include "uart.h"
#include "spine.h"
*/

inline void transmit_mode(TRANSFER_MODE mode) { 
	switch (mode) {
    case TRANSMIT_SEND:
    {
      PORTD_PCR6 = PORT_PCR_MUX(0);
      PORTD_PCR7 = PORT_PCR_MUX(3);
      UART0_C2 = UART_C2_TE_MASK;
      break ;
    }
    case TRANSMIT_RECEIVE:
    {
      PORTD_PCR6 = PORT_PCR_MUX(3);
      PORTD_PCR7 = PORT_PCR_MUX(0);
      UART0_C2 = UART_C2_RE_MASK;
      break ;
    }
    default:
    {
      break ;
    }
  }
}

void Anki::Cozmo::HAL::UART::read(void* p, int length) {
	transmit_mode(TRANSMIT_RECEIVE);

	uint8_t* data = (uint8_t*) p;
	for (int i = 0; i < 2; i++) {
		while (!UART0_RCFIFO) ;
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
}
