#include <string.h>
#include <stdarg.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "../app/board.h"
//#include "reg_uart.h"   // uart register
#include "uart.h"       // uart definitions

typedef struct {
  GPIO_PORT port;
  GPIO_PIN pin;
} LED_Pin;

static const LED_Pin LEDs[] = {
  { GPIOPP(D0) },
  { GPIOPP(D3) },
  { GPIOPP(D6) },
  { GPIOPP(D9) },

  { GPIOPP(D1) },
  { GPIOPP(D4) },
  { GPIOPP(D7) },
  { GPIOPP(D10) },

  { GPIOPP(D2) },
  { GPIOPP(D5) },
  { GPIOPP(D8) },
  { GPIOPP(D11) },
};

//------------------------------------------------  
//    Uart
//------------------------------------------------

static inline void hal_uart_init(void)
{
  //connect pins
  GPIO_INIT_PIN(UTX, OUTPUT, PID_UART1_TX, 1, GPIO_POWER_RAIL_3V );
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

static inline void delayus(uint32_t us) {
  //loop is ~4 instructions + n*nop() -> 16 (@16MHz ~= 1us)
  for ( ; us > 0; us--) {
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
  }
}

static inline void bootmsg(void)
{
  bdaddr_t bdaddr;
  uint32_t info[4];
  
  //read factory programmed info from OTP header
  otp_read( OTP_ADDR_HEADER+OTP_HEADER_BDADDR_OFFSET, BDADDR_SIZE, bdaddr.addr );
  otp_read( OTP_ADDR_HEADER+offsetof(da14580_otp_header_t,custom_field), sizeof(info), (uint8_t*)&info );
  
  /*/DEBUG
  #warning DEBUG
  memcpy(bdaddr.addr, "\x01\x02\x03\x04\x05\x06", 6);
  info[0] = 0x12345678;
  info[1] = 0xabcdef12;
  info[2] = 0x1a2b3c4d;
  info[3] = 0xa5b6c7d8;
  //-*/
  
  //AN-B-001 Booting from serial interfaces: 'start TX' char 0x02
  //cubeboot sync byte 0xb2, 1) doesn't conflict with ascii parsers and 2) is distinct from dialog bootloader sync byte
  hal_uart_init();
  delayus(250);
  hal_uart_write("\xB2\xB2\xB2\xB2");
  delayus(1000);
  hal_uart_write("cubeboot ");
  
  //optimized bdaddr2str(): [hex] "##:##:##:##:##:##" LSB to MSB
  for(int i=0; i<BDADDR_SIZE; i++) {
    print8( bdaddr.addr[i] );
    if( i < BDADDR_SIZE-1 )
      hal_uart_putchar(':');
  }
  
  //factory defined fields
  for(int x=0; x < sizeof(info)/4; x++) {
    hal_uart_putchar(' ');
    print32(info[x]); //info[0]==esn, info[1]==hwrev, info[2]==model, info[3]==hash/signature
  }
  hal_uart_write("\n\n");
  
  //cleanup
  hal_uart_tx_flush();
  delayus(1000); //extra delay for final stop bit and receiver sync
  hal_uart_deinit();
}

void FirstBoot(void) {
  // Power up peripherals' power domain
  SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
  while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

  SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

  GPIO_INIT_PIN(BOOST_EN, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_1V );
  GPIO_INIT_PIN(D0, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  //print boot info
  bootmsg();

  // Blink pattern
  for (int p = 0; p < 12; p++) {
    GPIO_SetInactive(LEDs[p].port, LEDs[p].pin);
    for (int i = 0; i < 200000; i++) __nop();
    GPIO_SetActive(LEDs[p].port, LEDs[p].pin);
  }
}
