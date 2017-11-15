#include "datasheet.h"
#include "lights.h"
#include "irq.h"

#define GPIO_REGISTER(port) (volatile uint16_t*)(GPIO_BASE + (port << 5) + 2)

typedef struct {
  volatile uint16_t* port;
  uint16_t pin;
  int index;
} LedDefinition;

typedef struct {
  volatile uint16_t* port;
  uint16_t pin;
  uint16_t count;
} LedSlot;

static void SWTIM_IRQHandler(void);

static const LedDefinition pin_assignment[] = {
  // Blue
  { GPIO_REGISTER(2), 1 << 1,  2 },
  { GPIO_REGISTER(2), 1 << 7,  5 },
  { GPIO_REGISTER(2), 1 << 5,  8 },
  { GPIO_REGISTER(2), 1 << 4, 11 },

  // Green
  { GPIO_REGISTER(0), 1 << 7,  1 },
  { GPIO_REGISTER(2), 1 << 8,  4 },
  { GPIO_REGISTER(1), 1 << 0,  7 },
  { GPIO_REGISTER(1), 1 << 1, 10 },

  // Red
  { GPIO_REGISTER(2), 1 << 2,  0 },
  { GPIO_REGISTER(2), 1 << 9,  3 },
  { GPIO_REGISTER(2), 1 << 6,  6 },
  { GPIO_REGISTER(2), 1 << 3,  9 },
};

static const uint16_t SET_R0 = (1 << 7);
static const uint16_t SET_R1 = (1 << 1) | (1 << 0);
static const uint16_t SET_R2 = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)
                             | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);

// Timing registers and constants
static const int LED_COUNT = 12;
static const uint16_t TOTAL_TIME = (16000000 / 2 / 200);  // 200hz period at a /2 clock divider
static const int LED_SHIFT = 4;
static const int LIGHT_MINIMUM = 0x20;  // Minimum time for a light pattern

static IrqCallback swtim_irq;
static LedSlot led_slots[LED_COUNT+1];
static LedSlot* led_slot_current = led_slots;
static uint8_t intensity[LED_COUNT] = {
  0xFF, 0x00, 0x00,
  0x00, 0xFF, 0x00,
  0x00, 0x00, 0xFF,
  0xFF, 0xFF, 0xFF
};

static void led_next() {
  SetWord32(TIMER0_CTRL_REG,TIM0_CLK_DIV | TIM0_CLK_SEL | TIM0_CTRL); // 16mhz, no PWM / div

}

static void led_kickoff() {
  uint16_t dark_time = TOTAL_TIME;
  LedSlot* slots = led_slots;

  for (int i = 0; i < LED_COUNT; i++) {
    const LedDefinition* led = &pin_assignment[i];
    int scale = intensity[led->index];
    uint16_t timer = (scale * scale) >> LED_SHIFT;

    if (timer < LIGHT_MINIMUM) continue ;

    dark_time -= timer;

    slots->port = led->port;
    slots->pin = led->pin;
    slots->count = timer;
    slots++;
  }

  // Dummy write
  slots->port = GPIO_REGISTER(2);
  slots->pin = 0;
  slots->count = dark_time;
}

// These are mass pin set values
void hal_led_init(void) {
  hal_led_off();
  GPIO_SET(BOOST_EN);

  // Setup Timer (global, /2 divider (TIM0), fast clock)
  SetWord32(CLK_PER_REG, TMR_ENABLE | 1);

  // Setup Timer0
  SetWord16(TIMER0_RELOAD_N_REG, 0);
  SetWord16(TIMER0_RELOAD_M_REG, 0);
  SetWord16(TIMER0_ON_REG, TOTAL_TIME);

  // Configure the IRQ
  swtim_irq = IRQ_Vectors[SWTIM_IRQn];
  IRQ_Vectors[SWTIM_IRQn] = SWTIM_IRQHandler;
  NVIC_SetPriority(SWTIM_IRQn, 3);
  NVIC_EnableIRQ(SWTIM_IRQn);

  // Start the LED timer
  led_kickoff();
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
  *GPIO_REGISTER(0) = SET_R0;
  *GPIO_REGISTER(1) = SET_R1;
  *GPIO_REGISTER(2) = SET_R2;
}

static void SWTIM_IRQHandler(void) {
  SetWord32(TIMER0_CTRL_REG,TIM0_CLK_DIV | TIM0_CLK_SEL | TIM0_CTRL);
  led_next();
}
