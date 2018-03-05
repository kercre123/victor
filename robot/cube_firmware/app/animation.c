#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "animation.h"

// Execution State
enum {
  STATE_STATIC,
  STATE_HOLD,
  STATE_DECAY
};

typedef uint8_t AnimationState;

typedef struct AnimationFrame AnimationFrame;

struct AnimationFrame {
  AnimationFrame* next;
  uint8_t colors[3];
  uint8_t decay;
  uint8_t hold;
};

typedef struct {
  const AnimationFrame* current;
  AnimationState state;
  uint8_t index;
} AnimationChannel;

static AnimationFrame animation[MAX_KEYFRAMES];
static AnimationFrame staging[MAX_KEYFRAMES];
static AnimationChannel state[ANIMATION_CHANNELS];

extern uint8_t intensity[ANIMATION_CHANNELS * COLOR_CHANNELS];

static void setup_frame(int index, const AnimationFrame* frame) {
  AnimationChannel* channel = &state[index];
  channel->current = frame;
  channel->index = 0;

  if (frame->hold) {
    channel->state = STATE_HOLD;
  } else if (frame->decay) {
    channel->state = STATE_DECAY;
  } else {
    channel->state = STATE_STATIC;
  }

  // Initial color
  memcpy(&intensity[index * COLOR_CHANNELS], channel->current->colors, sizeof(channel->current->colors));
}

void animation_init(void) {
  memset(&animation, 0, sizeof(animation));
  memset(&staging, 0, sizeof(staging));

  for (int i = 0; i < MAX_KEYFRAMES; i++) {
    animation[i].next = &animation[0];
  }

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    setup_frame(i, &animation[0]);
  }
}

void animation_frames(const FrameCommand* frames) {
  AnimationFrame* target = &staging[frames->flags * FRAMES_PER_COMMAND];
  const KeyFrame* source = frames->frames;

  
  for (int i = 0; i < FRAMES_PER_COMMAND; i++, target++, source++) {
    // Animation settings
    target->decay = source->decay;
    target->hold = source->hold;

    // Unpack our solid color
    target->colors[0] = (source->color & 0xF800) * 0x21 >> 13;
    target->colors[1] = (source->color & 0x07E0) * 0x41 >>  9;
    target->colors[2] = (source->color & 0x001F) * 0x21 >>  2;
  }
}

void animation_index(const MapCommand* map) {
  AnimationFrame* target = animation;
  memcpy(animation, staging, sizeof(animation));
  
  for (int i = 0; i < MAX_KEYFRAMES / 2; i++) {
    uint8_t index = map->frame_map[i];
    (target++)->next = &animation[index & 0xF];
    (target++)->next = &animation[index >> 4];
  }

  for (int i = 0, c = 0; c < 4; i += 4, c++) {
    setup_frame(c, &animation[(map->initial >> i) & 0xF]);
  }
}

void animation_tick(void) {
  uint8_t* target = intensity;
  
  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    AnimationChannel* channel = &state[i];
    const AnimationFrame* current = channel->current;

    switch (channel->state) {
    case STATE_DECAY:
      {
        int lerp = channel->index * 0x100 / current->decay;
        
        for (int c = 0; c < COLOR_CHANNELS; c++) {
          int value = (current->colors[c] * (0x100 - lerp)) + (current->next->colors[c] * lerp);
          *(target++) = value >> 8;
        }
      }

      if (++channel->index >= current->decay) {
        setup_frame(i, current->next);
      }

      break ;

    case STATE_HOLD:
      if (++channel->index >= current->hold) {
        if (current->decay) {
          channel->state = STATE_DECAY;
          channel->index = 0;
        } else {
          setup_frame(i, current->next);
        }
      }

    case STATE_STATIC:
      target += COLOR_CHANNELS;
      break ;
    }
  }
}

