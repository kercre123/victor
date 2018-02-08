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


/****************************************************************************************
* @brief ANKI LED stuff
****************************************************************************************/

#include "board.h"

//led bitfield
#define LED_D1_RED    0x0001
#define LED_D1_GRN    0x0002
#define LED_D1_BLU    0x0004
#define LED_D2_RED    0x0008
#define LED_D2_GRN    0x0010
#define LED_D2_BLU    0x0020
#define LED_D3_RED    0x0040
#define LED_D3_GRN    0x0080
#define LED_D3_BLU    0x0100
#define LED_D4_RED    0x0200
#define LED_D4_GRN    0x0400
#define LED_D4_BLU    0x0800

void anki_led_on( uint16_t led_bf )
{
  board_init();
  GPIO_CLR(BOOST); //disable VLED
  
  //turn on specified leds
  if( led_bf & LED_D1_RED ) { GPIO_CLR(D2); }
  if( led_bf & LED_D1_GRN ) { GPIO_CLR(D1); }
  if( led_bf & LED_D1_BLU ) { GPIO_CLR(D0); }
  if( led_bf & LED_D2_RED ) { GPIO_CLR(D5); }
  if( led_bf & LED_D2_GRN ) { GPIO_CLR(D4); }
  if( led_bf & LED_D2_BLU ) { GPIO_CLR(D3); }
  if( led_bf & LED_D3_RED ) { GPIO_CLR(D8); }
  if( led_bf & LED_D3_GRN ) { GPIO_CLR(D7); }
  if( led_bf & LED_D3_BLU ) { GPIO_CLR(D6); }
  if( led_bf & LED_D4_RED ) { GPIO_CLR(D11); }
  if( led_bf & LED_D4_GRN ) { GPIO_CLR(D10); }
  if( led_bf & LED_D4_BLU ) { GPIO_CLR(D9); }
  
  //enable VLED
  if( led_bf > 0 ) {
    GPIO_SET(BOOST);
  }
}


/****************************************************************************************
* @brief ANKI Hacky hack time
****************************************************************************************/

#define chan2freq(cn)   ( 2402 + 2*(cn) )
#define freq2chan(f)    ( ((f)-2402) / 2 ) //valid >= 2402

enum test_pattern_e {
  TEST_PATTERN_NA_CARRIER       = -1,
  TEST_PATTERN_PSEUDO_RANDOM_9  = 0x00, //傍est pattern is Pseudo random 9
  TEST_PATTERN_11110000         = 0x01, //傍est pattern is 11110000
  TEST_PATTERN_10101010         = 0x02, //傍est pattern is 10101010
  TEST_PATTERN_PSEUDO_RANDOM_15 = 0x03, //傍est pattern is Pseudo random 15
  TEST_PATTERN_11111111         = 0x04, //傍est pattern is 11111111
  TEST_PATTERN_00000000         = 0x05, //傍est pattern is 00000000
  TEST_PATTERN_00001111         = 0x06, //傍est pattern is 00001111
  TEST_PATTERN_01010101         = 0x07, //傍est pattern is 01010101
};

void anki_start_tx( uint16_t freq_mhz, int test_pattern )
{
  if( freq_mhz < 2402 )
    freq_mhz = 2402;
  if( freq_mhz > 2480 )
    freq_mhz = 2480;
  
  if( test_pattern < 0 ) //Carrier
  {
    uint8_t buf[] = { 'T', freq2chan(freq_mhz) };
    hci_unmodulated_cmd( (uint8_t*)buf ); //start the carrier
  }
  else //modulated
  {
    uint8_t buf[] = { freq2chan(freq_mhz), test_pattern & 0xFF };
    hci_tx_start_continue_test_mode( (uint8_t*)buf );
  }
}


/****************************************************************************************
* @brief app_on_init hook
****************************************************************************************/

typedef struct {
  uint16_t  freq_mhz;
  int       pattern;
  uint16_t  led_bitfield;
} test_cfg_t;

//called from system_init()
void anki_app_on_init(void)
{
  //pick a test. any test
  const test_cfg_t cfg = { 2402, TEST_PATTERN_NA_CARRIER,       LED_D2_RED | LED_D4_RED };
  //const test_cfg_t cfg = { 2442, TEST_PATTERN_NA_CARRIER,       LED_D2_GRN | LED_D4_GRN };
  //const test_cfg_t cfg = { 2480, TEST_PATTERN_NA_CARRIER,       LED_D2_BLU | LED_D4_BLU };
  //const test_cfg_t cfg = { 2402, TEST_PATTERN_PSEUDO_RANDOM_15, LED_D2_RED };
  //const test_cfg_t cfg = { 2442, TEST_PATTERN_PSEUDO_RANDOM_15, LED_D2_GRN };
  //const test_cfg_t cfg = { 2480, TEST_PATTERN_PSEUDO_RANDOM_15, LED_D2_BLU };
  
  anki_start_tx( cfg.freq_mhz, cfg.pattern );
  anki_led_on( cfg.led_bitfield );
  
  //Note1: LED states are chosen to match cube 1.0 FCC patterns (avoids confusion)
  //Note2: Cube 1.0 used f{2402,2442,2482}. We use 2480 (2482 not available)
}

/*
CUBE 1.0 FCC FIRMWARE, OPERATIONAL NOTES (FROM EMAIL MAY 25, 2016):
-------------------------------------------------------------------
3 or 4 Blue LEDs:  RX mode

1 Red LED:  TX cube packets at 2402MHz
1 Green LED:  TX cube packets at 2442MHz
1 Blue LED:  TX cube packets at 2482MHz

2 Red, Green, or Blue LEDs:  TX carrier signals at 2402,2442,2482MHz

If 1 LED is lit, the LED lights at max brightness, suitable for LED testing.
If 2 LEDs are lit, the LED lights at 50% brightness.
*/

