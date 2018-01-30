#include <stdint.h>
#include "board.h"
#include "datasheet.h"
#include "gpio.h"
#include "hal_led.h"
#include "hal_timer.h"

//------------------------------------------------  
//    LED pwm config
//------------------------------------------------

typedef struct { 
  uint8_t period;
  uint8_t duty;
} led_pwm_t;

#define LED_PWM_RED   {32,16}
#define LED_PWM_GRN   {32,16}
#define LED_PWM_BLU   {32,16}

//map each led to pwm output freq & pulse width
static const led_pwm_t led_pwm[] = {
  LED_PWM_RED, //D1
  LED_PWM_GRN,
  LED_PWM_BLU,
  
  LED_PWM_RED, //D2
  LED_PWM_GRN,
  LED_PWM_BLU,
  
  LED_PWM_RED, //D3
  LED_PWM_GRN,
  LED_PWM_BLU,
  
  LED_PWM_RED, //D4
  LED_PWM_GRN,
  LED_PWM_BLU,
};

//------------------------------------------------  
//    Interface
//------------------------------------------------

void hal_led_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;
  
  board_init(); //cfg pins
  
  //board sets BOOST pin as 1V for v2.1 hardware. Revert to 3V for backwards compatibility
  GPIO_INIT_PIN(BOOST, OUTPUT, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  
  hal_pwm_init();
  hal_timer_init();
  hal_led_off();
  //hal_led_power(0);
  
  init = 1;
}

void hal_led_power(bool on)
{
  //na. forward compat for v2.1+ hw
}

static uint8_t m_hal_led_is_off = 0;
void hal_led_off(void)
{
  if( !m_hal_led_is_off )
  {
    hal_pwm_stop();
    hal_timer_wait(10); //wait for stored boost energy to dissipate
    
    GPIO_SET(D0);
    GPIO_SET(D1);
    GPIO_SET(D2);
    GPIO_SET(D3);
    GPIO_SET(D4);
    GPIO_SET(D5);
    GPIO_SET(D6);
    GPIO_SET(D7);
    GPIO_SET(D8);
    GPIO_SET(D9); //GPIO_ConfigurePinPower( GPIOPP(D9), GPIO_POWER_RAIL_3V );
    GPIO_SET(D10);
    GPIO_SET(D11);
  }
  m_hal_led_is_off = 1;
}

void hal_led_on(uint16_t n)
{
  hal_led_off();
  
  switch(n)
  {
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
    case HAL_LED_D4_GRN: GPIO_CLR(D10); /*GPIO_ConfigurePinPower( GPIOPP(D9), GPIO_POWER_RAIL_1V );*/ break;
    case HAL_LED_D4_BLU: GPIO_CLR(D9); break;
    
    default: return; //DO NOT turn on pwm without an active led / current path
  }
  
  m_hal_led_is_off = 0;
  hal_pwm_start( led_pwm[n].period, led_pwm[n].duty );
}

//------------------------------------------------  
//    DEBUG
//------------------------------------------------

//TEST DATA: VLED current (relative) vs pwm settings
//static const led_pwm_t pwm_settings[1] = {30,15};

//pwm settings for given color and number of active leds (index+1)={1..4}
//static const led_pwm_t pwm_red[4] = { {32,16}, {32,16}, {32,16}, {32,16} };
//static const led_pwm_t pwm_grn[4] = { {32,16}, {32,16}, {32,16}, {32,16} };
/*
static const led_pwm_t pwm_blu[4] = { {32,11},    //4.05mA  -> ~1.35mA/ea @ 1:3 duty
                                      {24, 9},    //8.33mA  -> ~1.38mA/ea @ 1:3 duty
                                      {24,10},    //12.58mA -> ~1.40mA/ea @ 1:3 duty
                                      {30,15} };  //16.82mA -> ~1.40mA/ea @ 1:3 duty
*/
/*
static const led_pwm_t pwm_blu[4] = { {22, 9},    //9.54mA  -> ~3.18mA/ea @ 1:3 duty
                                      {32,16},    //15.66mA -> ~2.61mA/ea @ 1:3 duty
                                      {32,16},    //16.6mA  -> ~1.84mA/ea @ 1:3 duty
                                      {32,16} };  //17.22mA -> ~1.44mA/ea @ 1:3 duty
*/

