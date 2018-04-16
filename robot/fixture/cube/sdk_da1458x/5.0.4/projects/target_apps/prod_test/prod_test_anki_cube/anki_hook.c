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

/****************************************************************************************
* @brief ANKI Hacky hack time
****************************************************************************************/

#define chan2freq(cn)   ( 2402 + 2*(cn) )
#define freq2chan(f)    ( ((f)-2402) / 2 ) //valid >= 2402

enum test_pattern_e {
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

void anki_start_hci_test( uint16_t freq_mhz, int test_pattern )
{
  uint8_t channel = freq_mhz < 2402 ? 0 : (freq_mhz > 2480 ? freq2chan(2480) : freq2chan(freq_mhz));
  
  if( test_pattern == TEST_PATTERN_RX_PACKET ) {
    uint8_t buf[] = { channel };
    hci_start_prod_rx_test( (uint8_t*)buf );
  }
  else if( test_pattern == TEST_PATTERN_RX_UNMODULATED ) {
    uint8_t buf[] = { 'R', channel };
    hci_unmodulated_cmd( (uint8_t*)buf );
  }
  else if( test_pattern == TEST_PATTERN_TX_CARRIER ) {
    uint8_t buf[] = { 'T', channel };
    hci_unmodulated_cmd( (uint8_t*)buf );
  }
  else if( test_pattern >= 0 ) { //TX modulated
    uint8_t buf[] = { channel, test_pattern & 0xFF };
    hci_tx_start_continue_test_mode( (uint8_t*)buf );
  }
}

/****************************************************************************************
* @brief app_on_init hook
****************************************************************************************/

/*
Cube test modes and LED patterns for all 7 tests
0: RX-umod 2402     4 Green LED @ 100% duty
1: RX-pkt  2402     4 Blue  LED @ 100% duty

2: TX Data 2402     1 Red   LED @ 100% duty
3: TX Data 2442     1 Green LED @ 100% duty
4: TX Data 2480     1 Blue  LED @ 100% duty

5: TX Tone 2402     2 Red   LED @  50% duty
6: TX Tone 2442     2 Green LED @  50% duty
7: TX Tone 2480     2 Blue  LED @  50% duty

RX should receive continuously.
TX should transmit continuously.
"Data" is all 0x55 bytes.
"Tone" is a carrier/continuous wave.

LED brightness is also part of the certification.
For tests 0-4, just set the LED GPIOs and walk away.
The tests 5-7, LED modulation is required, but can be at any frequency >50Hz (such as loop iterations).
*/

typedef struct {
  uint16_t  freq_mhz;
  int       pattern;
  uint8_t   led[12];
  bool      led_static;
} test_cfg_t;

static const int COUNTDOWN_TIME   = 200 * 10; // 10 seconds to trigger
extern uint8_t tap_count;

static const test_cfg_t test_modes[] = {
  { 2402, TEST_PATTERN_RX_UNMODULATED , { 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00 }, true },
  { 2402, TEST_PATTERN_RX_PACKET      , { 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF }, true },
  { 2402, TEST_PATTERN_01010101       , { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, true },
  { 2442, TEST_PATTERN_01010101       , { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, true },
  { 2480, TEST_PATTERN_01010101       , { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, true },
  { 2402, TEST_PATTERN_TX_CARRIER     , { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 }, false },
  { 2442, TEST_PATTERN_TX_CARRIER     , { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 }, false },
  { 2480, TEST_PATTERN_TX_CARRIER     , { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 }, false }
};
static const int TOTAL_TEST_MODES = sizeof(test_modes)/sizeof(test_cfg_t);

//called from system_init()
void anki_app_on_init(void)
{
  hal_led_init();
  hal_acc_init();
}

void led_set_static(const uint8_t* colors)
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

void main_exec(void) {
  static int countdown = COUNTDOWN_TIME;
  static uint8_t last_count = ~0;
  static int selected = TOTAL_TEST_MODES-1;

  // We are already transmitting
  if (countdown == 0) return ;

  // Select mode
  const test_cfg_t *cfg = &test_modes[selected];
  
  if (--countdown > 0) {
    if (tap_count != last_count) {
      last_count = tap_count;
      countdown = COUNTDOWN_TIME;
      if (++selected >= TOTAL_TEST_MODES) selected = 0;
    }

    static const uint8_t off[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
    
    hal_led_set((countdown & 64) ? off : cfg->led);
  } else {
    
    //set final leds (some modes req. static pins)
    if( !cfg->led_static ) {
      hal_led_set(cfg->led);
    } else {
      hal_led_stop();
      led_set_static(cfg->led);
    }
    
    anki_start_hci_test(cfg->freq_mhz, cfg->pattern);
  }

  hal_acc_tick();
}


