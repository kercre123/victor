#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvCpr.h"
#include "DrvUart.h"
#include <swcLeonUtils.h>

#define TEST_FINISH_ADR      (LHB_ROM_UPR_ADR)
#define DEBUG_MSG_ADR        (LHB_ROM_UPR_ADR-4)
#define RES_ADR              (LHB_ROM_UPR_ADR-8)
#define EXP_DATA_ADR         (LHB_ROM_UPR_ADR-0xC)
#define ACT_DATA_ADR         (LHB_ROM_UPR_ADR-0x10)
#define RD_ADR               (LHB_ROM_UPR_ADR-0x14)
#define VCS_EVENT_REG        (LHB_ROM_UPR_ADR-0x18)
#define IRQ_TB_MESSAGE_ADR   (LHB_ROM_UPR_ADR-0x2C)
#define LEON_MEM_DUMP_START  0x90007f98 //(LHB_ROM_UPR_ADR-0x68)
#define LEON_MEM_DUMP_LEN    0x90007f9C //(LHB_ROM_UPR_ADR-0x64)
                              

int DrvApbUartVcsPutChar( int c ) {
  c &= 0xFF;
  SET_REG_WORD(DEBUG_MSG_ADR, c);
  return c;
}
            
/* ***********************************************************************//**
   *************************************************************************
UART basic init
@{
----------------------------------------------------------------------------
@}
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
@{
info:
    - overrides the register
@}
     
 ************************************************************************* */
void DrvApbUartInit(u32 fifoEn, u32 oeTxEn, u32 rxEn, u32 loop, u32 flowEn )
{

	   // Ensure that the UART is clocked (ABP Brige 1 is clocked by default)
//	   SET_REG_WORD(CPR_CLK_EN0_ADR, GET_REG_WORD_VAL(CPR_CLK_EN0_ADR) | D_CPR_CLK1_EN_UART );
       DrvCprSysDeviceAction(ENABLE_CLKS,DEV_UART);

	   
       SET_REG_WORD(UART_CTRL_ADR, ((fifoEn  & 1) << 31) |
                                   ((fifoEn  & 1) << 11) |   // if fifo is enabled, enable fifo read 
                                   ((oeTxEn & 1) << 12) |
                                   ((oeTxEn & 1) << 1 ) |
                                   ((rxEn    & 1)      ) |
                                   ((flowEn  & 1) << 6 ) |                                   
                                   ((loop     & 1) << 7 ));                                   
}

/* ***********************************************************************//**
   *************************************************************************
UART flow
@{
----------------------------------------------------------------------------
@}
@param
    oddEven     - 0 even parity
                -  1 odd parity
                .
@return
@{
info:
     also sets the parity enable  
     does not override the other fields of the register
@}     
 ************************************************************************* */
void DrvApbUartParity(u32  oddEven)
{
       SET_REG_WORD(UART_CTRL_ADR, GET_REG_WORD_VAL(UART_CTRL_ADR) |
                                             (1              << 5) |            // enable parity
                                             ((oddEven & 1) << 4));
}

/* ***********************************************************************//**
   *************************************************************************
UART interupts enable
@{
----------------------------------------------------------------------------
@}
@param
     RF  - enable/disable receiver FIFO level interrupts
@param     
     TF  - enable/disable transmitter FIFO level interrupts
@param     
     RI  - enable/disable generation of interrupt on receive
@param     
     TI  - enable/disable generation of interrupt on transmitt
@return
@{
info:
     does not override the other fields of the register
@}     
 ************************************************************************* */
void DrvApbUartEnInt(u32 RF, u32 TF, u32 TI, u32 RI)
{
       SET_REG_WORD(UART_CTRL_ADR, GET_REG_WORD_VAL(UART_CTRL_ADR) |
                     ((RF & 1) << 10) |
                     ((TF & 1) <<  9) |
                     ((TI & 1) <<  3) |
                     ((RI & 1) <<  2));
}

/* ***********************************************************************//**
   *************************************************************************
UART test init
@{
----------------------------------------------------------------------------
@}
@param
     baud        - desired UART baud rate 
@param     
     baseClock   - clock of the UART from cpr
@return
@{
info:
    - use Hz for both parameters
    - UART_SCALER_ADR must have a value, so that the resulting freq should be
       8 times higher than the desired boud rate
    .
@}       
 ************************************************************************* */
void DrvApbUartBaud(u32 baud, u32 baseClock,u32 enableExtClk)
{
    SET_REG_WORD(UART_SCALER_ADR, (baseClock<<3)/baud);
    SET_REG_WORD(UART_CTRL_ADR  , GET_REG_WORD_VAL(UART_CTRL_ADR) | ((enableExtClk & 1) << 8) );
}

