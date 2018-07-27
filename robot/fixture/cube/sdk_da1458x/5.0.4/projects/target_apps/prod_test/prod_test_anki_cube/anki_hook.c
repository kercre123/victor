/*
 * INCLUDES
 ****************************************************************************************
 */
#include "da1458x_scatter_config.h"
#include "arch.h"
#include "arch_api.h"
#include <stdlib.h>
#include <stddef.h>     // standard definitions
#include <stdbool.h>    // boolean definition
#include "boot.h"       // boot definition
#include "rwip.h"       // BLE initialization
#include "syscntl.h"    // System control initialization
#include "emi.h"        // EMI initialization
#include "intc.h"       // Interrupt initialization
#include "em_map_ble.h"
#include "ke_mem.h"
#include "ke_event.h"
#include "user_periph_setup.h"

#include "uart.h"   // UART initialization
#include "nvds.h"   // NVDS initialization
#include "rf.h"     // RF initialization
#include "app.h"    // application functions
#include "dbg.h"    // For dbg_warning function

#include "global_io.h"

#include "datasheet.h"

#include "em_map_ble_user.h"
#include "em_map_ble.h"

#include "lld_sleep.h"
#include "rwble.h"
#include "rf_580.h"
#include "gpio.h"

#include "lld_evt.h"
#include "arch_console.h"

#include "arch_system.h"

#include "arch_patch.h"

#include "arch_wdg.h"

#include "user_callback_config.h"

#include "board.h"

#include "app/accel.h"
#include "app/lights.h"

//------------------------------------------------  
//    Uart & Led
//------------------------------------------------

static inline void hal_uart_init(void)
{
  //connect pins
  GPIO_INIT_PIN(UTX, OUTPUT, PID_UART1_TX, 1, GPIO_POWER_RAIL_3V );
  //GPIO_INIT_PIN(URX, INPUT,  PID_UART1_RX, 0, GPIO_POWER_RAIL_3V );
  //GPIO_INIT_PIN(URX, INPUT_PULLUP, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  
  //enable UART1 peripheral clock
  SetBits16(CLK_PER_REG, UART1_ENABLE, 1);
  
  SetBits16(UART_LCR_REG, UART_DLAB, 0); //DLAB=0
  SetWord16(UART_IIR_FCR_REG, 7); //reset TX/RX FIFOs, FIFO ENABLED
  SetWord16(UART_IER_DLH_REG, 0); //disable all interrupts
  NVIC_DisableIRQ(UART_IRQn);     //"
  
  //Configure 'Divisor Latch' (baud rate). Note: DLL/DHL register accesses require LCR.DLAB=1
  const uint16_t baudr = UART_BAUDRATE_115K2; //UART_BAUDRATE_57K6;
  SetBits16(UART_LCR_REG, UART_DLAB, 1); //DLAB=1
  SetWord16(UART_IER_DLH_REG, (baudr >> 8) & 0xFF);
  SetWord16(UART_RBR_THR_DLL_REG, baudr & 0xFF);
  SetWord16(UART_LCR_REG, UART_CHARFORMAT_8); //DLAB=0, stop bits=1 parity=disabled. datalen=8bit
}
/*
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
*/
static int hal_uart_putchar(int c)
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

#define printU16_(v)  printU32_((v));
void printU32_(uint32_t val)
{
  bool printing = 0;
  uint32_t place = 1000000000;
  
  while( place )
  {
    int digit = val/place;
    val -= digit*place;
    
    if( digit > 0 || printing || place==1 ) {
      hal_uart_putchar( 0x30 + digit );
      printing = 1; //print all following digits
    }
    place /= 10;
  }
  
  hal_uart_putchar(' ');
}

static void led_set_static(const uint8_t* colors)
{
  //Init boost regulator control (NOTE: 1V-BAT logic level)
  GPIO_INIT_PIN(BOOST, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_1V );
  
  // Init LEDs
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
  
  for(int i=0; i<12; i++) {
    if( colors[i] > 0 ) {
      switch(i) {
        case HAL_LED_D1_RED: GPIO_CLR(D2); break;
        case HAL_LED_D1_GRN: GPIO_CLR(D1); break;
        case HAL_LED_D1_BLU: GPIO_CLR(D0); break;
        case HAL_LED_D2_RED: GPIO_CLR(D5); break;
        case HAL_LED_D2_GRN: GPIO_CLR(D4); break;
        case HAL_LED_D2_BLU: GPIO_CLR(D3); break;
        case HAL_LED_D3_RED: GPIO_CLR(D8); break;
        case HAL_LED_D3_GRN: GPIO_CLR(D7); break;
        case HAL_LED_D3_BLU: GPIO_CLR(D6); break;
        case HAL_LED_D4_RED: GPIO_CLR(D11); break;
        case HAL_LED_D4_GRN: GPIO_CLR(D10); break;
        case HAL_LED_D4_BLU: GPIO_CLR(D9); break;
      }
    }
  }
}

