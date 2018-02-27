#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#pragma anon_unions

#define ANIMATION_CHANNELS 4
#define MAX_KEYFRAMES 4
#define COLOR_CHANNELS 3

enum {
  ANIM_FLAGS_CHANNEL = 0x03,
  ANIM_FLAGS_RESET   = 0x04
};

enum {
  COMMAND_CUBE_LIGHTS
};

typedef uint8_t CubeCommand;

// Inbound format
typedef struct {
  uint16_t color;
  uint8_t attack;
  uint8_t hold;
} KeyFrame;

typedef struct {
  uint8_t flags;
  KeyFrame frame[MAX_KEYFRAMES];
} LightChannel;

typedef struct {
  CubeCommand command;
  union {
    LightChannel lights;
  };
} Payload;

#endif
