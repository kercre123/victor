#include <assert.h>
#include <stdint.h>
#include "datasheet.h"
#include <core_cm0.h>
#include "board.h"
#include "common.h"
#include "hal_timer.h"

//select hardware timer0 or systick counter
#ifndef HAL_TIMER_USE_0_TMR0_1_SYSTICK
#define HAL_TIMER_USE_0_TMR0_1_SYSTICK   1
#endif

#if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0
static const uint32_t FsysMHz = 1; //1MHz with reference clock selected. Otherwise (SYSTEM_CLOCK / 1000000);
static const uint32_t SysTickReloadVal = 0xFFFF * FsysMHz;
#endif

static void(*m_hal_timer_isr_callback)(void) = NULL;
static void(*m_hal_timer_LED_callback)(void) = NULL;

void hal_timer_init(void)
{
  static uint8_t init = 0;
  if( init ) //already initialized
    return;
  
  #if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0
    NVIC_DisableIRQ(SysTick_IRQn);
  
    //Note about SysTick clk src (CTRL reg): If your device does not implement a reference clock, this bit reads-as-one and ignore writes.
    SysTick->LOAD = SysTickReloadVal; //set reload such that 1 counter cycle is exactly 65,536us (16-bit us counter)
    SysTick->VAL  = SysTickReloadVal;
    SysTick->CTRL = //SysTick_CTRL_CLKSOURCE_Msk |  //0 = reference 1MHz clock, 1 = processor clock
                    //SysTick_CTRL_TICKINT_Msk |    //1 = counting down to zero asserts the SysTick exception request
                    SysTick_CTRL_ENABLE_Msk;      //1 = counter enabled
    
  #else
    SetBits16(TIMER0_CTRL_REG, TIM0_CTRL, 0); //Timer0 off
    SetBits16(CLK_PER_REG, TMR_ENABLE, 1);    //enable TIMER0,TIMER2 clock
    SetBits16(CLK_PER_REG, TMR_DIV, 0);       //CLK_PER_REG.TMR_DIV<1:0> = {0:divby1->16Mhz, 1:divby2->8MHz, 2:divby4->4Mhz, 3:divby8->8MHz}
    
    // clear PWM settings register to not generate PWM
    SetWord16(TIMER0_RELOAD_M_REG, 0);
    SetWord16(TIMER0_RELOAD_N_REG, 0);
    SetBits16(TIMER0_CTRL_REG, PWM_MODE, 0);
    
    //config: Freq=1.6MHz -> T=0.625us, low priority interrupts
    SetBits16(TIMER0_CTRL_REG, TIM0_CLK_DIV, 0);  //1: use clock directly, 0: divide by 10
    SetBits16(TIMER0_CTRL_REG, TIM0_CLK_SEL, 1);  //1: use fast clock (2-16MHz, as configured), 0: use slow clock (32kHz)
    NVIC_SetPriority (SWTIM_IRQn, 3);             //set Priority for TIM0 Interrupt to be the lowest
    
    //reload value, use full range
    SetWord16(TIMER0_ON_REG, 0xFFFF);
    
    //Timer0 run
    SetBits16(TIMER0_CTRL_REG, TIM0_CTRL, 1);
    
  #endif
  
  init = 1;
}

uint32_t hal_timer_get_ticks(void)
{
  // The code below turns a 16-bit into a 32-bit timer (with frequent polling)
  volatile static uint16_t high = 0;	// Supply the missing high bits
  volatile static uint16_t last = 0;	// Last read hardware counter
  uint32_t now;
  
  // NOTE:  This must be interrupt-safe for encoder code, so take care below
  __disable_irq();
  {
    #if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0
      //reload value (e.g. max count) is calculated such that resulting range is 0..65k (16-bit)
      uint32_t count = SysTickReloadVal - SysTick->VAL; //make count-down into count-up
      now = count / FsysMHz; //#us = count*Tsys[us] = count/Fsys[MHz]
    #else
      now = 0xFFFF - GetWord16(TIMER0_ON_REG); //down-counter to up-counter
    #endif
    
    if (now < last)	// Each time we wrap, increase the high part by 1
      high++;
    
    last = now;
    now |= (high << 16);
  }
  __enable_irq();
  
  return now;
}