/****************************************************************************************
* @brief ANKI Hacky hack time
****************************************************************************************/

#define chan2freq(cn)   ( 2402 + 2*(cn) )
#define freq2chan(f)    ( ((f)-2402) / 2 ) //valid >= 2402
const uint8_t ch2402 = freq2chan(2402);
const uint8_t ch2440 = freq2chan(2440);
const uint8_t ch2442 = freq2chan(2442);
const uint8_t ch2480 = freq2chan(2480);

enum test_pattern_e {
  TEST_PATTERN_TX_PACKET        = -4,
  TEST_PATTERN_RX_PACKET        = -3,
  TEST_PATTERN_RX_UNMODULATED   = -2,
  TEST_PATTERN_TX_CARRIER       = -1,
  TEST_PATTERN_PSEUDO_RANDOM_9  = 0x00, //傍est pattern is Pseudo random 9
  TEST_PATTERN_11110000         = 0x01, //傍est pattern is 11110000
  TEST_PATTERN_10101010         = 0x02, //傍est pattern is 10101010
  TEST_PATTERN_PSEUDO_RANDOM_15 = 0x03, //傍est pattern is Pseudo random 15
  TEST_PATTERN_11111111         = 0x04, //傍est pattern is 11111111
  TEST_PATTERN_00000000         = 0x05, //傍est pattern is 00000000
  TEST_PATTERN_00001111         = 0x06, //傍est pattern is 00001111
  TEST_PATTERN_01010101         = 0x07, //傍est pattern is 01010101
};

/*
Cube test modes and LED patterns

LED 1 Red 100% -> TX-data 2402 MHz
LED 1 Grn 100% -> TX-data 2442 MHz
LED 1 Blu 100% -> TX-data 2480 MHz

LED 2 Red  25% -> TX-tone 2402 MHz
LED 2 Grn  25% -> TX-tone 2442 MHz
LED 2 Blu  25% -> TX-tone 2480 MHz

LED 3 Red  25% -> TX-pkt  2402 MHz
LED 3 Grn  25% -> TX-pkt  2440 MHz
LED 3 Blu  25% -> TX-pkt  2480 MHz

LED 4 Red  25% -> RX-pkt  2402 MHz
LED 4 Grn  25% -> RX-pkt  2440 MHz
LED 4 Blu  25% -> RX-pkt  2480 MHz

RX should receive continuously.
TX should transmit continuously.
"Data" is all 0x55 bytes.
"Tone" is a carrier/continuous wave.

LED brightness is also part of the certification.
For TX-data, just set the LED GPIOs and walk away.
The TX-tone, LED modulation is required, but can be at any frequency >50Hz (such as loop iterations).
*/

#define OFF   0x00,0x00,0x00
#define RED   0xFF,0x00,0x00
#define GRN   0x00,0xFF,0x00
#define BLU   0x00,0x00,0xFF

typedef struct {
  uint8_t   channel;
  int8_t    pattern;
  uint8_t   led_static;
  uint8_t   led[12];
} test_cfg_t;

