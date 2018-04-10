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
  uint8_t   led[12];
} test_cfg_t;

static const int TOTAL_TEST_MODES = 6;
static const int COUNTDOWN_TIME   = 200 * 10; // 10 seconds to trigger
extern uint8_t tap_count;

static const test_cfg_t test_modes[TOTAL_TEST_MODES] = {
  { 2402, TEST_PATTERN_NA_CARRIER, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { 2442, TEST_PATTERN_NA_CARRIER, { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { 2480, TEST_PATTERN_NA_CARRIER, { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { 2402,   TEST_PATTERN_01010101, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { 2442,   TEST_PATTERN_01010101, { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 } },
  { 2480,   TEST_PATTERN_01010101, { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 } }
};

//called from system_init()
void anki_app_on_init(void)
{
  //;
  //anki_led_on( cfg.led_bitfield );
  
  //Note1: LED states are chosen to match cube 1.0 FCC patterns (avoids confusion)
  //Note2: Cube 1.0 used f{2402,2442,2482}. We use 2480 (2482 not available)
  
  hal_led_init();
  hal_acc_init();
}

void main_exec() {
  static int countdown = COUNTDOWN_TIME;
  static uint8_t last_count = ~0;
  static int selected = 0;

  // We are already transmitting
  if (countdown == 0) return ;

  // Select mode
  const test_cfg_t *cfg = &test_modes[selected];
  
  if (--countdown > 0) {
    if (tap_count != last_count) {
      last_count = tap_count;
      if (++selected >= TOTAL_TEST_MODES) selected = 0;
    }

    static const uint8_t off[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
    
    hal_led_set((countdown & 128) ? off : cfg->led);
  } else {
    anki_start_tx(cfg->freq_mhz, cfg->pattern);
  }

  hal_acc_tick();
}
