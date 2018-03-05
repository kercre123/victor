#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#pragma anon_unions

#define MAX_KEYFRAMES 16
#define ANIMATION_CHANNELS 4
#define COLOR_CHANNELS 3
#define FRAMES_PER_COMMAND 4

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
  uint16_t data[10];
} BaseCommand;

typedef struct {
  uint16_t color;
  uint8_t hold;
  uint8_t decay;
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
  uint16_t initial;  // Starting indexes for channels 0..3
  uint8_t  frame_map[MAX_KEYFRAMES / 2]; 
} MapCommand;

#endif
