#ifndef HAL_PWM_H_
#define HAL_PWM_H_

#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

#define HAL_PWM_CLK             (16000000/1) //fast clock, divide-by-1 = 16MHz count freq
#define HAL_PWM_FREQ(period_)   (HAL_PWM_CLK/(period_))

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void hal_pwm_init(void);
void hal_pwm_stop(void);

//period - valid range {3..0x4000}
//duty - valid range {2..period-1}
void hal_pwm_start(uint16_t period, uint16_t duty);

#endif //HAL_PWM_H_

