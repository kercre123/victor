#include <assert.h>
#include <stdint.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "board.h"
#include "common.h"
#include "hal_pwm.h"  

void hal_pwm_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;
  
  //cfg PWM2 pin
  GPIO_INIT_PIN(BOOST, OUTPUT, PID_PWM2, 0, GPIO_POWER_RAIL_3V );
  
  //Enables TIMER0,TIMER2 clock
  SetBits16(CLK_PER_REG, TMR_ENABLE, 1);
  
  //set TIMER0,TIMER2 clock division factor
  //Ttick = 1/CLK = {62.5,125,250,500}ns @ divider {1,2,4,8}
  SetBits16(CLK_PER_REG, TMR_DIV, 0); //CLK_PER_REG.TMR_DIV<1:0> = {0:divby1->16Mhz, 1:divby2->8MHz, 2:divby4->4Mhz, 3:divby8->8MHz}
  
  //init timer2 PWM
  SetBits16(TRIPLE_PWM_CTRL_REG, TRIPLE_PWM_ENABLE, 1);
  SetBits16(TRIPLE_PWM_CTRL_REG, HW_PAUSE_EN, 0);
  SetBits16(TRIPLE_PWM_CTRL_REG, SW_PAUSE_EN, 1);
  //SetWord16(TRIPLE_PWM_FREQUENCY, 0x3fff);
  
  init = 1;
}

void hal_pwm_stop(void) {
  SetBits16(TRIPLE_PWM_CTRL_REG, SW_PAUSE_EN, 1);
  SetWord16(PWM2_DUTY_CYCLE, 1 ); //duty must be < 100% or output will not clear
}

static inline void hal_pwm_run_(void) {
  SetBits16(TRIPLE_PWM_CTRL_REG, SW_PAUSE_EN, 0); //note: resuming from pause, first period is not at specified duty/freq
}

void hal_pwm_start(uint16_t period, uint16_t duty)
{
  hal_pwm_stop();
  
  //Set the period [frequency] register
  period = period < 3 ? 3 : (period > 0x4000 ? 0x4000 : period);  //period value valid range {3..0x4000}
  SetWord16(TRIPLE_PWM_FREQUENCY, period - 1 ); //period register valid range {2..0x3FFF}
  
  if( duty < 2 ) //-> 0% OFF
    return;
  else if( duty >= period ) //100% ON
    SetWord16(PWM2_DUTY_CYCLE, period ); //TRIPLE_PWM_FREQUENCY+1 will set output to 1
  else
    SetWord16(PWM2_DUTY_CYCLE, duty - 1 ); //duty register valid range {1..TRIPLE_PWM_FREQUENCY-2}
  
  hal_pwm_run_();
}

//------------------------------------------------  
//    Debug
//------------------------------------------------

/*#include "hal_timer.h"
void hal_pwm_debug_no_return(void)
{
  const int state_time_us = 1*1000;
  hal_pwm_init();
  hal_pwm_stop();
  
  //pwm output on CAPI pin instead of BOOST
  GPIO_INIT_PIN(BOOST, INPUT_PULLDOWN, PID_GPIO, 0, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(CAPI,  OUTPUT,         PID_PWM2, 0, GPIO_POWER_RAIL_3V );
  
  int cnt = 0;
  while(1)
  {
    hal_timer_wait(state_time_us);
    switch(++cnt) {
      case 0:   hal_pwm_stop(); break;
      case 1:   hal_pwm_start(32, 8); break;
      case 2:   hal_pwm_stop(); break;
      case 3:   hal_pwm_start(32, 16); break;
      case 4:   hal_pwm_stop(); break;
      case 5:   hal_pwm_start(32, 24); break;
      case 6:   hal_pwm_stop(); break;
      case 7:   hal_pwm_start(32, 32); break;
      case 8:   hal_pwm_stop(); break;
      case 9:   hal_pwm_start(32, 100); break;
      case 10:  hal_pwm_stop(); break;
      default:  cnt = 0; break;
    }
  }
}
//-*/
