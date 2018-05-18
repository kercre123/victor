#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#ifdef __ARMCC_VERSION
  #pragma anon_unions
#endif

#define MAX_KEYFRAMES 256
#define ANIMATION_CHANNELS 4
#define COLOR_CHANNELS 3
#define FRAMES_PER_COMMAND 3

enum {
  COMMAND_LIGHT_INDEX = 0,
  COMMAND_LIGHT_KEYFRAMES = 1,
  COMMAND_ACCEL_DATA = 2,
  COMMAND_ACCEL_FAILURE = 3
};

typedef uint8_t CubeCommand;

// Inbound format
typedef struct {
  CubeCommand command;
  uint8_t flags;
  uint8_t data[20];
} BaseCommand;

typedef struct {
  uint8_t colors[3];
  uint8_t hold;
  uint8_t decay;
  uint8_t link;
} KeyFrame;

typedef struct {
  CubeCommand command;
  uint8_t   flags;
  int16_t   axis[3];
  int16_t   tap_pos;
  int16_t   tap_neg;
  uint8_t   tap_count;
  uint8_t   tap_time;
} TapCommand;

typedef struct {
  CubeCommand command;
  uint8_t flags;
  KeyFrame frames[FRAMES_PER_COMMAND];
} FrameCommand;

typedef struct {
  CubeCommand command;
  uint8_t flags;
  uint8_t initial[ANIMATION_CHANNELS];
} MapCommand;

#endif
