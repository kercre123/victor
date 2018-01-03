#include <stdint.h>

#include "animation.h"

#define FLAGS_CHANNEL 0x03
#define FLAGS_COPY    0x04
#define FLAGS_RESET   0x08

enum {
  STATE_HOLD,
  STATE_HOLD_END,
  STATE_ATTACK_START,
  STATE_HOLD_START,
  STATE_ATTACK_END
};

typedef struct {
  uint8_t r, g, b;
  uint8_t attack;
  uint8_t hold;
} Color;

typedef struct {
  Color start;
  Color end;
} Animation;

typedef struct {
  uint8_t flags;
  Animation target;
} Packet;

void animation_init(void) {
}

void animation_tick(void) {
}

void animation_write(int length, const void* raw) {
  const Packet* packet = (const Packet*) raw;
  
  if (length < sizeof(Packet)) return ;

  // CALCULATE SHIT HERE
}
