#include "datasheet.h"
#include "lights.h"
#include "irq.h"

#define GPIO_S_REGISTER(port) (volatile uint16_t*)(GPIO_BASE + (port << 5) + 2)
#define GPIO_R_REGISTER(port) (volatile uint16_t*)(GPIO_BASE + (port << 5) + 4)

typedef struct {
  volatile uint16_t* port;
  uint16_t pin;
  uint16_t index;
} LedDefinition;

typedef struct {
  volatile uint16_t* port;
  uint16_t pin;
  uint16_t time;
} LedSlot;

static void SWTIM_IRQHandler(void);
static void led_calculate(void);
static inline void led_off(void);

static const LedDefinition pin_assignment[] = {
  // Blue
  { GPIO_R_REGISTER(2), 1 << 1,  2 },
  { GPIO_R_REGISTER(2), 1 << 7,  5 },
  { GPIO_R_REGISTER(2), 1 << 5,  8 },
  { GPIO_R_REGISTER(2), 1 << 4, 11 },

  // Green
  { GPIO_R_REGISTER(0), 1 << 7,  1 },
  { GPIO_R_REGISTER(2), 1 << 8,  4 },
  { GPIO_R_REGISTER(1), 1 << 0,  7 },
  { GPIO_R_REGISTER(1), 1 << 1, 10 },

  // Red
  { GPIO_R_REGISTER(2), 1 << 2,  0 },
  { GPIO_R_REGISTER(2), 1 << 9,  3 },
  { GPIO_R_REGISTER(2), 1 << 6,  6 },
  { GPIO_R_REGISTER(2), 1 << 3,  9 },
};

static const uint16_t SET_R0 = (1 << 7);
static const uint16_t SET_R1 = (1 << 1) | (1 << 0);
static const uint16_t SET_R2 = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)
                             | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9);

// Timing registers and constants
static const int LED_COUNT = 12;
static const uint16_t TOTAL_TIME = (16000000 / 2 / 200);  // 200hz period at a /2 clock divider
static const int LED_SHIFT = 4;
static const int LIGHT_MINIMUM = 0x40;  // Minimum time for a light pattern

static LedSlot led_slot[LED_COUNT+1];
static LedSlot* led_current;
static volatile uint16_t* led_port;
static uint16_t led_pin;

static IrqCallback swtim_irq;
uint8_t intensity[LED_COUNT];

// These are mass pin set values
void hal_led_init(void) {
  //Init boost regulator control (NOTE: 1V-BAT logic level)
  GPIO_INIT_PIN(BOOST_EN, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_1V );

  // Init LEDs
  GPIO_INIT_PIN(D0, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, OUTPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );

  // Setup Timer (global, /2 divider (TIM0), fast clock)
  SetWord32(CLK_PER_REG, TMR_ENABLE | 1);

  // Setup Timer0
  SetWord16(TIMER0_RELOAD_N_REG, 0);
  SetWord16(TIMER0_RELOAD_M_REG, 0);

  // Configure the IRQ
  swtim_irq = IRQ_Vectors[SWTIM_IRQn];
  IRQ_Vectors[SWTIM_IRQn] = SWTIM_IRQHandler;
  NVIC_SetPriority(SWTIM_IRQn, 3);
  NVIC_EnableIRQ(SWTIM_IRQn);

  // Start the LED timer
  //memset(intensity, 0, sizeof(intensity));

  led_pin = 0;
  led_calculate();

  // Fill in some default values (Start out in dark time)
  SetWord16(TIMER0_ON_REG, TOTAL_TIME);
  SetWord32(TIMER0_CTRL_REG, TIM0_CLK_DIV | TIM0_CLK_SEL | TIM0_CTRL); // 16mhz, no PWM / div
}

void hal_led_stop(void) {
  // Stop Timer and reset
  SetWord32(TIMER0_CTRL_REG, 0);
  SetWord32(CLK_PER_REG, 0);

  // Teardown timer
  NVIC_DisableIRQ(SWTIM_IRQn);
  IRQ_Vectors[SWTIM_IRQn] = swtim_irq;

  led_off();
  GPIO_CLR(BOOST_EN);

  // Disable GPIO
  GPIO_INIT_PIN(D0, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D1, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D2, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D3, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D4, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D5, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D6, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D7, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D8, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D9, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D10, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
  GPIO_INIT_PIN(D11, INPUT, PID_GPIO, 1, GPIO_POWER_RAIL_3V );
}

void hal_led_set(const uint8_t* colors) {
  memcpy(intensity, colors, sizeof(intensity));
}

static inline void led_off(void)
{
  *GPIO_S_REGISTER(0) = SET_R0;
  *GPIO_S_REGISTER(1) = SET_R1;
  *GPIO_S_REGISTER(2) = SET_R2;
}

static void led_calculate(void) {
  const LedDefinition* led = pin_assignment;
  uint16_t dark_time = TOTAL_TIME;
  LedSlot* led_active = led_slot;
  
  led_current = led_slot;

  for (int i = 0; i < LED_COUNT; i++, led++) {
    int ticks = intensity[led->index];
    ticks = (ticks * ticks) >> LED_SHIFT;

    if (ticks < LIGHT_MINIMUM) continue ;
    
    dark_time -= ticks;
    
    led_active->port = led->port;
    led_active->pin = led->pin;
    led_active->time = ticks;
    led_active++;
  }

  // Dark time has no pin
  led_active->pin = 0;
  led_active->time = dark_time;
}

static void SWTIM_IRQHandler(void) {
  led_off();
  if (led_pin) *led_port = led_pin;

  led_port = led_current->port;
  led_pin = led_current->pin;
  SetWord16(TIMER0_ON_REG, led_current->time);

  if (led_current->pin) {
    ++led_current;
  } else {
    led_calculate();
  }
}
