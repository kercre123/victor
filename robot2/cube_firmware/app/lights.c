#include "datasheet.h"
#include "lights.h"
#include "irq.h"

static IrqCallback swtim_irq;
static void SWTIM_IRQHandler(void);

// 8000 total ticks
#define TICKS_PER_FRAME   (1600000 / 200)
#define LED_WRAP          (12)

void hal_led_init(void) {
  hal_led_off();
  GPIO_SET(BOOST_EN);

  swtim_irq = IRQ_Vectors[SWTIM_IRQn];
  IRQ_Vectors[SWTIM_IRQn] = SWTIM_IRQHandler;

  // Setup Timer (global, no divider, fast clock)
  SetWord32(CLK_PER_REG, TMR_ENABLE);
  
  // Setup Timer0
  SetWord16(TIMER0_RELOAD_N_REG, 0);
  SetWord16(TIMER0_RELOAD_M_REG, 0);

  NVIC_SetPriority (SWTIM_IRQn, 3);

  SetWord16(TIMER0_ON_REG, TICKS_PER_FRAME / LED_WRAP);
  NVIC_EnableIRQ(SWTIM_IRQn);
  
  // Start Timer
  SetWord32(TIMER0_CTRL_REG,TIM0_CLK_DIV | TIM0_CLK_SEL | TIM0_CTRL); // 16mhz, no PWM / div
}

void hal_led_stop(void) {
  // Stop Timer and reset
  SetWord32(TIMER0_CTRL_REG, 0);
  SetWord32(CLK_PER_REG, 0);

  // Teardown timer
  NVIC_DisableIRQ(SWTIM_IRQn);
  IRQ_Vectors[SWTIM_IRQn] = swtim_irq;

  hal_led_off();
  GPIO_CLR(BOOST_EN);
}

void hal_led_off(void)
{
  GPIO_SET(D0);
  GPIO_SET(D1);
  GPIO_SET(D2);
  GPIO_SET(D3);
  GPIO_SET(D4);
  GPIO_SET(D5);
  GPIO_SET(D6);
  GPIO_SET(D7);
  GPIO_SET(D8);
  GPIO_SET(D9);
  GPIO_SET(D10);
  GPIO_SET(D11);
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
    case HAL_LED_D4_GRN: GPIO_CLR(D10); break;
    case HAL_LED_D4_BLU: GPIO_CLR(D9); break;

    default: return;
  }
}

static void SWTIM_IRQHandler(void) {
  static int countup = 0;
  static int led = 0;

  if (countup++ < 10) return ;
  hal_led_on(led++);

  if (led >= LED_WRAP) {
    led = 0;
  }
  countup = 0;
}
