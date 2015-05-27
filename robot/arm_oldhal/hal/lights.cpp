#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/spineData.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {			       
      extern volatile GlobalDataToBody g_dataToBody;
      char const backpackLightLUT[5] = {0, 1, 2, 2, 3};
      
      // All colors are on the same GPIO in 2.1, which simplifies the timer code
      #define GPIO_COLORS GPIOH
      GPIO_PIN_SOURCE(RED, GPIO_COLORS, 11);
      GPIO_PIN_SOURCE(GREEN, GPIO_COLORS, 12);
      GPIO_PIN_SOURCE(BLUE, GPIO_COLORS, 8);
      
      GPIO_PIN_SOURCE(EYECLK, GPIOA, 7);
      GPIO_PIN_SOURCE(EYERST, GPIOB, 1);
      
      GPIO_PIN_SOURCE(HEADLIGHT1, GPIOB, 3);
      GPIO_PIN_SOURCE(HEADLIGHT2, GPIOH, 13);
 
      // Map the natural LED order (as shown in hal.h) to the hardware swizzled order
      // See schematic if you care about why the LEDs are swizzled
      static const u32 CHANNEL_MASK = 7;  // Bitmask for all 8 channels
      static const u8 HW_CHANNELS[8] = {5, 1, 2, 0, 6, 7, 3, 4};
      static u32 m_channels[8];   // The actual RGB color values in use
      
      static const int CH_RED = 2, CH_GREEN = 1, CH_BLUE = 0;
      
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
        // 173Hz = 90MHz/8 eyes/(255^2)/(Prescaler+1) - about 1600 CPU cycles/Hz (8-32 IRQs)
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
        TIM14->EGR = TIM_PSCReloadMode_Immediate;

        // Route interrupt
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannel = TIM8_TRG_COM_TIM14_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_Init(&NVIC_InitStructure);      
      }
            
      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color)
      {
        g_dataToBody.backpackColors[ backpackLightLUT[led_id] ] = color;    
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

// This interrupt runs thousands of times a second to create a steady image on the face LEDs
// It must be high priority - incorrect priorities and disable_irq cause flicker!
// Please keep this code optimized - avoid function calls and loops 
extern "C" void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;  
  static u8 s_color[4];     // Cache the current color, since mid-cycle changes cause flicker
  static u8 s_which = 0;    // Which LED to light
  static u8 s_now = 0;      // The current 'time' in the per-LED PWM cycle (0xff = full bright)

  // If this is the end of the last LED, switch to the next LED
  if (0 == s_now)
  {
    // Advance to next LED, or reset to beginning on first LED
    s_which = (s_which + 1) & CHANNEL_MASK;
    if (0 == s_which)
      GPIO_SET(GPIO_EYERST, PIN_EYERST);
    else
      GPIO_SET(GPIO_EYECLK, PIN_EYECLK);   
    
    // Point to the beginning of the next light with everything turned off
    GPIO_SET(GPIO_COLORS, PIN_RED | PIN_GREEN | PIN_BLUE);    
    *((int*)s_color) = m_channels[s_which];   // Cache the new color
    s_now = 255;
  }
    
  // If the LED is due to turn, turn it on - if not, see if it is next to turn on
  u8 nexttime = 0;
  if (s_now == s_color[CH_BLUE])
    GPIO_RESET(GPIO_COLORS, PIN_BLUE);
  else if (s_color[CH_BLUE] < s_now)
    nexttime = s_color[CH_BLUE];
  
  if (s_now == s_color[CH_GREEN])
    GPIO_RESET(GPIO_COLORS, PIN_GREEN);
  else if (s_color[CH_GREEN] < s_now && s_color[CH_GREEN] > nexttime)
    nexttime = s_color[CH_GREEN];
  
  if (s_now == s_color[CH_RED])
    GPIO_RESET(GPIO_COLORS, PIN_RED);
  else if (s_color[CH_RED] < s_now && s_color[CH_RED] > nexttime)
    nexttime = s_color[CH_RED];
    
  // Figure out how many cycles to wait before the next color turns on
  // Gamma correction requires us to use the square of intensity here (linear PWM = RGB^2)
  int howlong = (s_now * s_now) - (nexttime * nexttime);  // Keep all 16-bits, since 1 is a valid result
  s_now = nexttime;    // Next time we get an interrupt, now will be nexttime!

  // Schedule the next timer interrupt
  TIM14->SR = 0;        // Acknowledge interrupt
  TIM14->ARR = howlong; // Next time to trigger
  TIM14->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off one pulse

  // In case we just pulsed EYERST or EYECLK, let them low again
  GPIO_RESET(GPIO_EYERST, PIN_EYERST);
  GPIO_RESET(GPIO_EYECLK, PIN_EYECLK);
}
