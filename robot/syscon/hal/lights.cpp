#include <string.h>

#include "nrf.h"
#include "nrf_gpio.h"

#include "timer.h"
#include "lights.h"
#include "backpack.h"
#include "cubes.h"

ControllerLights lightController;

static inline void AlphaBlend(
  LightSet& color, 
  const uint16_t startColor, const uint16_t endColor,
  const uint16_t phase, const int frames)
{
  const uint32_t alpha = (0x10000 * phase) / frames;
  const uint32_t invAlpha = 0x10000 - alpha;

  const LightSet start = { UNPACK_COLORS(startColor) };
  const LightSet end = { UNPACK_COLORS(endColor) };
  
  color.red = (start.red * invAlpha + end.red * alpha) >> 16;
  color.green = (start.green * invAlpha + end.green * alpha) >> 16;
  color.blue = (start.blue * invAlpha + end.blue * alpha) >> 16;
  color.ir = (alpha < 0x8000) ? start.ir : end.ir;
}

static inline bool transition(int& phase, LightMode& mode, const uint16_t frames, const LightMode next) {
  if (phase >= frames) {
    phase -= frames;
    mode = next;
    return true;
  }
  return false;
}

static void CalculateLEDColor(LightValues& light, const uint32_t delta)
{ 
  light.phase += delta;

  // Advance light
  for (;;) {
    // Current keyframe
    const Keyframe* now = &light.keyframes[light.index];
    
    switch (light.mode) {
      case SOLID:
        return ;

      case HOLD:
        if (transition(light.phase, light.mode, now->holdFrames, FADE)) {
          continue ;
        }

        return ;

      case FADE:
        // Next frame
        int next_frame = light.index + 1;
        if (next_frame >= light.frameCount) {
          next_frame = 0;
        }

        // Next keyframe
        const Keyframe* then = &light.keyframes[next_frame];

        // Transition to a hold phase
        if (transition(light.phase, light.mode, now->transitionFrames, HOLD)) {
          light.index = next_frame;
          
          LightSet color = { UNPACK_COLORS(then->color) };
          memcpy(&light.values, &color, sizeof(color));

          continue ;
        }

        AlphaBlend(light.values, now->color, then->color, light.phase, now->transitionFrames);

        return ;
    }
  }
}

void Lights::init() {  
  // Set all our lights to off
  memset(&lightController, 0, sizeof(lightController));
}

int Lights::manage() {
  static unsigned int last_counter = 0;
  static unsigned int accumulator = 0;

  unsigned int counter = GetCounter();
  unsigned int delta = counter - last_counter;
  last_counter = counter;

  accumulator += delta * FRAME_RATE;

  int frame_delta = 0;
  while (accumulator >= TICKS_PER_SECOND) {
    frame_delta++;
    accumulator -= TICKS_PER_SECOND;
  }

  if (frame_delta <= 0) {
    return 0;
  }

  for(int i = 0; i < TOTAL_LIGHTS; ++i)
  {
    CalculateLEDColor(lightController.lights[i], frame_delta);
  }
  
  return frame_delta;
}

void Lights::update(LightValues& light, const LightState* params) {
  if (params->onColor == params->offColor) {
    light.mode = SOLID;
  } else {
    light.mode = HOLD;
    light.index = 0;
    // offset + offFrames so we start in the fade on phase
    light.phase = params->offset + params->offFrames;
    light.frameCount = 2;

    light.keyframes[0].color = params->offColor;
    light.keyframes[0].holdFrames = params->offFrames;
    light.keyframes[0].transitionFrames = params->transitionOnFrames;

    light.keyframes[1].color = params->onColor;
    light.keyframes[1].holdFrames = params->onFrames;
    light.keyframes[1].transitionFrames = params->transitionOffFrames;
  }

  // Preset the color
  LightSet color = { UNPACK_COLORS(params->offColor) };
  memcpy(&light.values, &color, sizeof(color));
}
