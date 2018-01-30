#include <string.h>
#include <stdarg.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "board.h"
#include "hal_uart.h"

//#include "reg_uart.h"   // uart register
//#include "uart.h"       // uart definition
#define UART_BAUDRATE_115K2         9   // = 16000000 / (16 * 115200),  actual baud rate = 111111.111,  error = -3.549%
#define UART_BAUDRATE_57K6          17  // = 16000000 / (16 * 57600),   actual baud rate = 58823.529,   error = 2.124%
#define UART_BAUDRATE_38K4          26  // = 16000000 / (16 * 38400),   actual baud rate = 38461.538,   error = 0.16%
#define UART_BAUDRATE_28K8          35  // = 16000000 / (16 * 28800),   actual baud rate = 28571.429,   error = -0.794%
#define UART_BAUDRATE_19K2          52  // = 16000000 / (16 * 19200),   actual baud rate = 19230.769,   error = 0.16%
#define UART_BAUDRATE_9K6           104 // = 16000000 / (16 * 9600),    actual baud rate = 9615.385,    error = 0.16%
#define UART_BAUDRATE_2K4           417 // = 16000000 / (16 * 2400),    actual baud rate = 2398.082,    error = -0.08%
enum {
  UART_CHARFORMAT_5 = 0,
  UART_CHARFORMAT_6 = 1,
  UART_CHARFORMAT_7 = 2,
  UART_CHARFORMAT_8 = 3
};

//map integer baudrate to register values
static inline uint16_t baudr_( const uint32_t baudrate ) {
  switch( baudrate ) {
    case 115200: return UART_BAUDRATE_115K2; //break;
    case 57600:  return UART_BAUDRATE_57K6; //break;
    case 38400:  return UART_BAUDRATE_38K4; //break;
    case 28800:  return UART_BAUDRATE_28K8; //break;
    case 19200:  return UART_BAUDRATE_19K2; //break;
    case 9600:   return UART_BAUDRATE_9K6; //break;
  }
  return UART_BAUDRATE_57K6; //default
}

void hal_uart_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;

  //connect pins
  GPIO_INIT_PIN(UTX, OUTPUT, PID_UART1_TX, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(URX, INPUT,  PID_UART1_RX, 0, GPIO_POWER_RAIL_3V );
  
  //enable UART1 peripheral clock
  SetBits16(CLK_PER_REG, UART1_ENABLE, 1);
  
  SetBits16(UART_LCR_REG, UART_DLAB, 0); //DLAB=0
  SetWord16(UART_IIR_FCR_REG, 7); //reset TX/RX FIFOs, FIFO ENABLED
  SetWord16(UART_IER_DLH_REG, 0); //disable all interrupts
  NVIC_DisableIRQ(UART_IRQn);     //"
  
  //Configure 'Divisor Latch' (baud rate). Note: DLL/DHL register accesses require LCR.DLAB=1
  uint16_t baudr = baudr_(COMMS_BAUDRATE);
  SetBits16(UART_LCR_REG, UART_DLAB, 1); //DLAB=1
  SetWord16(UART_IER_DLH_REG, (baudr >> 8) & 0xFF);
  SetWord16(UART_RBR_THR_DLL_REG, baudr & 0xFF);
  SetWord16(UART_LCR_REG, UART_CHARFORMAT_8); //DLAB=0, stop bits=1 parity=disabled. datalen=8bit
  
  //Interrupts
  //SetBits16(UART_IER_DLH_REG, ETBEI_dlh1, 1); //transmit
  //SetBits16(UART_IER_DLH_REG, ERBFI_dlh0, 1); //receive
  //NVIC_SetPriority(UART_IRQn, 3);
  //NVIC_EnableIRQ(UART_IRQn);
  //NVIC_ClearPendingIRQ(UART_IRQn);
  
  hal_uart_rx_flush();
  init = 1;
}

//------------------------------------------------  
//    RX
//------------------------------------------------

void hal_uart_rx_flush(void)
{
  volatile int x;
  do {
    x = GetWord16(UART_RBR_THR_DLL_REG); //fifo pop
    x = GetWord16(UART_LSR_REG); //read stat bits to clear
  } while( (x & (UART_RFE | UART_B1 | UART_FE | UART_PE | UART_OE | UART_DR)) //check all rx flags
        || (GetWord16(UART_USR_REG) & UART_RFNE) ); //rx fifo not empty
}

