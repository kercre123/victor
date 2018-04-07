/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART driver
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

/*
 * DEFINES
 *****************************************************************************************
 */

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

enum UART_ID
{
    MODEM_STAT         = 0,
    NO_INT_PEND        = 1,
    THR_EMPTY          = 2,
    RECEIVED_AVAILABLE = 4,
    UART_TIMEOUT       = 12
};

/// RX_LVL values
enum UART_RXLVL
{
    UART_RXLVL_1,
    UART_RXLVL_4,
    UART_RXLVL_8,
    UART_RXLVL_14
};

/// WORD_LEN values
enum UART_WORDLEN
{
    UART_WORDLEN_5,
    UART_WORDLEN_6,
    UART_WORDLEN_7,
    UART_WORDLEN_8
};

/// FIFO_SZ values
enum UART_FIFOSIZE
{
    UART_FIFOSIZE_16,
    UART_FIFOSIZE_32,
    UART_FIFOSIZE_64,
    UART_FIFOSIZE_128
};

/*
 * STRUCT DEFINITIONS
 *****************************************************************************************
 */

/* TX and RX channel class holding data used for asynchronous read and write data
 * transactions
 */
/// UART TX RX Channel
struct uart2_txrxchannel
{
    /// size
    uint32_t  size;
    /// buffer pointer
    uint8_t  *bufptr;
    /// call back function pointer
    void (*callback) (uint8_t);
};

