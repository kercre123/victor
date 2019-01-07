#include <string.h>

#include "common.h"
#include "hardware.h"
#include "timer.h"
#include "lights.h"

typedef void (*void_funct)(void);
static void_funct light_handler;

struct LightChannel {
  void_funct  funct;
  uint8_t     shift_out;
  uint16_t    time;
};

struct LightWorkspace {
  uint16_t  time;
  uint8_t   mask;
};

static const int LIGHT_CHANNELS   = 4;
static const int LIGHT_COLORS     = 3;
static const int LIGHT_MINIMUM    = 128;
static const int LIGHT_SHIFT      = 16;

static const uint16_t WIS_GAMMA_TABLE[] = { 30000, 30000, 30000 };
static const uint16_t VIC_GAMMA_TABLE[] = { 20000, 20000, 50000 };

static const uint8_t REORDER[] = { 
   0,  3,  6,  9, 
   1,  4,  7, 10,
   2,  5,  8, 11
};

static const uint8_t default_value[LIGHT_CHANNELS][LIGHT_COLORS] = {
  { 0xFF, 0xFF, 0xFF },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 }
};

static uint8_t value[LIGHT_CHANNELS * LIGHT_COLORS];

static LightChannel light[LIGHT_CHANNELS * LIGHT_COLORS + 1];
static LightChannel *current_light;
static bool enabled = false;

static void set_lights(LightChannel*& target, const uint16_t* gamma, uint8_t shift_mask);
static void leds_off(void);

void Lights::init(void) {
  TIM17->CR1 = 0;

  // Prep our light structs
  receive((const uint8_t*)&default_value);
  light_handler = leds_off;

  // Configure the timer
  TIM17->CR2 = 0;
  TIM17->PSC = 0;
  TIM17->DIER = TIM_DIER_UIE;

  NVIC_SetPriority(TIM17_IRQn, PRIORITY_LIGHTS);
  NVIC_EnableIRQ(TIM17_IRQn);
}

extern "C" void TIM17_IRQHandler(void) {
  TIM17->SR = 0;
  light_handler();
}

void Lights::receive(const uint8_t* data) {
  const uint8_t* order_table = REORDER;
  
  for (int i = 0; i < LIGHT_CHANNELS * LIGHT_COLORS; i++) {
    value[i] = data[*(order_table++)];
  }
}

void Lights::enable() {
  enabled = true;
  leds_off();
}

void Lights::disable(void) {
  receive((const uint8_t*)default_value);
  enabled = false;
}

static void output_shift_vic(uint8_t data) {
  for (int bit = 0x80; bit > 0; bit >>= 1) {
    if (data & bit) {
      LED_DAT::set();    
    } else {
      LED_DAT::reset();
    }
    LED_CLK_VIC::set(); 
    __asm { nop };
    LED_CLK_VIC::reset();
  }
}

static void output_shift_wis(uint8_t data) {
  for (int bit = 0x80; bit > 0; bit >>= 1) {
    if (data & bit) {
      LED_DAT::set();    
    } else {
      LED_DAT::reset();
    }
    LED_CLK_WIS::set(); 
    __asm { nop };
    LED_CLK_WIS::reset();
  }
}

static void output_shift(uint8_t data) {
  if (IS_WHISKEY) {
    output_shift_wis(data);
  } else {
    output_shift_vic(data);
  }
}

static void leds_off(void) {
  output_shift(*HW_REVISION >= 2 ? 0x1F : 0xE0);
}

static void shifter() {
  output_shift(current_light->shift_out);
  TIM17->ARR = current_light->time;
  light_handler = (++current_light)->funct;
  TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
}

void Lights::tick(void) {
  LightChannel* target = current_light = &light[0];
  
  if (enabled) {
    if (*HW_REVISION >= 2) {     
      set_lights(target, WIS_GAMMA_TABLE, 0xFF);
    } else {
      set_lights(target, VIC_GAMMA_TABLE, 0x00);
    }
  }

  // Finalize the chain with our termination function
  target->funct = leds_off;

  // Kick off our LED chain
  light[0].funct();
}

static void set_lights(LightChannel*& target, const uint16_t* gamma, uint8_t shift_mask) {
  using namespace Lights;

  LightWorkspace light[4];
  LightWorkspace* sorted[4];

  const uint8_t* colors = value;

  for (int ch = 0; ch < LIGHT_COLORS; ch++) {
    uint32_t dark_offset = *(gamma++);

    int clr3;
    switch(ch) {
      case 0:   clr3 = 0x08; break ;
      case 2:   clr3 = 0x10; break ;
      default:  clr3 = 0x00; break ;
    }
    
    int mask = shift_mask ^ (0x4 >> ch) ^ clr3;
    
      // Stub cells
    for (int x = 0; x < LIGHT_CHANNELS; x++) {
      uint32_t intensity = (uint32_t)*(colors++);

      light[x].time = (dark_offset * intensity * intensity) >> LIGHT_SHIFT;

      if (x < 3) {
        light[x].mask = 0x80 >> x;
      } else {
        light[x].mask = clr3;
      }

      sorted[x] = &light[x];
    }

    // Sort light channels
    for (int x = 0; x < LIGHT_CHANNELS; x++) {
      for (int y = x + 1; y < LIGHT_CHANNELS; y++) {
        if (sorted[x]->time <= sorted[y]->time) continue ;
  
        LightWorkspace* t = sorted[y]; 
        sorted[y] = sorted[x]; 
        sorted[x] = t;
      }
    }

    // Merge channels
    int prev_time = 0;
    
    for (int x = 0; x < LIGHT_CHANNELS; x++) {
      int delta = sorted[x]->time - prev_time;

      target->shift_out = mask;
      target->time = delta;
      target->funct = shifter;

      mask ^= sorted[x]->mask;

      if (delta >= LIGHT_MINIMUM) {
        target++;
        prev_time = sorted[x]->time;
      }
    }
  }
}
