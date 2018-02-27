#include <stdint.h>
#include <string.h>

#include "animation.h"

#define ANIMATION_CHANNELS 4
#define MAX_KEYFRAMES 4
#define COLOR_CHANNELS 3

enum {
  ANIM_FLAGS_CHANNEL = 0x03,
  ANIM_FLAGS_RESET   = 0x08
};

// Execution State
enum AnimationState {
  STATE_STATIC,
  STATE_HOLD,
  STATE_ATTACK
};

struct Animation {
  struct Animation* next;
  AnimationState state;

  uint8_t index;

  uint8_t channel[3];
  uint8_t attack;
  uint8_t hold;
};

// Inbound format
struct KeyFrame {
  uint16_t color;
  uint8_t attack;
  uint8_t hold;
};

struct Payload {
  uint16_t flags;
  KeyFrame frame[MAX_KEYFRAMES];
};

static Animation animation[ANIMATION_CHANNELS][MAX_KEYFRAMES];
static Animation staging[ANIMATION_CHANNELS][MAX_KEYFRAMES];
static Animation* current_frame[ANIMATION_CHANNELS];

extern uint8_t intensity[ANIMATION_CHANNELS * COLOR_CHANNELS];

void animation_init(void) {
  memset(&animation, 0, sizeof(animation));
  memset(&staging, 0, sizeof(staging));

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    current_frame[i] = animation[i][0].next = &animation[i][0];
  }
}

void animation_tick(void) {
  uint8_t* target = intensity;

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    Animation* current = current_frame[i];

    switch (current->state) {
      case STATE_STATIC:
        memcpy(target, current->channel, sizeof(current->channel));
        target += sizeof(current->channel);

        continue ;

      case STATE_HOLD:
        memcpy(target, current->channel, sizeof(current->channel));
        target += sizeof(current->channel);

        if (++current->index < current->hold) continue ;

        current->state = current->attack ? STATE_ATTACK : STATE_HOLD;
        break ;

      case STATE_ATTACK:
        int lerp = current->index * 0x100 / current->attack;
        
        for (int c = 0; c < COLOR_CHANNELS; c++) {
          int value = (current->channel[c] * lerp) + (current->next->channel[c] * (0x100 - lerp));
          *(target++) = value >> 8;
        }

        if (++current->index < current->attack) continue ;

        current->state = current->hold ? STATE_HOLD : STATE_ATTACK;
        break ;
    }

    current->index = 0;
    current_frame[i] = current->next;
   }
}

void animation_write(int length, const void* raw) {
  // Ignore invalid
  int lights = (length - sizeof(uint16_t) + sizeof(KeyFrame) - 1) / sizeof(KeyFrame);

  if (lights <= 0) return ;

  // Align payload
  Payload payload;
  memset(&payload, 0, sizeof(Payload));
  memcpy(&payload, raw, length);

  int channel = payload.flags & ANIM_FLAGS_CHANNEL;
  Animation* target = staging[channel];

  for (int i = 0; i < lights; i++, target++) {
    KeyFrame* frame = &payload.frame[i];
    
    // Initial state
    if (frame->attack) {
      target->state = STATE_ATTACK;
    } else if (frame->hold) {
      target->state = STATE_HOLD;
    } else {
      target->state = STATE_STATIC;
    }

    target->index = 0;

    // Animation settings
    target->attack = frame->attack;
    target->hold = frame->hold;

    // Unpack our solid color
    target->channel[0] = (frame->color & 0xF800) * 0x21 >> (2 + 11);
    target->channel[1] = (frame->color & 0x07E0) * 0x41 >> (4 +  5);
    target->channel[2] = (frame->color & 0x001F) * 0x21 >> (2 +  0);

    if (lights == i + 1) {
      target->next = &animation[channel][0];
    } else {
      target->next = &animation[channel][i+1];
    }
  }

  // Copy flag was set, reset the animation controller
  if (payload.flags & ANIM_FLAGS_RESET) {
    for (int i = 0; i < ANIMATION_CHANNELS; i++) {
      memcpy(animation, staging, sizeof(staging));
      current_frame[i] = &animation[i][0];
    }
  }
}
