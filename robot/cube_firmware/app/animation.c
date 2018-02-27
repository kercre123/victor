#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "animation.h"

// Execution State
enum {
  STATE_STATIC,
  STATE_HOLD,
  STATE_ATTACK
};

typedef uint8_t AnimationState;

typedef struct Animation Animation;
struct Animation {
  Animation* next;
  AnimationState state;

  uint8_t index;

  uint8_t channel[3];
  uint8_t attack;
  uint8_t hold;
};

static Animation animation[MAX_KEYFRAMES];
static Animation staging[MAX_KEYFRAMES];
static Animation* current_frame[ANIMATION_CHANNELS];

extern uint8_t intensity[ANIMATION_CHANNELS * COLOR_CHANNELS];

void animation_init(void) {
  memset(&animation, 0, sizeof(animation));
  memset(&staging, 0, sizeof(staging));

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    current_frame[i] =
    animation[i].next = 
    staging[i].next = &animation[0];
  }
}

void animation_tick(void) {
  uint8_t* target = intensity;

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    Animation* current = current_frame[i];

    switch (current->state) {
    case STATE_ATTACK:
      {
        int lerp = current->index * 0x100 / current->attack;
        
        for (int c = 0; c < COLOR_CHANNELS; c++) {
          int value = (current->channel[c] * (0x100 - lerp)) + (current->next->channel[c] * lerp);
          *(target++) = value >> 8;
        }
      }

      if (++current->index >= current->attack) {
        current->state = current->hold ? STATE_HOLD : STATE_ATTACK;
        current_frame[i] = current->next;
        current->index = 0;
      }

      break ;
    case STATE_HOLD:
      memcpy(target, current->channel, sizeof(current->channel));
      target += sizeof(current->channel);

      if (++current->index >= current->hold) {
        current->state = current->attack ? STATE_ATTACK : STATE_HOLD;
        current->index = 0;
      }

      break ;

    case STATE_STATIC:
      memcpy(target, current->channel, sizeof(current->channel));
      target += sizeof(current->channel);

      break ;
    }
  }
}

void animation_frames(int channel, const KeyFrame* frames) {
  Animation* target = &staging[channel * FRAMES_PER_COMMAND];

  for (int i = 0; i < FRAMES_PER_COMMAND; i++, target++) {
    // Initial state
    if (frames->hold) {
      target->state = STATE_HOLD;
    } else if (frames->attack) {
      target->state = STATE_ATTACK;
    } else {
      target->state = STATE_STATIC;
    }

    target->index = 0;

    // Animation settings
    target->attack = frames->attack;
    target->hold = frames->hold;

    // Unpack our solid color
    target->channel[0] = (frames->color & 0xF800) * 0x21 >> (2 + 11);
    target->channel[1] = (frames->color & 0x07E0) * 0x41 >> (4 +  5);
    target->channel[2] = (frames->color & 0x001F) * 0x21 >> (2 +  0);
  }
}

void animation_index(const FrameMap* map) {
  memcpy(animation, staging, sizeof(staging));

  for (int i = 0, c = 0; c < 4; i += 4, c++) {
    current_frame[i] = &animation[(map->initial >> i) & 0xF];
  }

  for (int i = 0, kf = 0; kf < MAX_KEYFRAMES; i++, kf += 2) {
    uint8_t index = map->frame_map[i];
    animation[kf+0].next = &animation[index & 0xF];
    animation[kf+1].next = &animation[index >> 4];
  }
}
