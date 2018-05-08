#include <string.h>

#include "common.h"
#include "hardware.h"
#include "timer.h"
#include "lights.h"

static inline void kick_off();

#define wait() __nop();

#include "led_func.h" // Generated with LEDs.py

static void darkness();

static const void_funct function_table[4][8] = {
  { darkness, led_1x2,  led_2x2, led_3x2, led_4x2, led_5x2, led_6x2, led_7x2 },
  { darkness, led_1x1,  led_2x1, led_3x1, led_4x1, led_5x1, led_6x1, led_7x1 },
  { darkness, led_1x0,  led_2x0, led_3x0, led_4x0, led_5x0, led_6x0, led_7x0 },
  { darkness, led_1x3, darkness, led_1x3, led_2x3, led_3x3, led_2x3, led_3x3 }  // Green channel is fixed
};

struct LightChannel {
  void_funct  funct;
  uint16_t    time;
};

struct LightWorkspace {
  uint16_t  time;
  uint8_t   mask;
};

static const int LIGHT_MINIMUM    = 32;
static const int LIGHT_CHANNELS   = 4;
static const int LIGHT_COLORS     = 3;

static const int LIGHT_SHIFT      = 17 + 2; // 2 is to account for prescalar
static const uint8_t DARK_OFFSET  = 200; // 245 = 0% dark

static uint8_t value[LIGHT_CHANNELS][LIGHT_COLORS] = {
  { 0xFF, 0xFF, 0xFF },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00 }
};

static LightChannel light[LIGHT_CHANNELS * LIGHT_COLORS + 1];
static LightChannel *current_light;
static bool disabled;

void Lights::init(void) {
  disabled = false;
  leds_off();
}

void Lights::receive(const uint8_t* data) {
  memcpy(value, data, sizeof(value));
}

void Lights::disable(void) {
  memset(value, 0, sizeof(value));
  disabled = true;
}

static void kick_off(void) {
  TIM14->CCR1 = TIM14->CNT + current_light->time;
  Timer::LightHandler = (++current_light)->funct;
}

static void darkness() {
  leds_off();
  kick_off();
}

void Lights::tick(void) {
  // End of list
  LightChannel* target = current_light = &light[0];

  for (int ch = 0; !disabled && ch < LIGHT_CHANNELS; ch++) {
    // Create light channels
    LightWorkspace light[LIGHT_COLORS];
    LightWorkspace* sorted[LIGHT_COLORS];
    int slots = LIGHT_COLORS;

    // Stub cells
    for (int x = 0; x < LIGHT_COLORS; x++) {
      uint32_t intensity = DARK_OFFSET * (uint32_t)value[ch][x];

      light[x].time = (intensity * intensity) >> LIGHT_SHIFT;
      light[x].mask = 1 << x;
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
    int prev_time = 0;
    int mask = 0x7;
    
    for (int x = 0; x < slots; x++) {
      int delta = sorted[x]->time - prev_time;

      target->funct = function_table[ch][mask];
      target->time = delta;

      mask ^= sorted[x]->mask;

      if (delta >= LIGHT_MINIMUM) target++;
    }
  }

  // Finalize the chain with our termination function
  target->funct = leds_off;

  // Kick off our LED chain
  light[0].funct();
}
