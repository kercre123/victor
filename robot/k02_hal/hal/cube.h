#ifndef __CUBE_H
#define __CUBE_H

#define MAX_CUBES 7
#define NUM_BLOCK_LEDS 4

extern AcceleratorPacket g_AccelStatus[MAX_CUBES];
extern uint16_t g_LedStatus[MAX_CUBES][NUM_BLOCK_LEDS];

#endif
