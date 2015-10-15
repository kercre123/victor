/*
 * File	: uart.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * Modified for Anki use by Daniel Casner May 2015
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
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "osapi.h"
#include "driver/uart_register.h"
#include "mem.h"
#include "os_type.h"
#include "user_interface.h"
#include "telnet.h"
#include "client.h"

// UartDev is defined and initialized in rom code.
extern UartDevice    UartDev;

#define min(a, b) (a < b ? a : b)

inline void uart_rx_intr_disable(uint8 uart_no)
{
  CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
}

inline void uart_rx_intr_enable(uint8 uart_no)
{
  SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
}

inline void uart_tx_intr_disable(uint8 uart_no)
{
  CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
}

inline void uart_tx_intr_enable(uint8 uart_no)
{
  SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
}


/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART0 used for data TX/RX, RX buffer size is 0x100, interrupt enabled
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
uart_config(uint8 uart_no)
{
    if (uart_no == UART1)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    }
    else
    {
        /* rcv_buff size if 0x100 */
        //ETS_UART_INTR_ATTACH(uart_rx_intr_handler,  &(UartDev.rcv_buff));
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }
    uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate));//SET BAUDRATE

    WRITE_PERI_REG(UART_CONF0(uart_no), UartDev.exist_parity   //SET BIT AND PARITY MODE
                                                                        | UartDev.parity
                                                                        | (UartDev.stop_bits << UART_STOP_BIT_NUM_S)
                                                                        | (UartDev.data_bits << UART_BIT_NUM_S));

    //clear rx and tx fifo,not ready
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);    //RESET FIFO
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

    if (uart_no == UART0){
        //set rx fifo trigger
        WRITE_PERI_REG(UART_CONF1(uart_no),
        ((50   & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
        ((0x10 & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S));
        //SET_PERI_REG_MASK( UART_CONF0(uart_no),UART_TX_FLOW_EN);  //add this sentense to add a tx flow control via MTCK( CTS )
        SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_TOUT_INT_ENA |UART_FRM_ERR_INT_ENA);
    }else{
        WRITE_PERI_REG(UART_CONF1(uart_no),((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));//TrigLvl default val == 1
    }
    //clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
    //enable rx_interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA);
}

/******************************************************************************
 * FunctionName : uart_tx_one_char
 * Description  : Internal used function
 *                Use uart1 interface to transfer one char
 * Parameters   : uint8 uart - which UART to use
 *              : uint8 TxChar - character to tx
 * Returns      : OK
*******************************************************************************/
STATUS uart_tx_one_char(uint8 uart, uint8 TxChar)
{
    while (true){ // Wait for FIFO available
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
            break;
        }
    }
    WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
    return OK;
}


/******************************************************************************
* FunctionName : uart_tx_one_char_no_wait
* Description  : uart tx a single char without waiting for fifo
* Parameters   : uint8 uart - uart port
*                uint8 TxChar - char to tx
* Returns      : OK if byte queued, BUSY if no space in FIFO
*******************************************************************************/
STATUS uart_tx_one_char_no_wait(uint8 uart, uint8 TxChar)
{
  uint8 fifo_cnt = (( READ_PERI_REG(UART_STATUS(uart))>>UART_TXFIFO_CNT_S)& UART_TXFIFO_CNT);
  if (fifo_cnt < 126) // If space available in buffer
  {
    WRITE_PERI_REG(UART_FIFO(uart) , TxChar); // Queue byte
    return OK;
  }
  else
  {
    return BUSY;
  }
}

/******************************************************************************
 * FunctionName : uart_tx_buffer
 * Description  : Transfer buffer out specified UART, waits for FIFO
 * Parameters   : uint8 uart - The uart to TX on
 *                uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart_tx_buffer(uint8 uart, uint8 *buf, uint16 len)
{
    uint16 i;
    for (i = 0; i < len; i++)
    {
        uart_tx_one_char(uart, buf[i]);
    }
}

/******************************************************************************
* FunctionName : os_put_char
* Description  : Write char function for os_printf etc. Writes to UART1, aka the
*                debug port.
*******************************************************************************/
void os_put_char(uint8 c)
{
  uart_tx_one_char_no_wait(UART0, c);
  //uart_tx_one_char(UART0, c);
}

void os_put_hex(unsigned int i, unsigned short nibbles)
{
  static const char HEXTABLE[16] = "0123456789abcdef";
  unsigned short n = 1;
  while (n <= nibbles)
  {
    os_put_char(HEXTABLE[(i>>((nibbles-n)*4))&0xf]);
    n += 1;
  }
}

/******************************************************************************
* FunctionName : uart_init
* Description  : user interface for init uart
* Parameters   : UartBautRate uart0_br - uart0 bautrate
*                UartBautRate uart1_br - uart1 bautrate
* Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart_init(UartBautRate uart0_br, UartBautRate uart1_br)
{
  UartDev.baut_rate = uart0_br;
  uart_config(UART0);
  UartDev.baut_rate = uart1_br;
  uart_config(UART1);

  os_install_putc1(os_put_char);
}

void ICACHE_FLASH_ATTR uart_start()
{
  ETS_UART_INTR_ENABLE(); // What does this do? If we don't call it, it nothing works
  uart_rx_intr_enable(UART0);
}

void ICACHE_FLASH_ATTR
UART_SetWordLength(uint8 uart_no, UartBitsNum4Char len)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no),UART_BIT_NUM,len,UART_BIT_NUM_S);
}

void ICACHE_FLASH_ATTR
UART_SetStopBits(uint8 uart_no, UartStopBitsNum bit_num)
{
    SET_PERI_REG_BITS(UART_CONF0(uart_no),UART_STOP_BIT_NUM,bit_num,UART_STOP_BIT_NUM_S);
}

void ICACHE_FLASH_ATTR
UART_SetLineInverse(uint8 uart_no, UART_LineLevelInverse inverse_mask)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_LINE_INV_MASK);
    SET_PERI_REG_MASK(UART_CONF0(uart_no), inverse_mask);
}

void ICACHE_FLASH_ATTR
UART_SetParity(uint8 uart_no, UartParityMode Parity_mode)
{
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_PARITY |UART_PARITY_EN);
    if(Parity_mode==NONE_BITS){
    }else{
        SET_PERI_REG_MASK(UART_CONF0(uart_no), Parity_mode|UART_PARITY_EN);
    }
}

void ICACHE_FLASH_ATTR
UART_SetBaudrate(uint8 uart_no,uint32 baud_rate)
{
    uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}

void ICACHE_FLASH_ATTR
UART_SetFlowCtrl(uint8 uart_no,UART_HwFlowCtrl flow_ctrl,uint8 rx_thresh)
{
    if(flow_ctrl&USART_HardwareFlowControl_RTS){
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
        SET_PERI_REG_BITS(UART_CONF1(uart_no),UART_RX_FLOW_THRHD,rx_thresh,UART_RX_FLOW_THRHD_S);
        SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }else{
        CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }
    if(flow_ctrl&USART_HardwareFlowControl_CTS){
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
        SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    }else{
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
    }
}

void ICACHE_FLASH_ATTR
UART_WaitTxFifoEmpty(uint8 uart_no , uint32 time_out_us) //do not use if tx flow control enabled
{
    uint32 t_s = system_get_time();
    while (READ_PERI_REG(UART_STATUS(uart_no)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S)){
        if(( system_get_time() - t_s )> time_out_us){
            break;
        }
    }
}

void ICACHE_FLASH_ATTR
UART_ResetFifo(uint8 uart_no)
{
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}

void ICACHE_FLASH_ATTR
UART_ClearIntrStatus(uint8 uart_no,uint32 clr_mask)
{
    WRITE_PERI_REG(UART_INT_CLR(uart_no), clr_mask);
}

void ICACHE_FLASH_ATTR
UART_SetIntrEna(uint8 uart_no,uint32 ena_mask)
{
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), ena_mask);
}
