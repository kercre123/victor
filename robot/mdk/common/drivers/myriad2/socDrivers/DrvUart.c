#include <mv_types.h>
#include <registersMyriad2.h>
#include "DrvCpr.h"

#include "DrvUartDefines.h"
#include "DrvUart.h"

#include <swcLeonUtils.h>
#include <DrvRegUtils.h>
#include "stdio.h"

#define UART_RX_FIFO_SIZE (64)
#define UART_TX_FIFO_SIZE (64)

/* ***********************************************************************//**
   *************************************************************************
UART basic init
@{
----------------------------------------------------------------------------
@}
@param
    divider - baud rate = serial clock / (16 * divider)
@return
@{
info:

@}
     
 ************************************************************************* */
 
int DrvUartInit(tDrvUartCfg * uartCfg, u32 sysSpeedHz)
{

    u32 divider;
    u32 lcr_register;
    u32 ret_val;
    
    ret_val = D_UART_INIT_FUNC_STATUS_OK;
   // 1. Write to Modem Control Register (MCR) to program SIR mode, auto flow, loopback, modem control outputs
   if (uartCfg->LoopBack == Yes)
   {
        SET_REG_WORD(UART_MCR_ADR, D_UART_MCR_LB); // set just loopback mode 
    }
   else     
        SET_REG_WORD(UART_MCR_ADR, 0x00);          // don't set anything
        
        
   // 2. Write 1 to LCR[7] (DLAB) bit
   SET_REG_WORD(UART_LCR_ADR, D_UART_LCR_DLAB); // set divisor acces latch bit 
   
   // 3.Write to DLL, DLH to set up divisor for required baud rate
   
   if ( sysSpeedHz == 0 )
       return D_UART_INIT_FUNC_STATUS_DIVIDER0;

   if ( uartCfg->BaudRate == 0 )
       return D_UART_INIT_FUNC_STATUS_BAUDRATE0;
       
   divider = sysSpeedHz / (uartCfg->BaudRate * 16);
   
   if (divider == 0)
   {
      divider = 1;    // Baud rate bigger than system clock / 16. If divider is 0 UART is not transmittig.
      ret_val = D_UART_INIT_FUNC_STATUS_INVALID_BAUD;
   }
   
   SET_REG_WORD(UART_DLH_ADR, (divider >> 8) & 0xFF );   
   SET_REG_WORD(UART_DLL_ADR, divider & 0xFF);   
   
   // 4. Write 0 to LCR[7] (DLAB) bit
   SET_REG_WORD(UART_LCR_ADR, 0); // set DLAB to 0
   
   
   // 5. Write to LCR to set up transfer characteristics such as data length, number of stop bits, parity bits, and so on   
   lcr_register = 0;
   
   switch (uartCfg->DataLength)
   {
     case (UartDataBits5): lcr_register |= D_UART_LCR_DLS5; 
                           break;
     case (UartDataBits6): lcr_register |= D_UART_LCR_DLS6; 
                           break;
     case (UartDataBits7): lcr_register |= D_UART_LCR_DLS7; 
                           break;
     case (UartDataBits8): 
     default:              lcr_register |= D_UART_LCR_DLS8; 
   }  

   switch (uartCfg->StopBits)
   {
     case (UartStopBit1): lcr_register |= D_UART_LCR_STOP1; 
                           break;
     case (UartStopBit2):
     default:              lcr_register |= D_UART_LCR_STOP2; 
   }  

   switch (uartCfg->Parity)
   {
     case (UartParityOdd): lcr_register |= D_UART_LCR_PEN; 
                           break;
     case (UartParityEven):lcr_register |= D_UART_LCR_EPS | D_UART_LCR_PEN; 
                           break;
     case (UartParityNone):     
     default:              
              break;  // or else it wont compile 
   }  
   
   SET_REG_WORD(UART_LCR_ADR, lcr_register );
   
   // 6. If FIFO_MODE != NONE (FIFO mode enabled), write to FCR to enable FIFOs and set Transmit FIFO threshold level
   SET_REG_WORD(UART_FCR_ADR, D_UART_FCR_RCVR_1CH  | // rx interrupt when a ch is available 
                              D_UART_FCR_TET_EMPTY | // tx interrupt when FIFO is empty 
                              D_UART_FCR_XFIFOR    | // XMIT FIFO Reset
                              D_UART_FCR_RFIFOR    | // RCVR FIFO Reset
                              D_UART_FCR_FIFOE       // enable FIFO
                              );    
    
   // 7. Write to IER to enable required interrupts
   SET_REG_WORD(UART_IER_ADR, D_UART_IER_PTIME | // Programmable THRE Interrupt Mode Enable
                              D_UART_IER_ETBEI | // enabe transmit fifo empty
                              D_UART_IER_ERBFI   // enable received data available 
                              ); 
                              
   return ret_val;
}