static const test_cfg_t test_modes[] = {
  //RX-pkt @ freq
  { ch2402, TEST_PATTERN_RX_PACKET  , 0,{RED,RED,RED,RED} }, //4 Red   LED @ 1/12 duty
  { ch2440, TEST_PATTERN_RX_PACKET  , 0,{GRN,GRN,GRN,GRN} }, //4 Green LED @ 1/12 duty
  { ch2480, TEST_PATTERN_RX_PACKET  , 0,{BLU,BLU,BLU,BLU} }, //4 Blue  LED @ 1/12 duty
  //TX-data @ freq
  { ch2402, TEST_PATTERN_01010101   , 1,{RED,OFF,OFF,OFF} }, //1 Red   LED @ 100% duty
  { ch2442, TEST_PATTERN_01010101   , 1,{GRN,OFF,OFF,OFF} }, //1 Green LED @ 100% duty
  { ch2480, TEST_PATTERN_01010101   , 1,{BLU,OFF,OFF,OFF} }, //1 Blue  LED @ 100% duty
  //TX-tone @ freq
  { ch2402, TEST_PATTERN_TX_CARRIER , 0,{RED,OFF,RED,OFF} }, //2 Red   LED @ 1/12 duty
  { ch2442, TEST_PATTERN_TX_CARRIER , 0,{GRN,OFF,GRN,OFF} }, //2 Green LED @ 1/12 duty
  { ch2480, TEST_PATTERN_TX_CARRIER , 0,{BLU,OFF,BLU,OFF} }, //2 Blue  LED @ 1/12 duty
  //TX-pkt @ freq
  { ch2402, TEST_PATTERN_TX_PACKET  , 0,{RED,RED,RED,OFF} }, //3 Red   LED @ 1/12 duty
  { ch2440, TEST_PATTERN_TX_PACKET  , 0,{GRN,GRN,GRN,OFF} }, //3 Green LED @ 1/12 duty
  { ch2480, TEST_PATTERN_TX_PACKET  , 0,{BLU,BLU,BLU,OFF} }  //3 Blue  LED @ 1/12 duty
  //RX-unmod @ freq
  //{ ch2402, TEST_PATTERN_RX_UNMODULATED , {RED,RED,RED,OFF}, 0 }, //3 Red   LED @ 1/12 duty
  //{ ch2442, TEST_PATTERN_RX_UNMODULATED , {GRN,GRN,GRN,OFF}, 0 }, //3 Green LED @ 1/12 duty
  //{ ch2480, TEST_PATTERN_RX_UNMODULATED , {BLU,BLU,BLU,OFF}, 0 }  //3 Blue  LED @ 1/12 duty
};

