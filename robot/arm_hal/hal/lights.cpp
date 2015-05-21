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
			
      // Cozmo 3 implementation
      GPIO_PIN_SOURCE(EYE1nEN, GPIOC, 14);
      GPIO_PIN_SOURCE(EYE2nEN, GPIOB, 12);

      GPIO_PIN_SOURCE(RED1, GPIOB,  1);
      GPIO_PIN_SOURCE(RED2, GPIOB,  0);
      GPIO_PIN_SOURCE(RED3, GPIOB,  9);
      GPIO_PIN_SOURCE(RED4, GPIOA, 15);
      GPIO_PIN_SOURCE(GRN1, GPIOB, 10);
      GPIO_PIN_SOURCE(GRN2, GPIOA,  7);
      GPIO_PIN_SOURCE(GRN3, GPIOC, 13);
      GPIO_PIN_SOURCE(GRN4, GPIOB,  3);
      GPIO_PIN_SOURCE(BLU1, GPIOB,  2);
      GPIO_PIN_SOURCE(BLU2, GPIOA,  6);
      GPIO_PIN_SOURCE(BLU3, GPIOB,  8);
      GPIO_PIN_SOURCE(BLU4, GPIOB,  4);

      GPIO_PIN_SOURCE(IRLED, GPIOA, 12);

      // Map the natural LED order (as shown in hal.h) to the hardware swizzled order
      // See schematic if you care about why the LEDs are swizzled
      typedef union color_t {
        u32 asColor;
        u8  asLEDs[4];
      } ColorValue;
      
      static const u8 HW_CHANNELS[8] = {2, 3, 1, 0, 6, 7, 5, 4};
      static ColorValue m_channels[8];   // The actual RGB color values in use

      // Make some nice array handles into macro defined GPIOs
      static const int NUM_EYEnEN = 2;
      static GPIO_TypeDef* const EYEnEN_GPIO[2] = {GPIO_EYE1nEN, GPIO_EYE2nEN};
      static const u32 EYEnEN_PIN[2] = {PIN_EYE1nEN, PIN_EYE2nEN};
      static const u8 EYEnEN_SOURCE[2] = {SOURCE_EYE1nEN, SOURCE_EYE2nEN};

      static const int NUM_COLOR_CHANNELS = 4;
      static GPIO_TypeDef* const RED_GPIO[4] = {GPIO_RED1, GPIO_RED2, GPIO_RED3, GPIO_RED4}; // These must imeediately follow one
      static GPIO_TypeDef* const GRN_GPIO[4] = {GPIO_GRN1, GPIO_GRN2, GPIO_GRN3, GPIO_GRN4}; // another for below concatination
      static GPIO_TypeDef* const BLU_GPIO[4] = {GPIO_BLU1, GPIO_BLU2, GPIO_BLU3, GPIO_BLU4}; // hack
      static const u32 RED_PIN[4] = {PIN_RED1, PIN_RED2, PIN_RED3, PIN_RED4}; // These also need to concatinate
      static const u32 GRN_PIN[4] = {PIN_GRN1, PIN_GRN2, PIN_GRN3, PIN_GRN4};
      static const u32 BLU_PIN[4] = {PIN_BLU1, PIN_BLU2, PIN_BLU3, PIN_BLU4};
      static const u8 CLR_SOURCE[12] = {SOURCE_RED1, SOURCE_RED2, SOURCE_RED3, SOURCE_RED4, // Source is only used concatinated so declared as such
                                        SOURCE_GRN1, SOURCE_GRN2, SOURCE_GRN3, SOURCE_GRN4,
                                        SOURCE_BLU1, SOURCE_BLU2, SOURCE_BLU3, SOURCE_BLU4};


      static const int CH_RED = 2, CH_GREEN = 1, CH_BLUE = 0;

      // Initialize LED head/face light hardware
      void LightsInit()
      {
        int i;

        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        
        // Leave the high side pins off until first LED is set
        for (i=0; i<NUM_EYEnEN; ++i)
        {
          GPIO_SET(EYEnEN_GPIO[i], EYEnEN_PIN[i]);
          PIN_OD(EYEnEN_GPIO[i], EYEnEN_SOURCE[i]);
          PIN_OUT(EYEnEN_GPIO[i], EYEnEN_SOURCE[i]);
        }

        // Initialize all face LED colors to OFF
        // Low side drivers must be open drain to allow voltages > VDD
        // Above arrays concatinate so starting with red and indexing past it
        const int num_color_gpio = NUM_COLOR_CHANNELS * 3;
        for (i=0; i<num_color_gpio; ++i)
        {
          GPIO_SET(RED_GPIO[i], RED_PIN[i]);
          PIN_NOPULL(RED_GPIO[i], CLR_SOURCE[i]);
          PIN_OD(RED_GPIO[i], CLR_SOURCE[i]);
          PIN_OUT(RED_GPIO[i], CLR_SOURCE[i]);
        }

        // IR LED is controlled by N-FET so positive polarity unlike everything else
        GPIO_RESET(GPIO_IRLED, PIN_IRLED);
        PIN_PULLDOWN(GPIO_IRLED, SOURCE_IRLED);
        PIN_PP(GPIO_IRLED, SOURCE_IRLED);
        PIN_OUT(GPIO_IRLED, SOURCE_IRLED);

        // Initialize timer to rapidly blink LEDs, simulating dimming
        NVIC_InitTypeDef NVIC_InitStructure;
        TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM11, ENABLE);

        // Set up clock to multiplex the LEDs - must keep above 100Hz to hide flicker
        // 98Hz = 90MHz/2 eyes/(255^2)/(Prescaler+1)
        TIM_TimeBaseStructure.TIM_Prescaler = 6;
        TIM_TimeBaseStructure.TIM_Period = 65535;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(TIM11, &TIM_TimeBaseStructure);

        // Enable timer and interrupts
        TIM_SelectOnePulseMode(TIM11, TIM_OPMode_Single);
        TIM_ITConfig(TIM11, TIM_IT_Update, ENABLE);
        TIM_Cmd(TIM11, ENABLE);
        TIM11->CR1 |= TIM_CR1_URS;  // Prevent spurious interrupt when we touch EGR
        TIM11->EGR = TIM_PSCReloadMode_Immediate;

        // Route interrupt
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannel = TIM1_TRG_COM_TIM11_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_Init(&NVIC_InitStructure);
      }

      
      char const backpackLightLUT[5] = {0, 1, 2, 2, 3};
      // Light up one of the backpack LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color)
      {
        g_dataToBody.backpackColors[ backpackLightLUT[led_id] ] = color;    
      }

      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state)
      {
        if (state)
          GPIO_SET(GPIO_IRLED, PIN_IRLED);
        else
          GPIO_RESET(GPIO_IRLED, PIN_IRLED);
      }
    }
  }
}

