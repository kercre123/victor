#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
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

      // Map the natural LED order (as shown in hal.h) to the hardware swizzled order
      // See schematic if you care about why the LEDs are swizzled
      static const u32 CHANNEL_MASK = 7;  // Bitmask for all 8 channels
      static const u8 HW_CHANNELS[8] = {5, 1, 2, 0, 6, 7, 3, 4};
      static u32 m_channels[8];   // The actual RGB color values in use

      // Make some nice array handles into macro defined GPIOs
      static const int NUM_EYEnEN = 2;
      static const int EYEnEN_PIN[2]  = {PIN_EYE1nEN, PIN_EYE2nEN};
      static const int EYEnEN_GPIO[2] = {GPIO_EYE1nEN, GPIO_EYE2nEN};

      static const int NUM_COLLOR_CHANNELS = 4;
      static const int RED_GPIO[4] = {GPIO_RED1, GPIO_RED2, GPIO_RED3, GPIO_RED4}; // These must imeediately follow one
      static const int GRN_GPIO[4] = {GPIO_GRN1, GPIO_GRN2, GPIO_GRN3, GPIO_GRN4}; // another for below concatination
      static const int BLU_GPIO[4] = {GPIO_BLU1, GPIO_BLU2, GPIO_BLU3, GPIO_BLU4}; // hack
      static const int RED_PIN[4]  = {PIN_RED1, PIN_RED2, PIN_RED3, PIN_RED4}; // These also need to concatinate
      static const int GRN_PIN[4]  = {PIN_GRN1, PIN_GRN2, PIN_GRN3, PIN_GRN4};
      static const int BLU_PIN[4]  = {PIN_BLU1, PIN_BLU2, PIN_BLU4, PIN_BLU4};


      static const int CH_RED = 2, CH_GREEN = 1, CH_BLUE = 0;

      // Initialize LED head/face light hardware
      void LightsInit()
      {
        int i;

        // Leave the high side pins off until first LED is set
        for (i=0; i<NUM_EYEnEN; ++i)
        {
          GPIO_SET(EYEnEN_GPIO[i], EYEnEN_PIN[i]);
          GPIO_OD(EYEnEN_GPIO[i], EYEnEN_PIN[i]);
          GPIO_OUT(EYEnEN_GPIO[i], EYEnEN_PIN[i]);
        }

        // Initialize all face LED colors to OFF
        // Low side drivers must be open drain to allow voltages > VDD
        // Above arrays concatinate so starting with red and indexing past it
        const int num_color_gpio = NUM_COLLOR_CHANNELS * 3;
        for (i=0; i<num_color_gpio; ++i)
        {
          GPIO_SET(RED_GPIO[i], RED_PIN[i]);
          PIN_NOPULL(RED_GPIO[i], RED_PIN[i]);
          PIN_OD(RED_GPIO[i], RED_PIN[i]);
          PIN_OUT(RED_GPIO[i], RED_PIN[i]);
        }

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
        if (led_id < NUM_LEDS)  // Unsigned, so always >= 0
          m_channels[HW_CHANNELS[led_id]] = color;
      }

      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state)
      {
        // No headlights on Cozmo 3
        return;
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
  /*
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
  */
  // Schedule the next timer interrupt
  TIM14->SR = 0;        // Acknowledge interrupt
  TIM14->ARR = howlong; // Next time to trigger
  TIM14->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off one pulse
}
