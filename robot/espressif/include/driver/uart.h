/*
 * File	: uart.h
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef UART_APP_H
#define UART_APP_H

#include "uart_register.h"
#include "eagle_soc.h"
#include "c_types.h"

#define RX_BUFF_SIZE    256
#define TX_BUFF_SIZE    100
#define UART0   0
#define UART1   1

typedef enum {
    FIVE_BITS = 0x0,
    SIX_BITS = 0x1,
    SEVEN_BITS = 0x2,
    EIGHT_BITS = 0x3
} UartBitsNum4Char;

typedef enum {
    ONE_STOP_BIT             = 0x1,
    ONE_HALF_STOP_BIT        = 0x2,
    TWO_STOP_BIT             = 0x3
} UartStopBitsNum;

typedef enum {
    NONE_BITS = 0x2,
    ODD_BITS   = 1,
    EVEN_BITS = 0
} UartParityMode;

typedef enum {
    STICK_PARITY_DIS   = 0,
    STICK_PARITY_EN    = 1
} UartExistParity;

typedef enum {
    UART_None_Inverse = 0x0,
    UART_Rxd_Inverse = UART_RXD_INV,
    UART_CTS_Inverse = UART_CTS_INV,
    UART_Txd_Inverse = UART_TXD_INV,
    UART_RTS_Inverse = UART_RTS_INV,
} UART_LineLevelInverse;


typedef enum {
    BIT_RATE_300 = 300,
    BIT_RATE_600 = 600,
    BIT_RATE_1200 = 1200,
    BIT_RATE_2400 = 2400,
    BIT_RATE_4800 = 4800,
    BIT_RATE_9600   = 9600,
    BIT_RATE_19200  = 19200,
    BIT_RATE_38400  = 38400,
    BIT_RATE_57600  = 57600,
    BIT_RATE_74880  = 74880,
    BIT_RATE_115200 = 115200,
    BIT_RATE_230400 = 230400,
    BIT_RATE_460800 = 460800,
    BIT_RATE_921600 = 921600,
    BIT_RATE_1843200 = 1843200,
    BIT_RATE_3686400 = 3686400,
} UartBautRate;

typedef enum {
    NONE_CTRL,
    HARDWARE_CTRL,
    XON_XOFF_CTRL
} UartFlowCtrl;

typedef enum {
    USART_HardwareFlowControl_None = 0x0,
    USART_HardwareFlowControl_RTS = 0x1,
    USART_HardwareFlowControl_CTS = 0x2,
    USART_HardwareFlowControl_CTS_RTS = 0x3
} UART_HwFlowCtrl;

typedef enum {
    EMPTY,
    UNDER_WRITE,
    WRITE_OVER
} RcvMsgBuffState;

typedef struct {
    uint32     RcvBuffSize;
    uint8     *pRcvMsgBuff;
    uint8     *pWritePos;
    uint8     *pReadPos;
    uint8      TrigLvl; //JLU: may need to pad
    RcvMsgBuffState  BuffState;
} RcvMsgBuff;

typedef struct {
    uint32   TrxBuffSize;
    uint8   *pTrxBuff;
} TrxMsgBuff;

typedef enum {
    BAUD_RATE_DET,
    WAIT_SYNC_FRM,
    SRCH_MSG_HEAD,
    RCV_MSG_BODY,
    RCV_ESC_CHAR,
} RcvMsgState;

typedef struct {
    UartBautRate 	     baut_rate;
    UartBitsNum4Char  data_bits;
    UartExistParity      exist_parity;
    UartParityMode 	    parity;    // chip size in byte
    UartStopBitsNum   stop_bits;
    UartFlowCtrl         flow_ctrl;
    RcvMsgBuff          rcv_buff;
    TrxMsgBuff           trx_buff;
    RcvMsgState        rcv_state;
    int                      received;
    int                      buff_uart_no;  //indicate which uart use tx/rx buffer
} UartDevice;

void uart_init(UartBautRate uart0_br, UartBautRate uart1_br);
void uart0_sendStr(const char *str);


///////////////////////////////////////
#define UART_TX_BUF_SIZE 2048    //define the size of each tx buffer
#define UART_TX_BUF_SIZE_MIN 256 //define the min size to malloc a new buffer.
#define OS_LEFT_MEM (UART_TX_BUF_SIZE*8)//define the min size of heap resource remaining for the system
#define UART_FIFO_LEN  128  //define the tx fifo length

#define UART_TX_EMPTY_THRESH_VAL 0x10
#define UART_TX_BUFFER_SIZE 8192*1
#define URAT_TX_LOWER_SIZE 1024*6
#define URAT_TX_UPPER_SIZE  8000*1


#define UART_RX_BUFFER_SIZE 201

 //#define UART_TX_DEBUG
#define DBG  
#define DBG1 uart1_sendStr_no_wait
#define DBG2 os_printf

 struct UartBuffer{
    uint32     UartBuffSize;
    uint8     *pUartBuff;
    uint8     *pInPos;
    uint8     *pOutPos;
    STATUS  BuffState;
    uint16    Space;  //remanent space of the buffer
    uint8  TcpControl;
    struct  UartBuffer     *  nextBuff;
};

struct UartRxBuff{
    uint32     UartRxBuffSize;
    uint8     *pUartRxBuff;
    uint8     *pWritePos;
    uint8     *pReadPos;
    STATUS RxBuffState;
    uint32    Space;  //remanent space of the buffer
} ;

typedef enum {
    RUN = 0,
    BLOCK = 1,
} TCPState;

void ICACHE_FLASH_ATTR uart_test_rx();
STATUS uart_tx_one_char(uint8 uart, uint8 TxChar);
STATUS uart_tx_one_char_no_wait(uint8 uart, uint8 TxChar);
void ICACHE_FLASH_ATTR uart1_sendStr_no_wait(const char *str);
void ICACHE_FLASH_ATTR uart_rx_intr_enable(uint8 uart_no);
void ICACHE_FLASH_ATTR uart_rx_intr_disable(uint8 uart_no);

//==============================================
#define FUNC_UART0_CTS 4
#define UART_LINE_INV_MASK  (0x3f<<19)
void ICACHE_FLASH_ATTR UART_SetWordLength(uint8 uart_no, UartBitsNum4Char len);
void ICACHE_FLASH_ATTR UART_SetStopBits(uint8 uart_no, UartStopBitsNum bit_num);
void ICACHE_FLASH_ATTR UART_SetLineInverse(uint8 uart_no, UART_LineLevelInverse inverse_mask);
void ICACHE_FLASH_ATTR UART_SetParity(uint8 uart_no, UartParityMode Parity_mode);
void ICACHE_FLASH_ATTR UART_SetBaudrate(uint8 uart_no,uint32 baud_rate);
void ICACHE_FLASH_ATTR UART_SetFlowCtrl(uint8 uart_no,UART_HwFlowCtrl flow_ctrl,uint8 rx_thresh);
void ICACHE_FLASH_ATTR UART_WaitTxFifoEmpty(uint8 uart_no); //do not use if tx flow control enabled
void ICACHE_FLASH_ATTR UART_ResetFifo(uint8 uart_no);
void ICACHE_FLASH_ATTR UART_ClearIntrStatus(uint8 uart_no,uint32 clr_mask);
void ICACHE_FLASH_ATTR UART_SetIntrEna(uint8 uart_no,uint32 ena_mask);
void ICACHE_FLASH_ATTR UART_SetPrintPort(uint8 uart_no);
//==============================================




#endif

