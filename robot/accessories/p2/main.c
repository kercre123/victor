/**
 * Cozmo nRF31 "accessory" (cube/charger) firmware patch
 * Author:  Bryan Hood, Nathan Monson
 *
 * main.c - contains main execution and message parsing
 *
 * The nRF31 is incredibly cheap - too limited for "traditional, well-factored" code
 * Some tips:  http://www.barrgroup.com/Embedded-Systems/How-To/Optimal-C-Code-8051
 *
 * YOU MUST read the .lst files (assembly output) - Keil C51 USUALLY misunderstands your intention
 * Make sure the .lst shows locals "assigned to register" - limit the local's scope until it does
 * Slim the .lst by BREAKING UP expressions - a=*p; p++; is faster/shorter than a=*p++;
 * Use u8 as much as possible - avoid signed integers (char/s8), multiplies, divides, and 16/32-bit
 * Compares: Prefer implicit x !x, then --x, then == !=, then &, then < > as a last resort
 * Always use typed pointers (code, idata, pdata) - NEVER use generic pointers (e.g., memcpy())
 * Try to avoid passing parameters, ESPECIALLY pointers - use global arrays and constants as often as possible
 * Use #define for constants/one-line functions, "code" for arrays - Keil C51 ignores inline/const
 * Global/static initialization is broken - you must zero/set your globals in main()
 */
#include "hal.h"

// Useful variables from BootHAL for keeping radio/timing sync
u8 _waitLSB     _at_ SyncPkt;  // Little-endian count of 32768Hz ticks until first handshake
u8 _waitMSB     _at_ SyncPkt+1;

u8 _hopIndex    _at_ SyncPkt+2;// Must be 1..52, or 0 to disable hopping (see hopping section)
u8 _hopBlackout _at_ SyncPkt+3;// If not hopping, 3..80 (channel to park on) - if hopping 4..57 - can't be disabled (see hopping section)

u8 _beatTicks   _at_ SyncPkt+4;// Number of 32768Hz ticks per beat (usually 160-164, around 200Hz)
u8 _shakeBeats  _at_ SyncPkt+5;// Number of beats between radio handshakes (usually 7, around 30Hz)
u8 _listenTicks _at_ SyncPkt+6;// Number of 32768Hz ticks the radio listens for a packet (usually 30, enabling +/- 300uS jitter)

u8 _accelBeats  _at_ SyncPkt+7;// Number of beats per accelerometer reading (usually 4 for ~50Hz)
u8 _accelWait   _at_ SyncPkt+8;// Beats until next accelerometer reading (to synchronize cubes)

u8 _patchStart  _at_ SyncPkt+9;// MSB of the start address for the OTA patch to jump into

u8 _beatAdjust  _at_ SyncPkt-1;// Delay (positive) or advance (negative) the next beat by X ticks
u8 _beatCount   _at_ SyncPkt-2;// Incremented when a new beat starts

// Temporary holding area for incoming/outgoing packets
u8 data _radioPayload[32] _at_ 0x50;    // Largest packet supported by chip is 32 bytes

void delay_ms(u8 delay)
{
  do {
    u8 j = 16;
    do {
      u8 i = 248;
      while (--i);  // 4 cycles*248*16 is about 1ms (with overhead)
    } while (--j);
  } while(--delay);
}
void main()
{
  u8 i, j;

  EA = 0;
  
  // Do a little light spinny thing to show we got in
  i = j = 0;
  do {
    do {
      i--;
      LightOn(i&7);
      delay_ms(250);
      LightsOff();
      //delay_ms(1);
    } while (i);
    j += 1;
  } while (j != 12);
  
  // Now just go to sleep forever
  PWRDWN = RETENTION;
}
