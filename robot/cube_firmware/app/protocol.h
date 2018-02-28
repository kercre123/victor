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
  COMMAND_ACCEL_FAILURE = 2
};

typedef uint8_t CubeCommand;

// Inbound format
typedef struct {
  uint16_t color;
  uint8_t hold;
  uint8_t decay;
} KeyFrame;

typedef struct {
  uint16_t initial;  // Starting indexes for channels 0..3
  uint8_t  frame_map[MAX_KEYFRAMES / 2]; 
} FrameMap;

typedef struct {
  CubeCommand command;
  uint8_t flags;
  union {
    KeyFrame frames[FRAMES_PER_COMMAND];
    FrameMap framemap;
  };
} Payload;

#endif
