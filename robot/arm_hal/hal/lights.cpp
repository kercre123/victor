#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {			                
      GPIO_PIN_SOURCE(RED, GPIOH, 11);
      GPIO_PIN_SOURCE(GREEN, GPIOH, 12);
      GPIO_PIN_SOURCE(BLUE, GPIOH, 8);
      GPIO_PIN_SOURCE(EYECLK, GPIOA, 7);
      GPIO_PIN_SOURCE(EYERST, GPIOB, 1);
      
      GPIO_PIN_SOURCE(HEADLIGHT1, GPIOB, 3);
      GPIO_PIN_SOURCE(HEADLIGHT2, GPIOH, 13);
 
      // Map the natural LED order (as shown in hal.h) to the hardware swizzled order
      // See schematic if you care about why the LEDs are swizzled
      static const u8 HW_CHANNELS[8] = {5, 1, 2, 0, 6, 7, 3, 4};
      static u32 m_channels[8];   // The actual RGB color values in use
      
      // Initialize LED head/face light hardware
      void LightsInit()
      {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);

        // Leave the high side pins off until first LED is set
        GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
        PIN_OD(GPIO_EYECLK, SOURCE_EYECLK);   // For 2.1 pullup mod
        PIN_OUT(GPIO_EYECLK, SOURCE_EYECLK);        
        GPIO_RESET(GPIO_EYERST, PIN_EYERST);
        PIN_OD(GPIO_EYERST, SOURCE_EYERST);
        PIN_OUT(GPIO_EYERST, SOURCE_EYERST);

        // Initialize all face LED colors to OFF
        // Low side drivers must be open drain to allow voltages > VDD
        GPIO_SET(GPIO_RED, PIN_RED);        
        GPIO_SET(GPIO_GREEN, PIN_GREEN);        
        GPIO_SET(GPIO_BLUE, PIN_BLUE);
        PIN_NOPULL(GPIO_RED, SOURCE_RED);
        PIN_NOPULL(GPIO_GREEN, SOURCE_GREEN);        
        PIN_NOPULL(GPIO_BLUE, SOURCE_BLUE);
        PIN_OD(GPIO_RED, SOURCE_RED);
        PIN_OD(GPIO_GREEN, SOURCE_GREEN);        
        PIN_OD(GPIO_BLUE, SOURCE_BLUE);
        PIN_OUT(GPIO_RED, SOURCE_RED);
        PIN_OUT(GPIO_GREEN, SOURCE_GREEN);        
        PIN_OUT(GPIO_BLUE, SOURCE_BLUE);     

        // Initialize headlights (2.1)
        GPIO_SET(GPIO_HEADLIGHT1, PIN_HEADLIGHT1);        
        PIN_NOPULL(GPIO_HEADLIGHT1, SOURCE_HEADLIGHT1);
        PIN_OD(GPIO_HEADLIGHT1, SOURCE_HEADLIGHT1);
        PIN_OUT(GPIO_HEADLIGHT1, SOURCE_HEADLIGHT1);
        GPIO_SET(GPIO_HEADLIGHT2, PIN_HEADLIGHT2);        
        PIN_NOPULL(GPIO_HEADLIGHT2, SOURCE_HEADLIGHT2);
        PIN_OD(GPIO_HEADLIGHT2, SOURCE_HEADLIGHT2);
        PIN_OUT(GPIO_HEADLIGHT2, SOURCE_HEADLIGHT2);

        // Initialize timer to rapidly blink LEDs, simulating dimming
        NVIC_InitTypeDef NVIC_InitStructure;
        TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);

        // Set up clock to multiplex the LEDs - must keep above 100Hz to hide flicker
        // 173Hz = 90MHz/8 eyes/(255^2)/(Prescaler+1)
        TIM_TimeBaseStructure.TIM_Prescaler = 0;
        TIM_TimeBaseStructure.TIM_Period = 1000;  // Take a second before starting
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);

        // Enable timer and interrupts
        TIM_SelectOnePulseMode(TIM14, TIM_OPMode_Single);
        TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);
        TIM_Cmd(TIM14, ENABLE);
        TIM14->CR1 |= TIM_CR1_URS;  // Prevent spurious interrupt when we touch EGR

        // Route interrupt
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannel = TIM8_TRG_COM_TIM14_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_Init(&NVIC_InitStructure);      
      }
            
      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, LEDColor color)
      {
        if (led_id < NUM_LEDS)  // Unsigned, so always >= 0
          m_channels[HW_CHANNELS[led_id]] = color;
      }      
      
      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state)
      {
        if (state) {
          GPIO_RESET(GPIO_HEADLIGHT1, PIN_HEADLIGHT1);
          GPIO_RESET(GPIO_HEADLIGHT2, PIN_HEADLIGHT2);
        } else {
          GPIO_SET(GPIO_HEADLIGHT1, PIN_HEADLIGHT1);
          GPIO_SET(GPIO_HEADLIGHT2, PIN_HEADLIGHT2);
        }
      }
    }
  }
}

extern "C" void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;  
  static u8 s_which = 0;    // Which LED to light
  static u8 s_time = 0;     // The current 'time' in the per-LED PWM cycle (0xff = full bright)
  static u32 s_color = 0;   // Cache the current color so it doesn't change from underneath is
  
  // If this is the end of the last LED, switch to the next LED
  if (0 == s_time)
  {
    // Start with darkness
    GPIO_SET(GPIO_RED, PIN_RED);        
    GPIO_SET(GPIO_GREEN, PIN_GREEN);        
    GPIO_SET(GPIO_BLUE, PIN_BLUE);
        
    // Point to the start of the next light
    s_time = 255;
    if (++s_which >= sizeof(HW_CHANNELS))
      s_which = 0;
    s_color = m_channels[s_which];

    // Reset LED number to first LED, or advance to next
    if (0 == s_which)
      GPIO_SET(GPIO_EYERST, PIN_EYERST);
    else
      GPIO_SET(GPIO_EYECLK, PIN_EYECLK);    
  }
    
  // Turn on any LEDs that match this turn-on time
  u8* color = (u8*)(&s_color);
  if (s_time == color[2])
    GPIO_RESET(GPIO_RED, PIN_RED);
  if (s_time == color[1])
    GPIO_RESET(GPIO_GREEN, PIN_GREEN);
  if (s_time == color[0])
    GPIO_RESET(GPIO_BLUE, PIN_BLUE);
  
  // Figure out the next soonest time we need to turn on a light
  u8 nexttime = 0;
  for (int i = 0; i < 3; i++)
    if (color[i] > nexttime && color[i] < s_time) // Soonest time earlier than now
      nexttime = color[i];
    
  // Figure out how many cycles to wait before the next update, based on timing and refresh rate
  // Gamma correction requires us to use the square of intensity to compute the timing
  u32 howlong = (s_time * s_time) - (nexttime * nexttime);

  // Schedule the next timer interrupt
  TIM14->SR = 0;        // Acknowledge interrupt
  TIM14->ARR = howlong; // Next time to trigger
  TIM14->EGR = TIM_PSCReloadMode_Immediate;

  // If we just changed LEDs, finish changing
  if (255 == s_time)
  {
    if (0 == s_which)
      GPIO_RESET(GPIO_EYERST, PIN_EYERST);
    else
      GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
  }
  s_time = nexttime;    // Next time we call, it will be s_time

  TIM14->CR1 |= TIM_CR1_CEN;  
}