/// UART environment structure
struct uart2_env_tag
{
    /// tx channel
    struct uart2_txrxchannel tx;
    /// rx channel
    struct uart2_txrxchannel rx;
    /// error detect
    uint8_t errordetect;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// uart environment structure
static struct uart2_env_tag uart2_env __attribute__((section("retention_mem_area0"),zero_init));


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Serves the receive data available interrupt requests. It clears the requests and
 *        executes the callback function.
 *
 ****************************************************************************************
 */
static void uart2_rec_data_avail_isr(void)
{
    void (*callback) (uint8_t) = NULL;

    while (uart2_data_rdy_getf())
    {
        // Read the received in the FIFO
        *uart2_env.rx.bufptr = uart2_rxdata_getf();

        // Update RX parameters
        uart2_env.rx.size--;
        uart2_env.rx.bufptr++;

        // Check if all expected data have been received
        if (uart2_env.rx.size == 0)
        {
            // Reset RX parameters
            uart2_env.rx.bufptr = NULL;

            // Disable Rx interrupt
            uart2_rec_data_avail_setf(0);

            // Retrieve callback pointer
            callback = uart2_env.rx.callback;

            if(callback != NULL)
            {
                // Clear callback pointer
                uart2_env.rx.callback = NULL;

                // Call handler
                callback(UART_STATUS_OK);
            }
            else
            {
                ASSERT_ERR(0);
            }

            // Exit loop
            break;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Serves the receive data error interrupt requests. It clears the requests and
 *        executes the callback function.
 *
 ****************************************************************************************
 */
static void uart2_rec_error_isr(void)
{
    void (*callback) (uint8_t) = NULL;
    // Reset RX parameters
    uart2_env.rx.size = 0;
    uart2_env.rx.bufptr = NULL;

    // Disable RX interrupt
    SetBits16(UART2_IER_DLH_REG, ERBFI_dlh0, 0); // uart2_rec_data_avail_setf(0);

    // Retrieve callback pointer
    callback = uart2_env.rx.callback;

    if(callback != NULL)
    {
        // Clear callback pointer
        uart2_env.rx.callback = NULL;

        // Call handler
        callback(UART_STATUS_ERROR);
    }
    else
    {
        ASSERT_ERR(0);
    }
}

/**
 ****************************************************************************************
 * @brief Serves the transmit data fill interrupt requests. It clears the requests and
 *        executes the callback function.
 *
 * The callback function is called as soon as the last byte of the provided data is
 * put into the FIFO. The interrupt is disabled at the same time.
 ****************************************************************************************
 */
static void uart2_thr_empty_isr(void)
{
    void (*callback) (uint8_t) = NULL;
    // Fill TX FIFO until there is no more room inside it
    while (uart2_txfifo_full_getf())
    {
        // Put a byte in the FIFO
        uart2_txdata_setf(*uart2_env.tx.bufptr);

        // Update TX parameters
        uart2_env.tx.size--;
        uart2_env.tx.bufptr++;

        if (uart2_env.tx.size == 0)
        {
            // Reset TX parameters
            uart2_env.tx.bufptr = NULL;

            // Disable TX interrupt
            uart2_thr_empty_setf(0);

            // Retrieve callback pointer
            callback = uart2_env.tx.callback;

            if(callback != NULL)
            {
                // Clear callback pointer
                uart2_env.tx.callback = NULL;

                // Call handler
                callback(UART_STATUS_OK);
            }
            else
            {
                ASSERT_ERR(0);
            }

            // Exit loop
            break;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Check if RX FIFO is empty.
 *
 * @return FIFO empty      false: not empty  / true: empty
 *****************************************************************************************
 */
static bool uart2_is_rx_fifo_empty(void)
{
    return (uart2_data_rdy_getf() == 0);
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void uart2_init(uint16_t baudr, uint8_t mode)
{

    //ENABLE FIFO, REGISTER FCR IF UART2_LCR_REG.DLAB=0
    // XMIT FIFO RESET, RCVR FIFO RESET, FIFO ENABLED
    SetBits16(UART2_LCR_REG, UART_DLAB, 0);
    SetWord16(UART2_IIR_FCR_REG,7);

    //DISABLE INTERRUPTS, REGISTER IER IF UART2_LCR_REG.DLAB=0
    SetWord16(UART2_IER_DLH_REG, 0);

    // ACCESS DIVISORLATCH REGISTER FOR BAUDRATE, REGISTER UART_DLH/DLL_REG IF UART2_LCR_REG.DLAB=1
    SetBits16(UART2_LCR_REG, UART_DLAB, 1);
    SetWord16(UART2_IER_DLH_REG, (baudr >> 8) & 0xFF);
    SetWord16(UART2_RBR_THR_DLL_REG, baudr & 0xFF);

    // NO PARITY, 1 STOP BIT, 8 DATA LENGTH AND
    SetWord16(UART2_LCR_REG,mode);

    //ENABLE TX INTERRUPTS, REGISTER IER IF UART2_LCR_REG.DLAB=0
    SetBits16(UART2_LCR_REG, UART_DLAB, 0);

    NVIC_DisableIRQ(UART2_IRQn);
    NVIC_SetPriority(UART2_IRQn, 3);
    NVIC_EnableIRQ(UART2_IRQn);
    NVIC_ClearPendingIRQ(UART2_IRQn);

    // Configure UART environment
    uart2_env.errordetect = UART_ERROR_DETECT_DISABLED;
    uart2_env.rx.bufptr = NULL;
    uart2_env.rx.size = 0;
    uart2_env.tx.bufptr = NULL;
    uart2_env.tx.size = 0;
}

void uart2_flow_on(void)
{
    // Configure modem (HW flow control enable)
    SetWord32(UART2_MCR_REG, UART_AFCE|UART_RTS);
}

bool uart2_flow_off(void)
{
    bool flow_off = true;
    volatile unsigned int i;

    do
    {
        // First check if no transmission is ongoing
        if ((uart2_temt_getf() == 0) || (uart2_thre_getf() == 0))
        {
            flow_off = false;
            break;
        }

        // Configure modem (HW flow control disable, 'RTS flow off')
        SetWord32(UART2_MCR_REG, 0);

        // Wait for 1 character duration to ensure host has not started a transmission at the
        // same time
        for (i=0;i<350;i++);

        // Check if data has been received during wait time
        if(!uart2_is_rx_fifo_empty())
        {
            // Re-enable UART flow
            uart2_flow_on();

            // We failed stopping the flow
            flow_off = false;
        }
    } while(false);

    return (flow_off);
}

void uart2_finish_transfers(void)
{
    // Configure modem (HW flow control disable, 'RTS flow off')
    uart2_mcr_pack(UART_ENABLE,     // extfunc
                  UART_DISABLE,    // autorts
                  UART_ENABLE,     // autocts
                  UART_DISABLE);   // rts

    // Wait TX FIFO empty
    while(!uart2_thre_getf() || !uart2_temt_getf());
}

void uart2_read(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t))
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(uart2_env.rx.bufptr == NULL);

    // Prepare RX parameters
    uart2_env.rx.size = size;
    uart2_env.rx.bufptr = bufptr;
    uart2_env.rx.callback = callback;

    // Start data transaction
    uart2_rec_data_avail_setf(1);
}

void uart2_write(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t))
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(uart2_env.tx.bufptr == NULL);

    // Prepare TX parameters
    uart2_env.tx.size = size;
    uart2_env.tx.bufptr = bufptr;
    uart2_env.tx.callback = callback;


    /* start data transaction
     * first isr execution is done without interrupt generation to reduce
     * interrupt load
     */
    uart2_thr_empty_isr();
    if (uart2_env.tx.bufptr != NULL)
    {
        uart2_thr_empty_setf(1);
    }
}

void UART2_Handler(void)
{
    uint32_t idd;
    idd = 0x0F & GetWord32(UART2_IIR_FCR_REG);
    if(idd!=NO_INT_PEND)
    {
        switch(idd)
        {
          case UART_TIMEOUT:
            if ((uart2_env.errordetect == UART_ERROR_DETECT_ENABLED) && uart2_fifo_err_getf())
            {
                uart2_rec_error_isr();
            }
            break;
          case RECEIVED_AVAILABLE:
            uart2_rec_data_avail_isr();
            break;

          case THR_EMPTY:
            uart2_thr_empty_isr();
            break;

          default:
            break;
        }
    }
}
/// @} UART