/* ***********************************************************************//**
   *************************************************************************
UART test init
@{
----------------------------------------------------------------------------
@}
@param
     parity      - sets parity odd or even 
@param     
     baud        - desired UART baud rate 
@param     
     baseClk    - clock of the UART from cpr
@return
@{
info:
@}
 ************************************************************************* */
void DrvApbUartTestInit(u32 parity, u32 baud, u32 baseClk)
{
     parity = parity; // not supported for now
     DrvApbUartInit(1, 1, 1, 1, 1 );      // enable fifo, enable rx & tx, enable loop, flow ctrl
     //DrvApbUartParity(parity);                      // this could be used if desired
     DrvApbUartEnInt(1, 1, 1, 1);        // enable generation of all interrupts
     DrvApbUartBaud(baud, baseClk, 0);   // disable external clk
}


/* ***********************************************************************//**
   *************************************************************************
UART put char
@{
----------------------------------------------------------------------------
@}
@param: 
     chr - character for sending
@return
@{
info:
    can be used for continous writing
@}   
 ************************************************************************* */
int DrvApbUartPutChar( int chr )
{
    chr &= 0xFF;
    if (GET_REG_WORD_VAL(UART_CTRL_ADR) & 0x80000000)           // if fifo is enabled
    {
     while (GET_REG_WORD_VAL(UART_STATUS_ADR) & 0x200) NOP;     // wait while TX FIFO full
    } 
    else
    {
     while (GET_REG_WORD_VAL(UART_STATUS_ADR) & 0x002) NOP;     // wait while TX shift register is not empty 
    }
    
    SET_REG_WORD(UART_DATA_ADR, chr);
    return chr;
}


/* ***********************************************************************//**
   *************************************************************************
UART get char
@{
----------------------------------------------------------------------------
@}
@return
    a character received from UART
@{
info:
    can be used for continous reading
@}    
 ************************************************************************* */
u8 DrvApbUartGetChar()
{
    if (GET_REG_WORD_VAL(UART_CTRL_ADR) & 0x80000000) {                  // if fifo is enabled 
      while (!(GET_REG_WORD_VAL(UART_STATUS_ADR) & 0xFC000000))
        NOP;     // wait while RX FIFO is empty, rx count == 0
    } else {
      while (!(GET_REG_WORD_VAL(UART_STATUS_ADR) & 0x001))
        NOP;          // wait while RX shift register is empty
    }
    return GET_REG_WORD_VAL(UART_DATA_ADR) & 0xFF;
}

/* ***********************************************************************//**
   *************************************************************************
UART get char
@{
----------------------------------------------------------------------------
@}
@return
   0 if no character was read
   1 if a character was read
@param
   outData - a character received from UART , that is already in the fifo
@{
info:
    doesn't wait for a acharacter it just checks if there is one available
@}    
 ************************************************************************* */
u8 DrvApbUartGetCharNoWait(u32 *outData)
{
    if (GET_REG_WORD_VAL(UART_CTRL_ADR) & 0x80000000)                   // if fifo is enabled 
    {
         if (!(GET_REG_WORD_VAL(UART_STATUS_ADR) & 0xFC000000)) 
            return 0;
    } 
    else
    {
         if(!(GET_REG_WORD_VAL(UART_STATUS_ADR) & 0x001)) 
            return 0;
    }
    
    *outData = GET_REG_WORD_VAL(UART_DATA_ADR) & 0xFF;
    
    return 1;
}


/* ***********************************************************************//**
   *************************************************************************
UART put string
@{
----------------------------------------------------------------------------
@}
@param
      *str - a string of caracters, null terminated
@return
@{
info:
     used to send a string to output
@}    
 ************************************************************************* */
void DrvApbUartPutStr(char *str)
{
    while (*str != 0x00)
    {
       DrvApbUartPutChar(*str);
       ++str;
    }
       
}

/* ***********************************************************************//**
   *************************************************************************
UART flush 
@{
----------------------------------------------------------------------------
@}
@param
@return
@{
info:
     Called to ensure the application waits until any pending messages in the UART buffer
	 have been fully transmitted. Useful in situations where you would like to complete sending a 
	 message before reseting the system.
@}    
 ************************************************************************* */
void DrvApbUartFlush(void)
{
	unsigned int uartBufferEmpty;

	do
		{
		uartBufferEmpty = GET_REG_WORD_VAL(UART_STATUS_ADR) & (0x3 << 1);
		}
    while ( uartBufferEmpty != (0x3 <<1) );

	return;
}