//#define TEST_LEDS_ON()    do { GPIO_CLR(D0); GPIO_CLR(D3); GPIO_CLR(D6); GPIO_CLR(D9); /*GPIO_ConfigurePinPower( GPIOPP(D9), GPIO_POWER_RAIL_1V )*/ } while(__LINE__ == -1)
//#define TEST_LEDS_OFF()   do { GPIO_SET(D0); GPIO_SET(D3); GPIO_SET(D6); GPIO_SET(D9); /*GPIO_ConfigurePinPower( GPIOPP(D9), GPIO_POWER_RAIL_3V )*/ } while(__LINE__ == -1)
/*#define TEST_LEDS_ON(n_) do { \
    if( n_ >= 1 ) { GPIO_CLR(D9); } \
    if( n_ >= 2 ) { GPIO_CLR(D6); } \
    if( n_ >= 3 ) { GPIO_CLR(D3); } \
    if( n_ >= 4 ) { GPIO_CLR(D0); } \
  } while(__LINE__ == -1) //-*/
/*#define TEST_LEDS_OFF(n_) do { \
    if( n_ >= 1 ) { GPIO_SET(D9); } \
    if( n_ >= 2 ) { GPIO_SET(D6); } \
    if( n_ >= 3 ) { GPIO_SET(D3); } \
    if( n_ >= 4 ) { GPIO_SET(D0); } \
  } while(__LINE__ == -1) //-*/

/*
static inline void hal_led_dbg_leds_on(uint16_t led_bf)
{
  if( led_bf & (1 << HAL_LED_D1_RED) ) { GPIO_CLR(D2); }
  if( led_bf & (1 << HAL_LED_D1_GRN) ) { GPIO_CLR(D1); }
  if( led_bf & (1 << HAL_LED_D1_BLU) ) { GPIO_CLR(D0); }
  if( led_bf & (1 << HAL_LED_D2_RED) ) { GPIO_CLR(D5); }
  if( led_bf & (1 << HAL_LED_D2_GRN) ) { GPIO_CLR(D4); }
  if( led_bf & (1 << HAL_LED_D2_BLU) ) { GPIO_CLR(D3); }
  if( led_bf & (1 << HAL_LED_D3_RED) ) { GPIO_CLR(D8); }
  if( led_bf & (1 << HAL_LED_D3_GRN) ) { GPIO_CLR(D7); }
  if( led_bf & (1 << HAL_LED_D3_BLU) ) { GPIO_CLR(D6); }
  if( led_bf & (1 << HAL_LED_D4_RED) ) { GPIO_CLR(D11); }
  if( led_bf & (1 << HAL_LED_D4_GRN) ) { GPIO_CLR(D10); }
  if( led_bf & (1 << HAL_LED_D4_BLU) ) { GPIO_CLR(D9); }
  m_hal_led_is_off = 0;
}
*/

static void hal_led_on_dbg_(uint16_t n, uint16_t period, uint16_t duty)
{
  hal_led_off();
  
  switch(n)
  {
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
    
    default: return; //DO NOT turn on pwm without an active led / current path
  }
  
  m_hal_led_is_off = 0;
  hal_pwm_start( period, duty );
}