// This interrupt runs thousands of times a second to create a steady image on the face LEDs
// It must be high priority - incorrect priorities and disable_irq cause flicker!
// Please keep this code optimized - avoid function calls and loops
extern "C" void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  static ColorValue s_color[4];     // Cache the current color, since mid-cycle changes cause flicker
  static u8  s_which = 0;    // Which LED to light
  static u8  s_now   = 0;      // The current 'time' in the per-LED PWM cycle (0xff = full bright)
  const u8  NUM_COLOR_GPIO = NUM_COLOR_CHANNELS * 3;
  u32 howlong;
  u8 nexttime;
  u8 i;
  
  // If this is the end of the last LED, switch to the next LED
  if (0 == s_now)
  {
    // Turn everything off
    GPIO_SET(  GPIO_EYE2nEN, PIN_EYE2nEN);
    GPIO_SET(  GPIO_EYE1nEN, PIN_EYE1nEN);
    for (i=0; i<NUM_COLOR_GPIO; ++i)
    { // Concatination hack
      GPIO_SET(RED_GPIO[i], RED_PIN[i]);
    }

    if (s_which == 0)
    {
      // Cache the new colors for this eye
      s_color[0] = m_channels[0];
      s_color[1] = m_channels[1];
      s_color[2] = m_channels[2];
      s_color[3] = m_channels[3];
      s_which = 1; // Next time do other eye.
      // Select eye 1
      GPIO_RESET(GPIO_EYE1nEN, PIN_EYE1nEN);
    }
    else
    {
      // Cache the colors for this eye
      s_color[0] = m_channels[4];
      s_color[1] = m_channels[5];
      s_color[2] = m_channels[6];
      s_color[3] = m_channels[7];
      s_which = 0; // Next time do other eye
      // Select eye 2
      GPIO_RESET(GPIO_EYE2nEN, PIN_EYE2nEN);
    }  
      
    s_now = 255;
    
    // Now, we have to wait ~70uS for the other eye's gate to turn off
    TIM11->SR = 0;        // Acknowledge interrupt
    TIM11->ARR = 1000;    // Next time to trigger is 78uS
    TIM11->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off one pulse  
    return;    
  }

  // If the LED is due to turn, turn it on - if not, see if it is next to turn on
  nexttime = 0;
  for (i=0; i<NUM_COLOR_CHANNELS; ++i)
  {
    if (s_now == s_color[i].asLEDs[CH_BLUE])
      GPIO_RESET(BLU_GPIO[i], BLU_PIN[i]);
    else if (s_color[i].asLEDs[CH_BLUE] < s_now)
      nexttime = s_color[i].asLEDs[CH_BLUE];

    if (s_now == s_color[i].asLEDs[CH_GREEN])
      GPIO_RESET(GRN_GPIO[i], GRN_PIN[i]);
    else if (s_color[i].asLEDs[CH_GREEN] < s_now && s_color[i].asLEDs[CH_GREEN] > nexttime)
      nexttime = s_color[i].asLEDs[CH_GREEN];

    if (s_now == s_color[i].asLEDs[CH_RED])
      GPIO_RESET(RED_GPIO[i], RED_PIN[i]);
    else if (s_color[i].asLEDs[CH_RED] < s_now && s_color[i].asLEDs[CH_RED] > nexttime)
      nexttime = s_color[i].asLEDs[CH_RED];
  }
  // Figure out how many cycles to wait before the next color turns on
  // Gamma correction requires us to use the square of intensity here (linear PWM = RGB^2)
  howlong = (s_now * s_now) - (nexttime * nexttime);  // Keep all 16-bits, since 1 is a valid result
  s_now = nexttime;    // Next time we get an interrupt, now will be nexttime!
  
  // Schedule the next timer interrupt
  TIM11->SR = 0;        // Acknowledge interrupt
  TIM11->ARR = howlong; // Next time to trigger
  TIM11->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off one pulse
}
