#include <string.h>

#include "board.h"
#include "uart.h"
#include "datasheet.h"

//------------------------------------------------
//    Board
//------------------------------------------------

void hal_uart_init(void)
{
	// UART
	GPIO_INIT_PIN(UTX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
	GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
}

void hal_uart_stop(void) {
	// Do nothing.
}
