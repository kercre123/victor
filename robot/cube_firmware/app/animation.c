#include <stdint.h>
#include <string.h>

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

static AnimationChannel state[ANIMATION_CHANNELS];

static AnimationFrame animation[MAX_KEYFRAMES];
static AnimationFrame staging[MAX_KEYFRAMES];

extern uint8_t intensity[ANIMATION_CHANNELS * COLOR_CHANNELS];
static uint32_t div_tbl[0x100] = { 0xFFFFFFFF };

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

  for (int i = 1; i < 0x100; i++) {
    div_tbl[i] = 0x1000000 / i;
  }

  for (int i = 0; i < MAX_KEYFRAMES; i++) {
    animation[i].next = &animation[0];
  }

  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    setup_frame(i, &animation[0]);
  }
}

void animation_frames(const FrameCommand* frames) {
  const KeyFrame* source = frames->frames;
  uint8_t index = frames->flags;
  
  for (int i = 0; i < FRAMES_PER_COMMAND; i++, source++) {
    AnimationFrame* target = &staging[index++];

    // Unpack our solid color
    memcpy(target->colors, source->colors, sizeof(target->colors));

    // Animation settings
    target->decay = source->decay;
    target->hold = source->hold;
    target->next = &animation[source->link];
  }
}

void animation_index(const MapCommand* map) {
  memcpy(animation, staging, sizeof(animation));

  for (int c = 0; c < 4; c++) {
    setup_frame(c, &animation[map->initial[c]]);
  }
}

void animation_tick(void) {
  static const int DIVIDER = 6;
  static int overflow = 0;

  if (++overflow < DIVIDER) return ;
  overflow = 0;

  uint8_t* target = intensity;
  
  for (int i = 0; i < ANIMATION_CHANNELS; i++) {
    AnimationChannel* channel = &state[i];
    const AnimationFrame* current = channel->current;

    switch (channel->state) {
    case STATE_DECAY:
      {
        int lerp = channel->index * div_tbl[current->decay];
        
        for (int c = 0; c < COLOR_CHANNELS; c++) {
          *(target++) = ((current->colors[c] * (0x1000000 - lerp)) + (current->next->colors[c] * lerp)) >> 24;
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

