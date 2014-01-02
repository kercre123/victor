///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     UART Peripheral Driver
/// 

#ifndef DRV_UART_H
#define DRV_UART_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <DrvRegUtils.h>

// 2:  Definitions
// ----------------------------------------------------------------------------
// TODO: This should use the register header
#define CPR_DETECT_HW_SIM_REGISTER (0x80030000 + 0x0048)
#define IS_PLATFORM_VCS     ((( GET_REG_WORD_VAL(CPR_DETECT_HW_SIM_REGISTER) >> 13) & 0x7)==6)

//  TODO: Cleanup this implementation
//  Note: isaac_registers.h has wrong value.                                             
//int DrvVcsPutchar( int );
#define SYSTEM_PUTCHAR_FUNCTION ( IS_PLATFORM_VCS ? DrvApbUartVcsPutChar : DrvApbUartPutChar)

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

                       
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

//!@{
/* ***********************************************************************//**
   *************************************************************************
UART put char



@param[in] 
     chr - character for sending
@return

info:
    can be used for continous writing
   
 ************************************************************************* */
int  DrvApbUartPutChar( int chr);
int  DrvApbUartVcsPutChar( int chr);
//!@}


/* ***********************************************************************//**
   *************************************************************************
UART basic init



@param
    fifoEn     - enables/disables fifo
@param    
    oeTxEn     - enables output enable and tx
@param    
    rxEn       - enable/disable rx
@param    
    loop       - enable loop back
@param    
    flowEn     - enable/disable flow control
@return

info:
    - overrides the register

     
 ************************************************************************* */
void DrvApbUartInit(u32 fifoEn, u32 oeTxEn, u32 rxEn, u32 loop, u32 flowEn );

/* ***********************************************************************//**
   *************************************************************************
UART flow



@param
    oddEven     - 0 even parity
                -  1 odd parity
                .
@return

info:
     also sets the parity enable  
     does not override the other fields of the register
     
 ************************************************************************* */
void DrvApbUartParity(u32  oddEven);

/* ***********************************************************************//**
   *************************************************************************
UART interupts enable



@param
     RF  - enable/disable receiver FIFO level interrupts
@param     
     TF  - enable/disable transmitter FIFO level interrupts
@param     
     RI  - enable/disable generation of interrupt on receive
@param     
     TI  - enable/disable generation of interrupt on transmitt
@return

info:
     does not override the other fields of the register
     
 ************************************************************************* */
void DrvApbUartEnInt(u32 RF, u32 TF, u32 TI, u32 RI);

/* ***********************************************************************//**
   *************************************************************************
UART test init



@param
     baud        - desired UART baud rate 
@param     
     baseClock   - clock of the UART from cpr
@param     
     enableExtClk   - enable external clock
@return

info:
    - use Hz for both parameters
    - UART_SCALER_ADR must have a value, so that the resulting freq should be
       8 times higher than the desired boud rate
    .
       
 ************************************************************************* */
void DrvApbUartBaud(u32 baud, u32 baseClock,u32 enableExtClk);

/* ***********************************************************************//**
   *************************************************************************
UART test init



@param
     parity      - sets parity odd or even 
@param     
     baud        - desired UART baud rate 
@param     
     baseClk    - clock of the UART from cpr
@return

info:

 ************************************************************************* */
void DrvApbUartTestInit(u32 parity, u32 baud, u32 baseClk);

/* ***********************************************************************//**
   *************************************************************************
UART get char



@return
    a character received from UART

info:
    can be used for continous reading
    
 ************************************************************************* */
u8   DrvApbUartGetChar();

/* ***********************************************************************//**
   *************************************************************************
UART get char



@return
   0 if no character was read
   1 if a character was read
@param
   outData - a character received from UART , that is already in the fifo

info:
    doesn't wait for a acharacter it just checks if there is one available
    
 ************************************************************************* */
u8   DrvApbUartGetCharNoWait(u32 *outData);

/* ***********************************************************************//**
   *************************************************************************
UART put string



@param
      *str - a string of caracters, null terminated
@return

info:
     used to send a string to output
    
 ************************************************************************* */
void DrvApbUartPutStr(char *str);

/* ***********************************************************************//**
   *************************************************************************
UART flush 

@return

info:
     Called to ensure the application waits until any pending messages in the UART buffer
	 have been fully transmitted. Useful in situations where you would like to complete sending a 
	 message before reseting the system.
    
 ************************************************************************* */
void DrvApbUartFlush(void);

#endif // DRV_UART_H  
