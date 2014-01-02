///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the Uart Module
/// 
/// UART core functions 
///
/// -DrvUartInit - function to init the UART module 
/// -DrvUartPutChar - send one character and wait for it to be cleared out of the Transmit Holding Register
/// -DrvUartGetChar - wait to receive one characther and return it
/// -DrvUartTxData - sends an array of data using the FIFO 
/// -DrvUartRxData - gets all the data in the FIFO, while the UART is busy, but doesn't excede a capacity


#ifndef DRV_UART_SYN_H
#define DRV_UART_SYN_H 

#include <mv_types.h>
#include <DrvRegUtils.h>
#include <registersMyriad.h>

/// Defines for the output of the init function 
#define D_UART_INIT_FUNC_STATUS_OK           (-1)
#define D_UART_INIT_FUNC_STATUS_DIVIDER0     (0)
#define D_UART_INIT_FUNC_STATUS_BAUDRATE0    (1)
#define D_UART_INIT_FUNC_STATUS_INVALID_BAUD (2)

#define SYSTEM_PUTCHAR_FUNCTION ( (IS_PLATFORM_VCS) ? DrvUartVcsPutChar : DrvUartPutChar)



typedef enum drvUartDataBits 
{
  UartDataBits5=0,
  UartDataBits6,
  UartDataBits7,
  UartDataBits8
} tDrvUartDataBits;

typedef enum drvUartStopBits
{
   UartStopBit1 = 0,
   UartStopBit2
} tDrvUartStopBits;

typedef enum drvUartParity
{
   UartParityNone = 0,
   UartParityEven,
   UartParityOdd,
} tDrvUartParity;

typedef enum Boolen {No = 0, Yes = 1} tBoolean;

/// the structure that contains the configuration of the UART module
typedef struct drvUartCfg
{
  tDrvUartDataBits  DataLength;
  tDrvUartStopBits  StopBits;
  tDrvUartParity    Parity;
  u32               BaudRate;
  tBoolean          LoopBack;
} tDrvUartCfg;


/// Configure the UART driver for RX and TX
/// @param[in] uartCfg  - pointer to a tDrvUartCfg structure, which contains the UART configuration properties
/// @param[in] sysSpeedHz -speed of the system in Hz 
/// @return  int - representing an error code or a success code 
int DrvUartInit(tDrvUartCfg * uartCfg, u32 sysSpeedHz);


/// Sends character by character, waits for the register or FIFO to empty 
/// @param[in]  chr - character to send 
/// @return  void
int DrvUartPutChar(int chr);

int DrvUartVcsPutChar( int chr );
/// waits for the RX interrupt and gets one character 
/// @return  the caracter read from the FIFO
u8 DrvUartGetChar();

/// Send an Array on UART using the UART FIFO
/// @param[in] data - pointer to an array to be sent
/// @param[in] size - size of the array in bytes 
/// @return void
void DrvUartTxData(u8 *data, u32 size);

/// Receive a number of data bytes 
/// @param[out] d ata - pointer to memory location where data can be stored 
/// @param[out]  size - pointer to a u32 where the size of the received data can be sotred 
/// @param[in]  maxSize - maximum number of bytes to be received 
/// @return void
void DrvUartRxData(u8 *data, int *size, u32 maxSize);

#endif //DRV_UART_SYN_H
