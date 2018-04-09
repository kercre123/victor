/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART driver with GPIO toggle
 *
 * Copyright (C) RivieraWaves 2009-2012
 *
 * $Rev: 6880 $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup UART
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stddef.h>     // standard definition
#include "uart.h"       // uart definition
#include "reg_uart.h"   // uart register
#include "hcic_eif.h"   // hci uart definition
#include "global_io.h"

#include "rwble_config.h"
#include "user_periph_setup.h"

#include "gpio.h"

#ifdef CFG_WAKEUP_EXT_PROCESSOR
/*
 * STRUCT DEFINITIONS
 *****************************************************************************************
 */
/* TX and RX channel class holding data used for asynchronous read and write data
 * transactions
 */
/// UART TX RX Channel
struct uart_txrxchannel
{
    /// size
    uint32_t  size;
    /// buffer pointer
    uint8_t  *bufptr;
    /// callback function pointer
    void (*callback) (uint8_t);
};

/// UART environment structure
struct uart_env_tag
{
    /// tx channel
    struct uart_txrxchannel tx;
    /// rx channel
    struct uart_txrxchannel rx;
    /// error detect
    uint8_t errordetect;
};

/**
 ****************************************************************************************
 * @brief Serves the transmit data fill interrupt requests. It clears the requests and
 *        executes the callback function.
 *
 * The callback function is called as soon as the last byte of the provided data is
 * put into the FIFO. The interrupt is disabled at the same time.
 ****************************************************************************************
 */
// Declared here, since it is defined as static in uart.c file.
void uart_thr_empty_isr(void);

extern struct uart_env_tag uart_env;

void uart_write_ext_wkup_func(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t))
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(uart_env.tx.bufptr == NULL);
	
		GPIO_SetActive (EXT_WAKEUP_PORT, EXT_WAKEUP_PIN);
			
    // Prepare TX parameters
    uart_env.tx.size = size;
    uart_env.tx.bufptr = bufptr;
		uart_env.tx.callback = callback;

    /* start data transaction
     * first isr execution is done without interrupt generation to reduce
     * interrupt load
     */
    uart_thr_empty_isr();
    if (uart_env.tx.bufptr != NULL)
    {
			uart_thr_empty_setf(1);
    }
		GPIO_SetInactive (EXT_WAKEUP_PORT, EXT_WAKEUP_PIN);
}

#endif //CFG_WAKEUP_EXT_PROCESSOR

/// @} UART