uint32_t hal_timer_elapsed_us(uint32_t ticks_base)
{
  uint32_t tick_diff = hal_timer_get_ticks() - ticks_base;
  
  #if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0
    return tick_diff; //systick 1us period, full counter range
  #else
    //the maximum return value due to tick resolution is 0x9FFFff47
    if( tick_diff < 0xFFFFffff/625 )
      return (tick_diff * 625) / 1000; //1 tick = 0.625us
    else
      return (tick_diff / 1000) * 625; //lose a bit of precision, but avoids destructive maths overflow
  #endif  
}

void hal_timer_wait(uint32_t us)
{
  //this will break if us > ~0x9FFFff47 (when using Timer0)
  uint32_t start = hal_timer_get_ticks();
  while( hal_timer_elapsed_us(start) < us )
    ;
}

void hal_timer_register_callback( void(*callback)(void) )
{
  #if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; //disable SysTick exception
    if( (m_hal_timer_isr_callback = callback) != NULL )
      SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    
  #else
    NVIC_DisableIRQ(SWTIM_IRQn);
    if( (m_hal_timer_isr_callback = callback) != NULL )
      NVIC_EnableIRQ(SWTIM_IRQn);
    
  #endif  
}

void hal_timer_set_led_callback( void(*callback)(void), uint16_t period_us )
{
  #if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0 //Timer 0 is available to use for LED callbacks
  
    NVIC_DisableIRQ(SWTIM_IRQn);
    SetBits16(TIMER0_CTRL_REG, TIM0_CTRL, 0); //Timer0 off
  
    static uint8_t init = 0;
    if( !init )
    {
      init = 1;
      
      SetBits16(CLK_PER_REG, TMR_ENABLE, 1);    //enable TIMER0,TIMER2 clock
      SetBits16(CLK_PER_REG, TMR_DIV, 0);       //CLK_PER_REG.TMR_DIV<1:0> = {0:divby1->16Mhz, 1:divby2->8MHz, 2:divby4->4Mhz, 3:divby8->8MHz}
      
      SetWord16(TIMER0_RELOAD_M_REG, 0);        // clear PWM settings register to not generate PWM
      SetWord16(TIMER0_RELOAD_N_REG, 0);        // "
      SetBits16(TIMER0_CTRL_REG, PWM_MODE, 0);  // "
      
      //config: Freq=1.6MHz -> T=0.625us, low priority interrupts
      SetBits16(TIMER0_CTRL_REG, TIM0_CLK_DIV, 0);  //1: use clock directly, 0: divide by 10
      SetBits16(TIMER0_CTRL_REG, TIM0_CLK_SEL, 1);  //1: use fast clock (2-16MHz, as configured), 0: use slow clock (32kHz)
      NVIC_SetPriority (SWTIM_IRQn, 3);             //set Priority for TIM0 Interrupt to be the lowest
    }
    
    if( (m_hal_timer_LED_callback = callback) != NULL && period_us > 1 )
    {
      //Tisr[us] = (reload+1) * 0.625us
      uint16_t reload = (((uint32_t)period_us * 1000) / 625) - 1; //reload overflow at period > 40,960us
      SetWord16(TIMER0_ON_REG, reload);
      
      //Timer0 run
      SetBits16(TIMER0_CTRL_REG, TIM0_CTRL, 1);
      
      NVIC_EnableIRQ(SWTIM_IRQn);
    }
    
  #else
    (void)m_hal_timer_LED_callback; //led callback disabled
    (void)period_us;
  #endif 
}

//Interrupt handler
#if HAL_TIMER_USE_0_TMR0_1_SYSTICK > 0

  void $Sub$$SysTick_Handler(void) //$Sub$$ overrides ROM handler
  {
    if( m_hal_timer_isr_callback != NULL )
      m_hal_timer_isr_callback();
  }

  void SWTIM_Handler(void) //Timer 0 is led callback
  {
    if( m_hal_timer_LED_callback != NULL )
      m_hal_timer_LED_callback();
  }

#else

  void SWTIM_Handler(void) 
  {
    if( m_hal_timer_isr_callback != NULL )
      m_hal_timer_isr_callback();
  }

  //LED callback disabled
  
#endif

