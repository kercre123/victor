#ifndef HAL_TIMER_H_
#define HAL_TIMER_H_

#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

#define HAL_TIMER_FREQ            (16000000/10) //1.6MHz
#define HAL_TIMER_TICKS_PER_MS    1600

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void     hal_timer_init(void);
uint32_t hal_timer_get_ticks(void); //return tick counter
uint32_t hal_timer_elapsed_us(uint32_t ticks_base);
void     hal_timer_wait(uint32_t us);

void     hal_timer_register_callback( void(*callback)(void) ); //intterrupt on internal tmr counter overflow
void     hal_timer_set_led_callback( void(*callback)(void), uint16_t period_us ); //callback at specified period

#endif //HAL_TIMER_H_

