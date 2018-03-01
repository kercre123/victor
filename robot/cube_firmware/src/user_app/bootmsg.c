#include <string.h>
#include <stdarg.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "../app/board.h"
//#include "reg_uart.h"   // uart register
#include "uart.h"       // uart definitions

//------------------------------------------------  
//    Uart
//------------------------------------------------

static inline void hal_uart_init(void)
{
  //connect pins
  GPIO_INIT_PIN(UTX, OUTPUT, PID_UART1_TX, 0, GPIO_POWER_RAIL_3V );
  //GPIO_INIT_PIN(URX, INPUT,  PID_UART1_RX, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  
  //enable UART1 peripheral clock
  SetBits16(CLK_PER_REG, UART1_ENABLE, 1);
  
  SetBits16(UART_LCR_REG, UART_DLAB, 0); //DLAB=0
  SetWord16(UART_IIR_FCR_REG, 7); //reset TX/RX FIFOs, FIFO ENABLED
  SetWord16(UART_IER_DLH_REG, 0); //disable all interrupts
  NVIC_DisableIRQ(UART_IRQn);     //"
  
  //Configure 'Divisor Latch' (baud rate). Note: DLL/DHL register accesses require LCR.DLAB=1
  const uint16_t baudr = UART_BAUDRATE_57K6;
  SetBits16(UART_LCR_REG, UART_DLAB, 1); //DLAB=1
  SetWord16(UART_IER_DLH_REG, (baudr >> 8) & 0xFF);
  SetWord16(UART_RBR_THR_DLL_REG, baudr & 0xFF);
  SetWord16(UART_LCR_REG, UART_CHARFORMAT_8); //DLAB=0, stop bits=1 parity=disabled. datalen=8bit
}

static inline void hal_uart_deinit(void)
{
  GPIO_INIT_PIN(UTX, INPUT_PULLUP, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  SetBits16(CLK_PER_REG, UART1_ENABLE, 0);
}

static inline void hal_uart_tx_flush(void)
{
  //wait for TX FIFO to empty (flush the cache)
  while( !(GetWord16(UART_LSR_REG) & UART_THRE )); //THRE: "this bit indicates that the THR or TX FIFO is empty."
  //while( !(GetWord16(UART_USR_REG) & UART_TFE  )); //alternate: fifo-specific status bits
  
  //wait for TX to fully complete; ready for power-down etc
  while( GetWord16(UART_LSR_REG) & UART_TEMT ); //TEMT: "this bit is set whenever the Transmitter Shift Register and the FIFO are both empty."
}

static inline void hal_uart_putchar(int c)
{
  //wait for space in the TX fifo (cached write)
  while( !(GetWord16(UART_USR_REG) & UART_TFNF) );
  
  //write data to the FIFO; starts tx if idle
  SetWord16(UART_RBR_THR_DLL_REG, c & 0xFF);
  
  //hal_uart_tx_flush();
  //return c;
}

void hal_uart_write(const char *s)
{
  int c;
  while( (c = *s++) > 0 )
    hal_uart_putchar(c);
}

//void UART_Handler_func(void) {}

void print8(uint8_t val)
{
  for(int x=0; x<2; x++) {
    int nibble = (val & 0xf0) >> 4;
    char ascii = nibble >= 10 ? nibble - 10 + 0x61 : nibble + 0x30; //map to ascii char
    hal_uart_putchar( ascii );
    val <<= 4;
  }
}

void print32(uint32_t val)
{
  for(int x=0; x<4; x++) {
    print8( val >> 24 );
    val <<= 8;
  }
}

//------------------------------------------------  
//    OTP
//------------------------------------------------

#include "datasheet.h"
static inline void otp_read(uint32_t addr, int len, uint8_t *buf)
{
  //otpc_clock_enable();
  SetBits16(CLK_AMBA_REG, OTP_ENABLE, 1);     // set divider to 1	
  while ((GetWord16(ANA_STATUS_REG) & LDO_OTP_OK) != LDO_OTP_OK)
      /* Just wait */;
  
  // Before every read action, put the OTP back in MREAD mode.
  // In this way, we can handle the OTP as a normal
  // memory mapped block.
  SetBits32 (OTPC_MODE_REG, OTPC_MODE_MODE, 1 /*OTPC_MODE_MREAD*/);

  memcpy(buf, (uint8_t*)addr, len);
  
  //otpc_clock_disable();
  SetBits16(CLK_AMBA_REG, OTP_ENABLE, 0);     // set divider to 1	
}

//------------------------------------------------  
//    Boot Message
//------------------------------------------------

#include "bdaddr.h"
#include "da14580_otp_header.h"

void bootmsg(void)
{
  bdaddr_t bdaddr;
  struct { uint32_t esn; uint32_t hwrev; uint32_t model; } info;
  
  //read factory programmed info from OTP header
  otp_read( OTP_ADDR_HEADER+OTP_HEADER_BDADDR_OFFSET, BDADDR_SIZE, bdaddr.addr );
  otp_read( OTP_ADDR_HEADER+offsetof(da14580_otp_header_t,custom_field), sizeof(info), (uint8_t*)&info );
  
  /*/DEBUG
  memcpy(bdaddr.addr, "\x01\x02\x03\x04\x05\x06", 6);
  info.esn = 0x12345678;
  info.hwrev = 0xabcdef12;
  info.model = 0x1a2b3c4d;
  //-*/
  
  hal_uart_init();
  hal_uart_write("\n\n\n\ncubeboot "); //XXX: pick a sync byte that 1) doesn't conflict with ascii parsers and 2) is distinct from dialog bootloader sync bytes
  
  //optimized bdaddr2str(): [hex] "##:##:##:##:##:##" LSB to MSB
  for(int i=0; i<BDADDR_SIZE; i++) {
    print8( bdaddr.addr[i] );
    if( i < BDADDR_SIZE-1 )
      hal_uart_putchar(':');
  }
  hal_uart_putchar(' ');
  
  //print factory info
  print32(info.esn);
  hal_uart_putchar(' ');
  
  print32(info.hwrev);
  hal_uart_putchar(' ');
  
  print32(info.model);
  hal_uart_write("\n\n\n");
  
  //cleanup
  hal_uart_tx_flush();
  
  //extra delay for final stop bit and receiver sync
  for (volatile int i = 70*4; i > 0; i--) __nop();  // About 128us??
  
  hal_uart_deinit();
}