//loop to drive test leds at a given duty cycle [1:slices]
void hal_led_dbg_duty_cycle_(int slices, uint8_t led_n, int duration_ms)
{
  uint16_t pwm_period = 32; //pwm_blu[(num_leds-1)&3].period; //pwm_settings[0].period;
  uint16_t pwm_duty   = 16; //pwm_blu[(num_leds-1)&3].duty;   //pwm_settings[0].duty;
  uint8_t is_off_ = 1;
  
  do
  {
    for( int i=0; i < slices; i++ )
    {
      if( i == 0 && is_off_ ) {
        //TEST_LEDS_ON(num_leds);
        //hal_pwm_start( pwm_period, pwm_duty );
        hal_led_on_dbg_( led_n, pwm_period, pwm_duty );
        is_off_ = 0;
      } else if( i > 0 && !is_off_ ) {
        //hal_pwm_stop();
        //hal_timer_wait( VLED_drain_time_us );
        //TEST_LEDS_OFF(num_leds);
        hal_led_off();
        is_off_ = 1;
      }
      
      //hal_timer_wait(1000); //slice time
      extern void hal_led_dbg_delay_ms(uint32_t ms);
      hal_led_dbg_delay_ms(1);
    }
  }
  while( (duration_ms -= 1*slices) > -1 );
  
  //hal_pwm_stop();
  //hal_timer_wait( VLED_drain_time_us );
  //TEST_LEDS_OFF(num_leds);
  hal_led_off();
}

void hal_led_dbg_delay_ms(uint32_t ms)
{
  uint32_t start = hal_timer_get_ticks();
  while( hal_timer_elapsed_us(start) < 1000*ms )
  {
    //extern void vbat_manage(void);
    //vbat_manage(); //monitor/print battery voltages
  }
}

static volatile uint32_t leds_bf = 0;
void hal_led_update_(void) 
{
  static int n = 0;
  if( ++n >= 12 )
    n = 0;
  
  hal_led_off();
  if( leds_bf & (1<<n) )
    hal_led_on(n);
}

void hal_led_dbg_test_no_return(void)
{
  hal_led_init();
  hal_led_off();
  
  while(1)
  {
    /*/1:1 drive
    hal_led_on(HAL_LED_D3_RED); hal_led_dbg_delay_ms(5000); hal_led_off(); 
    //hal_led_dbg_delay_ms(2000);
    hal_led_on(HAL_LED_D3_GRN); hal_led_dbg_delay_ms(5000); hal_led_off();
    //hal_led_dbg_delay_ms(2000);
    hal_led_on(HAL_LED_D3_BLU); hal_led_dbg_delay_ms(5000); hal_led_off(); 
    //hal_led_dbg_delay_ms(2000);
    hal_led_dbg_delay_ms(5000);
    //-*/
    
    //1:12 drive
    hal_timer_set_led_callback( hal_led_update_, 695 ); //695us -> 120Hz @ 1:12 duty
    {
      leds_bf = (0); hal_led_dbg_delay_ms(1000);
      leds_bf = (1 << HAL_LED_D3_RED);  hal_led_dbg_delay_ms(3000);
      leds_bf = (1 << HAL_LED_D3_GRN);  hal_led_dbg_delay_ms(3000);
      leds_bf = (1 << HAL_LED_D3_BLU);  hal_led_dbg_delay_ms(3000);
      
      leds_bf = (0); hal_led_dbg_delay_ms(1000);
      leds_bf = (1 << HAL_LED_D3_RED) | (1 << HAL_LED_D3_GRN) | (1 << HAL_LED_D3_BLU); hal_led_dbg_delay_ms(2000);
    }
    hal_timer_set_led_callback( 0, 0 );
    //-*/
    
  }
  
  //while(1)
  {
    /*/brightness compare normalized led currents
    hal_led_dbg_duty_cycle_(3, 1, 2000);
    hal_led_dbg_duty_cycle_(3, 2, 2000);
    hal_led_dbg_duty_cycle_(3, 3, 2000);
    hal_led_dbg_duty_cycle_(3, 4, 2000);
    //-*/
    
    /*/brightness compare against 1.0 cube
    hal_led_dbg_duty_cycle_(3, 1, 5000);
    //-*/
    
    /*/pwm data gathing
    hal_led_dbg_duty_cycle_( 1, 1, 3000 );
    hal_timer_wait(3*1000*1000);
    hal_led_dbg_duty_cycle_( 1, 2, 3000 );
    hal_timer_wait(3*1000*1000);
    hal_led_dbg_duty_cycle_( 1, 3, 2000 );
    hal_timer_wait(3*1000*1000);
    hal_led_dbg_duty_cycle_( 1, 4, 2000 );
    hal_timer_wait(3*1000*1000);
    //-*/
  }
}