static const int TOTAL_TEST_MODES = sizeof(test_modes)/sizeof(test_cfg_t);
static const uint8_t led_off[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
static const int COUNTDOWN_TIME   = 200 * 10; // 10 seconds to trigger
static int selected = TOTAL_TEST_MODES-1;
static const test_cfg_t *cfg = &test_modes[TOTAL_TEST_MODES-1];

void anki_start_hci_test(void)
{
  int8_t pattern = cfg->pattern;
  uint8_t channel = cfg->channel;
  
  if( pattern == TEST_PATTERN_RX_PACKET ) {
    uint8_t buf[] = { channel };
    hci_start_prod_rx_test( (uint8_t*)buf );
  }
  //else if( pattern == TEST_PATTERN_RX_UNMODULATED ) {
  //  uint8_t buf[] = { 'R', channel };
  //  hci_unmodulated_cmd( (uint8_t*)buf );
  //}
  else if( pattern == TEST_PATTERN_TX_CARRIER ) {
    uint8_t buf[] = { 'T', channel };
    hci_unmodulated_cmd( (uint8_t*)buf );
  }
  else if( pattern >= 0 ) { //TX modulated
    uint8_t buf[] = { channel, pattern };
    hci_tx_start_continue_test_mode( (uint8_t*)buf );
  }
  //else if( pattern == TEST_PATTERN_TX_PACKET ) //do nothing. handled by manage()
}

static void anki_hci_manage(void)
{
  static int tickCnt=0;
  ++tickCnt;
  
  int8_t pattern = cfg->pattern;
  uint8_t channel = cfg->channel;
  
  if( pattern == TEST_PATTERN_RX_PACKET )
  {
    static uint16_t uart_initialized=0;
    if( !uart_initialized ) {
      uart_initialized = 1;
      hal_uart_init();
      hal_uart_putchar('\n');
    }
    
    if( tickCnt >= 50 ) //250ms
    {
      tickCnt=0;
      
      static struct { uint16_t slowtick, pktcnt, syncerr, crcerr, irqcnt, rssi1, rssi2; } s = {0};
      s.slowtick++; // = s.slowtick<0xFFFF ? s.slowtick+1 : s.slowtick; //dont' wrap
      s.pktcnt = test_rx_packet_nr;
      s.syncerr = test_rx_packet_nr_syncerr;
      s.crcerr = test_rx_packet_nr_crcerr;
      s.irqcnt = test_rx_irq_cnt;
      s.rssi1 = rx_test_rssi_1;
      s.rssi2 = rx_test_rssi_2;
      
      //end rx test and print pkt count
      if( s.slowtick == 5*4 )
      {
        hal_led_set(led_off);
        hci_end_rx_prod_test_cmd();
        //uart_finish_transfers();
        
        hal_uart_putchar('\n');
        hal_uart_putchar('\n');
        //printU16_(s.slowtick);
        printU32_(s.pktcnt);
        printU16_(s.syncerr);
        printU16_(s.crcerr);
        printU16_(s.irqcnt);
        printU16_(s.rssi1);
        printU16_(s.rssi2);
        hal_uart_putchar('\n');
        
        /*/use uart as configured by the stack
        static struct msg_rx_stop_send_info_back msg;
        msg.packet_type   = HCI_EVT_MSG_TYPE;
        msg.event_code    = HCI_CMD_CMPL_EVT_CODE; 
        msg.length        = 11;
        msg.param0        = 0x01;
        msg.param_opcode  = HCI_LE_END_PROD_RX_TEST_CMD_OPCODE;		
        msg.nb_packet_received = s.pktcnt; //test_rx_packet_nr;
        msg.nb_syncerr    = s.syncerr; //test_rx_packet_nr_syncerr;
        msg.nb_crc_error  = s.crcerr; //test_rx_packet_nr_crcerr;
        msg.rx_test_rssi  = s.rssi2; //rx_test_rssi_2;
        uart_write( (uint8_t*)&msg, sizeof(struct msg_rx_stop_send_info_back), NULL );
        //uart_finish_transfers();
        //for (volatile int j = 0; j < 5000; j++);
        //-*/
      }
      
      //repeat rx cycle
      if( s.slowtick == 10*4 ) {
        s.slowtick = 0;
        hal_led_set( cfg->led );
        anki_start_hci_test();
      }
      
    }
  }
  
  if( pattern == TEST_PATTERN_TX_PACKET )
  {
    static int8_t m_state = -1;
    if( m_state<0 || (m_state==STATE_START_TX && test_state==STATE_IDLE) ) { //tx complete
      m_state = STATE_IDLE;
      hal_led_set(led_off);
    }
    
    if( m_state==STATE_IDLE && tickCnt >= 5*200 )
    {
      tickCnt=0;
      
      const uint16_t numPackets = 1000;
      uint8_t buf[] = {
        channel,                //test_freq = ptr_data[0];
        37,                     //test_data_len = ptr_data[1];
        TEST_PATTERN_01010101,  //test_data_pattern = ptr_data[2];
        numPackets&0xFF, (numPackets>>8)&0xFF //text_tx_nr_of_packets =  (ptr_data[4]<<8) | ptr_data[3];
      };
      
      hal_led_set( cfg->led );
      hci_tx_send_nb_packages( (uint8_t*)buf );
      m_state = STATE_START_TX;
    }
  }
}

/****************************************************************************************
* @brief app_on_init hook
****************************************************************************************/

//called from system_init()
void anki_app_on_init(void)
{
  hal_led_init();
  hal_acc_init();
}

extern uint8_t tap_count;

//NOTE: main_exec() called from hal_led timer irq
void main_exec(void) {
  static int countdown = COUNTDOWN_TIME + 50;
  static uint8_t last_count = ~0;
  
  //test mode locked
  if (countdown == 0) {
    anki_hci_manage();
    return ;
  }
  
  hal_acc_tick();
  if (--countdown > COUNTDOWN_TIME) //accel warm-up period
    return;
  
  if (countdown > 0) {
    if (tap_count != last_count) {
      last_count = tap_count;
      countdown = COUNTDOWN_TIME;
      if (++selected >= TOTAL_TEST_MODES) selected = 0;
    }
    cfg = &test_modes[selected];
    
    hal_led_set((countdown & 64) ? led_off : cfg->led);
  } else {
    
    //set final leds (some modes req. static pins)
    if( !cfg->led_static ) {
      hal_led_set(cfg->led);
    } else {
      hal_led_stop(); //Note: this also stops main_exec(), called from led timer
      led_set_static(cfg->led);
    }
    
    anki_start_hci_test();
  }

}