int hal_uart_getchar(void)
{
  int status = GetWord16(UART_LSR_REG); //rx flags
  
  /*/status flags
  bool fifo_PFB_err = (status & UART_RFE) > 0; //parity||framing||break flag on >0 bytes in the fifo. cleared on LSR read if last offender is at head position
  bool break_char   = (status & UART_B1)  > 0; //fifo head char is break char. cleared on LSR read.
  bool framing_err  = (status & UART_FE)  > 0; //fifo head char had framing err. cleared on LSR read.
  bool parity_err   = (status & UART_PE)  > 0; //fifo head char had parity err. cleared on LSR read.
  bool overrun_err  = (status & UART_OE)  > 0; //rx while fifo full; new dat discarded. cleared on LSR read.
  bool data_ready   = (status & UART_DR)  > 0; //FIFO mode, set if any bytes remain in FIFO
  //-*/
  
  if( status & (UART_RFE | UART_B1 | UART_FE | UART_PE | UART_OE) ) //flush errors
    hal_uart_rx_flush();
  else if( status & UART_DR ) //GetWord16(UART_USR_REG) & UART_RFNE
    return GetWord16(UART_RBR_THR_DLL_REG); //fifo pop
  
  return -1;
}

//------------------------------------------------  
//    TX
//------------------------------------------------

void hal_uart_tx_flush(void)
{
  //wait for TX FIFO to empty (flush the cache)
  while( !(GetWord16(UART_LSR_REG) & UART_THRE )); //THRE: "this bit indicates that the THR or TX FIFO is empty."
  //while( !(GetWord16(UART_USR_REG) & UART_TFE  )); //alternate: fifo-specific status bits
  
  //wait for TX to fully complete; ready for power-down etc
  while( GetWord16(UART_LSR_REG) & UART_TEMT ); //TEMT: "this bit is set whenever the Transmitter Shift Register and the FIFO are both empty."
}

int hal_uart_putchar(int c)
{
  //wait for space in the TX fifo (cached write)
  while( !(GetWord16(UART_USR_REG) & UART_TFNF) );
  
  //write data to the FIFO; starts tx if idle
  SetWord16(UART_RBR_THR_DLL_REG, c & 0xFF);
  
  //hal_uart_tx_flush();
  return c;
}

void hal_uart_write(const char *s)
{
  int c;
  while( (c = *s++) > 0 )
    hal_uart_putchar(c);
}

int hal_uart_printf(const char * format, ... )
{
  char dest[128];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  
  hal_uart_write(dest);
  return len > sizeof(dest) ? sizeof(dest) : len;
}

//------------------------------------------------  
//    ISR
//------------------------------------------------

//Uart Interrupt ID (UART_IIR register <3:0>)
enum UART_ID {
  MODEM_STAT            = 0,
  NO_INT_PEND           = 1,
  THR_EMPTY             = 2,
  RECEIVED_AVAILABLE    = 4,
  RECEIVER_LINE_STATUS  = 6, //???
  BUSY_DETECT           = 7, //???
  CHAR_TIMEOUT          = 12
};

void UART_Handler_func(void)
{
  /*
  int txdata = 0;
  int idd = 0xf & GetWord16(UART_IIR_FCR_REG);
  if(idd != NO_INT_PEND)
  {
    switch(idd)
    {
      case CHAR_TIMEOUT:
        SetBits16(UART_IER_DLH_REG, ERBFI_dlh0, 0); // Disable RX interrupt
        //handler...
        break;
      case RECEIVED_AVAILABLE:
        while( uart_data_rdy_getf() ) {
          int dat = GetWord16(UART_RBR_THR_DLL_REG); //REG_PL_RD(UART_RHR_ADDR); //uart_rxdata_getf();
          //handler...
        }
        break;
      case THR_EMPTY:
        while( (uint8_t)GetBits16(UART_USR_REG,UART_TFNF) ) //while TX fifo not full
          SetWord16(UART_RBR_THR_DLL_REG, txdata); //REG_PL_WR(UART_THR_ADDR, (uint32_t)txdata << 0); //uart_txdata_setf(*uart_env.tx.bufptr);  // Put a byte in the FIFO
        break;
      default:
        break;
    }
  }
  //-*/
}
