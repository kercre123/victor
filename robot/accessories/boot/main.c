/**
 * Cozmo nRF31 "accessory" (cube/charger) boot loader
 * Author:  Bryan Hood, Nathan Monson
 *
 * main.c - contains startup code and the main OTA loop
 *
 * Accessories are expected to use OTA patches - don't put the main program in here
 * Rebuilding the bootloader BREAKS all patches - YOU MUST update advertise.c "hardware version"
 *
 * The bootloader provides library functions for the patches (see p2/hal.h)
 * BootHAL claims 87 bytes from 0x50..0xA6 - a patch can reclaim that by not using BootHAL
 * BootHAL also claims 8 bytes from 0x8..0xF for the LED/timer ISR
 * See main.c in your patch for full definitions
 */
#include "hal.h"

// Cube timing is synchronized to the robot using a "Sync packet" (aka Connect packet)
// (Why isn't this a struct?  Try it.. you're gonna love this compiler!)
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

// Perform a radio handshake, then interpret and execute the message
// Note:  Tap detect has been moved to "patch2"
void MainExecution()
{
  u8 i, dest;
  u8 idata *src;
  
  RadioHandshake();
  PetWatchdog();
      
  // Set LED values
  if (R2A_BASIC_SETLEDS == _radioPayload[0])
    LedSetValues(LED_START);
  
  // Receive an OTA message and copy the payload into PRAM
  if (R2A_OTA == (_radioPayload[0] & R2A_OTA))
  {
    dest = _radioPayload[0] << 4;
    src = _radioPayload+1;
    i = 16;
    do {
      _pram[dest] = *src;
      dest++;
      src++;
    } while (--i);
    
    // If we just wrote the last byte (packet 0xFF), try to burn it
    if (!dest)
      OTABurn();
  }
}

// Startup
void main()
{ 
  // Startup, init accelerometer, blink LEDs
  PetSlowWatchdog();
  RunTests();
  
  // Proceed to lower-power advertising mode - return with sync packet filled in
  Advertise();
  
  // If valid, start the requested OTA patch - see makesafe/cube.c for more details
  if (*(u8 code *)((_patchStart<<8)|1) == 0x81)
    ((void(code *)(void))((u16)_patchStart<<8))();

  // Enter high power consumption mode - watchdog is the best way to exit
  LedInit();

  // Now answer LED/OTA requests until connection stops
  while (1)
    MainExecution();
}
