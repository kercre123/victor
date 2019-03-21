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
static const int WIS_LIGHT_SHIFT  = 16;
static const int VIC_LIGHT_SHIFT  = 17;

static const uint16_t WIS_GAMMA_TABLE[] = { 30000, 30000, 30000 };
static const uint16_t VIC_DARK_OFFSET   = 220 * 220; // 245 = 0% dark

static const uint8_t default_value[LIGHT_CHANNELS][LIGHT_COLORS] = {
  { 0xFF, 0xFF, 0xFF },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 }
};

static uint8_t value[LIGHT_CHANNELS][LIGHT_COLORS];

static LightChannel light[LIGHT_CHANNELS * LIGHT_COLORS + 1];
static LightChannel *current_light;
static bool enabled = false;

static void victor_lights(LightChannel*& target);
static void whiskey_lights(LightChannel*& target);
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
  memcpy(value, data, sizeof(value));
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
      LED_DAT::reset();
    } else {
      LED_DAT::set();
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
  output_shift(0xE0);
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
    if (IS_WHISKEY) {
      whiskey_lights(target);
    } else {
      victor_lights(target);
    }
  }

  // Finalize the chain with our termination function
  target->funct = leds_off;

  // Kick off our LED chain
  light[0].funct();
}

static void whiskey_lights(LightChannel*& target) {
  using namespace Lights;

  const uint16_t* gamma = WIS_GAMMA_TABLE;
  
  LightWorkspace light[4];
  LightWorkspace* sorted[4];

  for (int clr = 0; clr < LIGHT_COLORS; clr++) {
    uint32_t dark_offset = *(gamma++);

    int clr3;

    switch(clr) {
      case 2:   clr3 = 0x10; break ;
      default:  clr3 = 0x00; break ;
      case 0:   clr3 = 0x08; break ;
    }
    
    // Stub cells
    for (int ch = 0; ch < LIGHT_CHANNELS; ch++) {
      uint32_t intensity = (uint32_t)value[ch][clr];

      light[ch].time = (dark_offset * intensity * intensity) >> WIS_LIGHT_SHIFT;

      if (ch < 3) {
        light[ch].mask = 0x80 >> ch;
      } else {
        light[ch].mask = clr3;
      }

      sorted[ch] = &light[ch];
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
    int mask = (0x1 << clr) ^ clr3;
    int prev_time = 0;

    for (int ch = 0; ch < LIGHT_CHANNELS; ch++) {
      int delta = sorted[ch]->time - prev_time;

      target->shift_out = mask;
      target->time = delta;
      target->funct = shifter;

      mask ^= sorted[ch]->mask;

      if (delta >= LIGHT_MINIMUM) {
        target++;
        prev_time = sorted[ch]->time;
      }
    }
  }
}

static void victor_lights(LightChannel*& target) {
  for (int ch = 0; ch < LIGHT_CHANNELS; ch++) {
    // Create light channels
    LightWorkspace light[LIGHT_COLORS];
    LightWorkspace* sorted[LIGHT_COLORS];

    // Stub cells
    for (int x = 0; x < LIGHT_COLORS; x++) {
      uint32_t intensity = (uint32_t)value[ch][x];

      if (ch != 3) {
        light[x].mask = 0x01 << x;
      } else if (x == 0) {
        light[x].mask = 0x10;
      } else if (x == 2) {
        light[x].mask = 0x08;
      } else {
        light[x].mask = 0;
        intensity = 0;
      }

      light[x].time = (VIC_DARK_OFFSET * intensity * intensity) >> VIC_LIGHT_SHIFT;

      sorted[x] = &light[x];
    }

    // Sort light channels
    for (int x = 0; x < LIGHT_COLORS - 1; x++) {
      for (int y = x + 1; y < LIGHT_COLORS; y++) {
        if (sorted[x]->time <= sorted[y]->time) continue ;
  
        LightWorkspace* t = sorted[y]; 
        sorted[y] = sorted[x]; 
        sorted[x] = t;
      }
    }

    // Merge channels
    int mask = (ch != 3) ? (0xE7 ^ (0x80 >> ch)) : 0xE0;
    int prev_time = 0;
    
    for (int x = 0; x < LIGHT_COLORS; x++) {
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

