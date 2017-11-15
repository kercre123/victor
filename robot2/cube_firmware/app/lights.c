#include "datasheet.h"
#include "lights.h"
#include "irq.h"

static IrqCallback swtim_irq;
static void SWTIM_IRQHandler(void);

volatile struct __CLK_PER_REG clk_per_reg __attribute__((at(CLK_PER_REG)));
volatile struct __TIMER0_CTRL_REG timer0_ctrl_reg __attribute__((at(TIMER0_CTRL_REG)));
volatile struct __TRIPLE_PWM_CTRL_REG triple_pwm_ctrl_reg __attribute__((at(TRIPLE_PWM_CTRL_REG)));

#define NO_PWM            0x0
#define RELOAD_100MS      2250

typedef enum
{
    PWM_MODE_ONE,
    PWM_MODE_CLOCK_DIV_BY_TWO
} PWM_MODE_t;

typedef enum
{
    TIM0_CLK_DIV_BY_10,
    TIM0_CLK_NO_DIV
} TIM0_CLK_DIV_t;

typedef enum
{
    TIM0_CLK_32K,
    TIM0_CLK_FAST
} TIM0_CLK_SEL_t;

typedef enum
{
    TIM0_CTRL_OFF_RESET,
    TIM0_CTRL_RUNNING
} TIM0_CTRL_t;

typedef enum
{
    CLK_PER_REG_TMR_DISABLED,
    CLK_PER_REG_TMR_ENABLED,
} CLK_PER_REG_TMR_ENABLE_t;

typedef enum
{
    CLK_PER_REG_TMR_DIV_1,
    CLK_PER_REG_TMR_DIV_2,
    CLK_PER_REG_TMR_DIV_4,
    CLK_PER_REG_TMR_DIV_8
} CLK_PER_REG_TMR_DIV_t;

typedef enum
{
    HW_CAN_NOT_PAUSE_PWM_2_3_4,
    HW_CAN_PAUSE_PWM_2_3_4
} TRIPLE_PWM_HW_PAUSE_EN_t;

typedef enum
{
    PWM_2_3_4_SW_PAUSE_DISABLED,
    PWM_2_3_4_SW_PAUSE_ENABLED
} TRIPLE_PWM_SW_PAUSE_EN_t;

typedef enum
{
    TRIPLE_PWM_DISABLED,
    TRIPLE_PWM_ENABLED
} TRIPLE_PWM_ENABLE_t;

void hal_led_init(void) {
  hal_led_off();
  GPIO_SET(BOOST_EN);

  swtim_irq = IRQ_Vectors[SWTIM_IRQn];
  IRQ_Vectors[SWTIM_IRQn] = SWTIM_IRQHandler;

  // Setup Timer (global, no divider, fast clock)
  clk_per_reg.BITFLD_TMR_ENABLE = CLK_PER_REG_TMR_ENABLED;
  clk_per_reg.BITFLD_TMR_DIV = CLK_PER_REG_TMR_DIV_1;
  
  // Setup Timer0
  SetWord16(TIMER0_RELOAD_N_REG, NO_PWM);
  SetWord16(TIMER0_RELOAD_M_REG, NO_PWM); 
  timer0_ctrl_reg.BITFLD_PWM_MODE = PWM_MODE_ONE;
  timer0_ctrl_reg.BITFLD_TIM0_CLK_DIV = TIM0_CLK_NO_DIV;
  timer0_ctrl_reg.BITFLD_TIM0_CLK_SEL = TIM0_CLK_FAST;
  NVIC_SetPriority (SWTIM_IRQn, 3);

  SetWord16(TIMER0_ON_REG, RELOAD_100MS);
  NVIC_EnableIRQ(SWTIM_IRQn);
  
  // Start Timer
  timer0_ctrl_reg.BITFLD_TIM0_CTRL = TIM0_CTRL_RUNNING;
}

void hal_led_stop(void) {
  // Stop Timer and reset
  timer0_ctrl_reg.BITFLD_TIM0_CTRL = TIM0_CTRL_OFF_RESET;

  // Teardown timer
  NVIC_DisableIRQ(SWTIM_IRQn);
  clk_per_reg.BITFLD_TMR_ENABLE = CLK_PER_REG_TMR_DISABLED;
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

  if (led >= 12) {
    led = 0;
  }
  countup = 0;
}
