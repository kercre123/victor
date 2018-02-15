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
#include "hcic_eif.h"   // hci uart definition
#include "global_io.h"

#include "rwble_config.h"
/*
 * DEFINES
 *****************************************************************************************
 */

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */
/// INT_PRIO values
#if 0
enum UART_INTPRIO
{
    UART_INTPRIO_MODEMSTAT     = 0,
    UART_INTPRIO_ALLDATASENT   = 1,
    UART_INTPRIO_DATAAVAILABLE = 2,
    UART_INTPRIO_LINESTAT      = 3,
    UART_INTPRIO_TIMEOUT       = 6
};
#endif

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
struct uart_txrxchannel
{
    /// size
    uint32_t  size;
    /// buffer pointer
    uint8_t  *bufptr;
    /// call back function pointer
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

extern struct uart_env_tag uart_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

//baudr=9-> 115k2
//mode=3->  // no parity, 1 stop bit 8 data length
void uart_init_func(uint8_t baudr, uint8_t mode )
{

    //ENABLE FIFO, REGISTER FCR IF UART_LCR_REG.DLAB=0
    // XMIT FIFO RESET, RCVR FIFO RESET, FIFO ENABLED
    SetBits16(UART_LCR_REG, UART_DLAB, 0);
    SetWord16(UART_IIR_FCR_REG,7); 

    //DISABLE INTERRUPTS, REGISTER IER IF UART_LCR_REG.DLAB=0
    SetWord16(UART_IER_DLH_REG, 0);

    // ACCESS DIVISORLATCH REGISTER FOR BAUDRATE 115200, REGISTER UART_DLH/DLL_REG IF UART_LCR_REG.DLAB=1
    SetBits16(UART_LCR_REG, UART_DLAB, 1);
    SetWord16(UART_IER_DLH_REG,0); // for serial clk 16MHz baudrate 115200 
    //SetWord16(UART_RBR_THR_DLL_REG,9); // set baudrate ~115200  = serial_clk/(16*9)
    SetWord16(UART_IER_DLH_REG, (baudr&0xFF>>0x8));
    SetWord16(UART_RBR_THR_DLL_REG,baudr&0xFF); // set baudrate ~115200  = serial_clk/(16*9)

    // NO PARITY, 1 STOP BIT, 8 DATA LENGTH AND 
    //SetWord16(UART_LCR_REG,3); 
    SetWord16(UART_LCR_REG,mode);

    //ENABLE TX INTERRUPTS, REGISTER IER IF UART_LCR_REG.DLAB=0
    SetBits16(UART_LCR_REG, UART_DLAB, 0);
    //SetBits16(UART_IER_DLH_REG, ETBEI_dlh1, 1);

    NVIC_EnableIRQ(UART_IRQn); 
    NVIC->ICPR[UART_IRQn]=1; //clear eventual pending bit, but not necessary becasuse this is already cleared automatically in HW

    // Configure UART environment
    uart_env.errordetect = UART_ERROR_DETECT_DISABLED;
    uart_env.rx.bufptr = NULL;
    uart_env.rx.size = 0;
    uart_env.tx.bufptr = NULL;
    uart_env.tx.size = 0;

    //SetWord32(UART_MCR_REG, UART_AFCE|UART_RTS); //Enable RTS/CTS handshake
//uart_write("uart_is_initialised",19);
}

/// @} UART
