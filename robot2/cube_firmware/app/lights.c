#include "datasheet.h"
#include "lights.h"
#include "irq.h"

// 40000 total ticks (/2 divider so dark time can run at 200hz)
#define TICKS_PER_FRAME   (8000000 / 200)
#define GPIO_S_REGISTER(port) (volatile uint16_t*)(GPIO_BASE + (port << 5) + 2)
#define GPIO_R_REGISTER(port) (volatile uint16_t*)(GPIO_BASE + (port << 5) + 4)

typedef struct {
  volatile uint16_t* toggle;
  uint16_t pin;
  int index;
} LED_DEF;

static void SWTIM_IRQHandler(void);

static const LED_DEF pin_assignment[] = {
  // Green
  { GPIO_R_REGISTER(2), 1 << 1,  2 },
  { GPIO_R_REGISTER(2), 1 << 7,  5 },
  { GPIO_R_REGISTER(2), 1 << 5,  8 },
  { GPIO_R_REGISTER(2), 1 << 4, 11 },

  // Red
  { GPIO_R_REGISTER(0), 1 << 7,  1 },
  { GPIO_R_REGISTER(2), 1 << 8,  4 },
  { GPIO_R_REGISTER(1), 1 << 0,  7 },
  { GPIO_R_REGISTER(1), 1 << 1, 10 },

  // Blue
  { GPIO_R_REGISTER(2), 1 << 2,  0 },
  { GPIO_R_REGISTER(2), 1 << 9,  3 },
  { GPIO_R_REGISTER(2), 1 << 6,  6 },
  { GPIO_R_REGISTER(2), 1 << 3,  9 },
};

static const uint16_t SET_R0 = (1 << 7);
static const uint16_t SET_R1 = (1 << 1) | (1 << 0);
static const uint16_t SET_R2 = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)
                             | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);

static const int LED_COUNT = 12;

static IrqCallback swtim_irq;
static uint8_t intensity[LED_COUNT];

// These are mass pin set values
void hal_led_init(void) {
  hal_led_off();
  GPIO_SET(BOOST_EN);

  // Setup Timer (global, /2 divider (TIM0), fast clock)
  SetWord32(CLK_PER_REG, TMR_ENABLE | 1);

  // Setup Timer0
  SetWord16(TIMER0_RELOAD_N_REG, 0);
  SetWord16(TIMER0_RELOAD_M_REG, 0);
  SetWord16(TIMER0_ON_REG, TICKS_PER_FRAME / LED_COUNT);

  // Configure the IRQ
  swtim_irq = IRQ_Vectors[SWTIM_IRQn];
  IRQ_Vectors[SWTIM_IRQn] = SWTIM_IRQHandler;
  NVIC_SetPriority(SWTIM_IRQn, 3);
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
  *GPIO_S_REGISTER(0) = SET_R0;
  *GPIO_S_REGISTER(1) = SET_R1;
  *GPIO_S_REGISTER(2) = SET_R2;
}

void hal_led_on(uint16_t n)
{
  hal_led_off();

  *pin_assignment[n].toggle = pin_assignment[n].pin;
}

static void SWTIM_IRQHandler(void) {
  static int led = 0;

  hal_led_on(led++);

  if (led >= LED_COUNT) {
    led = 0;
  }
}
