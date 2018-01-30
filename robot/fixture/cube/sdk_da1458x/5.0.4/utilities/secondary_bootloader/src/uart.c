/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief uart initialization & print functions
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
#include "global_io.h"
//#include "timer.h"
#include "gpio.h"
#include <core_cm0.h>
#include "user_periph_setup.h"
#include "uart.h"
#include "timer.h"

extern volatile WORD timeout_occur;
  
/*****************************************************************************************************/
/*                             U   A   R   T                                                         */
/*****************************************************************************************************/

/**
 ****************************************************************************************
 * @brief UART initialization 
 *
 ****************************************************************************************
 */
void uart_initialization(void)
{   
  SetBits16(CLK_PER_REG, UART1_ENABLE, 1);    // enable  clock for UART 1
  SetWord16(UART_LCR_REG, 0x80); // set bit to access DLH and DLL register 
  SetWord16(UART_RBR_THR_DLL_REG,UART_BAUDRATE&0xFF);//set low byte
   SetWord16(UART_LCR_REG,3);    // no parity, 1 stop bit 8 data length and clear bit 8
  SetBits16(UART_MCR_REG, UART_SIRE, 0);  // mode 0 for normal , 1 for IRDA
  SetWord16(UART_IIR_FCR_REG,1);  // enable fifo  
  SetBits16(UART_IER_DLH_REG,ERBFI_dlh0,0);  // IER access, disable interrupt for available data
  SetWord16(UART_RBR_THR_DLL_REG,UART_BAUDRATE);
  SetWord16(UART_LCR_REG,3);      
  SetWord16(UART_IIR_FCR_REG,UART_FIFO_EN|UART_RX_FIFO_RESET|UART_TX_FIFO_RESET);    // enable fifo and reset TX and RX fifo

  // initialize GPIOs
  GPIO_ConfigurePin( UART_GPIO_PORT, UART_TX_PIN, OUTPUT, PID_UART1_TX, false );
  GPIO_ConfigurePin( UART_GPIO_PORT, UART_RX_PIN, INPUT, PID_UART1_RX, false );    
}

/**
 ****************************************************************************************
 * @brief Disable UART and reset UART GPIOs
 *
 ****************************************************************************************
 */
void uart_release(void)
{
    SetBits16(CLK_PER_REG, UART1_ENABLE, 0); // disable clock for UART 1

    // reset GPIOs
    GPIO_ConfigurePin( UART_GPIO_PORT, UART_TX_PIN, INPUT, PID_GPIO, false );
    GPIO_ConfigurePin( UART_GPIO_PORT, UART_RX_PIN, INPUT, PID_GPIO, false );
}


/**
 ****************************************************************************************
 * @brief Receive byte from UART   
 *
 * @param[in] ch    Pointer to the variable that will store the received byte
 *
 * @return 1 on success, 0 on failure.
 ****************************************************************************************
 */ 
char uart_receive_byte(char *ch)
{
    start_timer(15);

    do {
    } while ( (GetWord16(UART_LSR_REG) & 0x01)==0 && (timeout_occur==0) );  // wait until serial data is ready 
    
    if (timeout_occur == 0)
    {
        stop_timer();
        *ch = 0xFF & GetWord16(UART_RBR_THR_DLL_REG);   // read from receive data buffer
        return 1;                                       // return no error
    }
    else                                                // timeout occur, no serial response
    {
        timeout_occur = 0;                              // clear timeout flag
        return 0;                                       // return error
    }
}

 /**
 ****************************************************************************************
 * @brief Sent byte from UART   
 * @param[in] ch:  character to print
 ****************************************************************************************
 */  
void uart_send_byte(char ch){
  do{
  }while((GetWord16(UART_LSR_REG)&0x20)==0);    // read status reg to check if THR is empty
  SetWord16(UART_RBR_THR_DLL_REG,(0xFF&ch));    // write to THR register
  do{                        // wait until start transmission 
  }while((GetWord16(UART_LSR_REG)&0x40)==0);    // read status reg to check if THR is empty
  return;
} 
