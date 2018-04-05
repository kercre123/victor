#include <stdbool.h>
#include <stdint.h>
#include "hal_led.h"
#include "hal_timer.h"
#include "leds.h"

//switch hardware/isr timer or software polling to drive LED refresh
#define LEDS_USE_HW_TIMER 1

//------------------------------------------------  
//    Locals
//------------------------------------------------

static uint16_t m_leds_state = 0; //bitfield on/off
static bool m_timer_on = 0;

void led_timer_callback_(void) 
{
  static int n = 0;
  if( ++n >= HAL_NUM_LEDS )
    n = 0;
  
  hal_led_off();
  if( m_leds_state & (1<<n) )
    hal_led_on(n);
}

static void m_set_leds(uint16_t led_bf)
{
  m_leds_state = led_bf; //update led bitfield
  
  //manage the timer
  if( m_leds_state && !m_timer_on )
  {
    m_timer_on = 1;
    #if LEDS_USE_HW_TIMER > 0
      hal_timer_set_led_callback( led_timer_callback_, 695 ); //695us -> 120Hz @ 1:12 duty
    #endif
  }
  
  if( !m_leds_state && m_timer_on )
  {
    m_timer_on = 0;
    #if LEDS_USE_HW_TIMER > 0
      hal_timer_set_led_callback( 0, 0 ); //timer off
    #endif
    hal_led_off(); //make sure led outputs are disabled!
  }
}

//------------------------------------------------  
//    Interface
//------------------------------------------------

void leds_init(void)
{
  static uint8_t init = 0;
  if( !init ) {
    hal_led_init();
    hal_led_off();
    hal_timer_init();
    init = 1;
  }
}

void leds_set(uint16_t led_bf) {
  m_set_leds( led_bf );
}

void leds_on(uint16_t led_bf) {
  m_set_leds( m_leds_state | led_bf );
}

void leds_off(uint16_t led_bf) {
  m_set_leds( m_leds_state & ~led_bf );
}

void leds_manage(void)
{
#if LEDS_USE_HW_TIMER < 1
  static uint32_t start = 0;
  if( hal_timer_elapsed_us(start) >= 695 ) //695us -> 120Hz @ 1:12 duty
  {
    start = hal_timer_get_ticks();
    if( m_timer_on )
      led_timer_callback_();
  }
#endif
}