int DrvUartPutChar(int chr)
{
    u32 bytesInTxFifo;
    u32 bytesInRxFifo;

    // In all cases we must wait until there is space in the transmit fifo before we send a character
    while ((GET_REG_WORD_VAL(UART_USR_ADR) & D_UART_USR_TFNF) != D_UART_USR_TFNF);   

    // However if the UART is configured in loopback mode the situation gets more complicated
    // The flow control signals are not looped back from the TX FIFO to the RX FIFO
    // Effectively this means we have a total FIFO size of only 64 bytes as the only push back is from the head of the RX fifo.
    // If the RX fifo is full and an additional byte is sent from the TX side, that byte will be lost.
    // As such, the only way we can be sure that a byte won't be lost is to find out how many bytes are currently queued in TX and RX FIFOs and 
    // ensure that we don't exceed the size of the RX FIFO.
    if ((GET_REG_WORD_VAL(UART_MCR_ADR) & D_UART_MCR_LB) == D_UART_MCR_LB) // Detect Loopback Mode
    {
        do
        {
            bytesInTxFifo = GET_REG_WORD_VAL(UART_TFL_ADR);
            bytesInRxFifo = GET_REG_WORD_VAL(UART_RFL_ADR);
        } while ((bytesInTxFifo + bytesInRxFifo + 2) >= UART_RX_FIFO_SIZE);
        // Note: The 2 bytes give us some margin as it isn't clear from the docs if there are holding registers at the head of the FIFOs
    }
    
    // 8. Write characters to be transmitted to transmit FIFO by writing to THR
    SET_REG_WORD(UART_THR_ADR, chr);   
    
    // 9. Wait for Transmit Holding Register empty interrupt (IIR[3:0] = 4b0010)
    // instead of IIR we should check for D_UART_LSR_TEMT bit in LSR
    //while ((GET_REG_WORD_VAL(UART_LSR_ADR) & D_UART_LSR_TEMT) != D_UART_LSR_TEMT);   
    
    // 10. More  data to transmit? Jump to 8
    // 11. Clear THR empty interrupt by reading IIR register
}   


int DrvUartVcsPutChar( int chr ) {
  chr &= 0xFF;
  SET_REG_WORD(DEBUG_MSG_ADR, chr);
  return chr;
}

u8 DrvUartGetChar()
{
   // The following is the recommended logic, however we use the alternative below
   // 8. Wait for Received Data Available interrupt (IIR[3:0] = 4b0100)
   //while ((GET_REG_WORD_VAL(UART_IIR_ADR) & 0xF) != D_UART_IIR_IID_RX_DATA) ; 

   // Wait until the Recieve Fifo Not Empty bit is set in the UART Status Register
   while ((GET_REG_WORD_VAL(UART_USR_ADR) & D_UART_USR_RFNE) != D_UART_USR_RFNE) ; 

   return GET_REG_WORD_VAL(UART_RBR_ADR);
}



void DrvUartTxData(u8 *data, u32 size)
{
   u32 idx;
   idx = 0;
   
   while (idx < size)
   {
          // wait if the FIFO is full
          while ((GET_REG_WORD_VAL(UART_USR_ADR) & D_UART_USR_TFNF) == 0);
          
          // send data
          SET_REG_WORD(UART_THR_ADR, data[idx]); 
          
          // next data
          ++idx;
   }
}


// get the data in the fifo
void DrvUartRxData(u8 *data, int *size, u32 maxSize)
{
     unsigned int idx;
     
     idx = 0;
     
     // while there is activity
     do 
     {
           // while the fifo is not empty
           while ((GET_REG_WORD_VAL(UART_USR_ADR) & D_UART_USR_RFNE) == D_UART_USR_RFNE)
           {
                 // read the data in the FIFO
                 data[idx] = GET_REG_WORD_VAL(UART_RBR_ADR);  
                 // next data 
                 ++idx;
          
                 // break if read to much
                 if (idx >= maxSize)
                       break;
           }
           
           // but break if to much data
           if (idx >= maxSize)
                break;
           
     } while ((GET_REG_WORD_VAL(UART_USR_ADR) & D_UART_USR_BUSY) == D_UART_USR_BUSY);

           
     *size = idx;
     
}


