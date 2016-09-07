/**
 * Cozmo nRF31 "accessory" (cube/charger) firmware patch
 * Author:  Bryan Hood, Nathan Monson
 *
 * main.c - contains main execution and message parsing
 *
 * The nRF31 is incredibly cheap - too slow and simple for "traditional, well-factored" code
 * Some tips:  http://www.barrgroup.com/Embedded-Systems/How-To/Optimal-C-Code-8051
 *
 * YOU MUST read the .lst files (assembly output) - Keil C51 USUALLY misunderstands your intention
 * Make sure the .lst shows locals "assigned to register" - limit the local's scope until it does
 * Slim the .lst by BREAKING UP expressions - a=*p; p++; is faster/shorter than a=*p++;
 * USE GLOBAL VARIABLES - avoid passing parameters whenever possible, ESPECIALLY pointers
 * Use u8 as much as possible - avoid signed integers (char/s8), multiplies, divides, and 16/32-bit
 * Compares: Prefer implicit x !x, then --x, then == !=, then &, then < > as a last resort
 * Always use typed pointers (code, idata, pdata) - NEVER use generic pointers (e.g., memcpy())
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

// Internal state tracking radio/accelerometer sync
u8 _hop;        // Current hopping channel (before hopBlackout)
u8 _shakeWait;  // How long until next handshake

#ifdef DEBUG
// Variables used by DebugPrint (if needed)
u8 dp1 _at_ 0x23;
u8 dp2 _at_ 0x24;
#endif

// Sleep until the next beat
void Sleep()
{
  // XXX: Add retention mode sleep to save more battery (future non-ISR-LED)
  while (!_beatCount)
    PWRDWN = STANDBY;
  PWRDWN = 0;
  _beatCount--;   // We should never miss >1 beat, but this catches us up if we do
}

extern u8 _tapTime;   // Hack until we get hopping sync in place
typedef struct {      // Hack LED structure to reduce radio current to ~15mA
  u8 dir, port, value;
} LedValue;
static LedValue idata _leds[1+12+1] _at_ LED_START;

// Perform a radio handshake, then interpret and execute the message
void MainExecution()
{
  Sleep();    // Sleep first, to skip the runt beat at startup

  // Do this first:  If we're ready for a handshake, talk to the robot
  if (!_shakeWait)
  {
    _shakeWait = _shakeBeats;
    if (RadioHandshake())
    {
      PetWatchdog();    // Only if we hear from the robot
     
      DebugPrint('r', _radioIn, HAND_LEN);
      DebugPrint('t', _radioOut, HAND_LEN);
      
      // Process future commands here
    }
    _tapTime++;    // Whether we get a packet or not, increment tap timer  

  // Do anything else but radio
  } else {
    // ISR-LED timing workaround - set LEDs at same moment each time
    if (R2A_BASIC_SETLEDS == _radioIn[0])
    {
      _leds[0].dir = DEFDIR;  // Keep LEDs off for first..
      _leds[0].port = 0;
      _leds[0].value = 32*4;  // ..32 ticks, to reduce radio current draw
      LedSetValues(_leds+1);
      _radioIn[0] = 255;
      _tapTime = _radioIn[13];
    }

    // If we're ready for the accelerometer, drain its FIFO
    if (_accelWait & 0x80) {
      AccelRead();
      _accelWait += _accelBeats;
    }
  }
  
  // Until next time...
  _shakeWait--;
  _accelWait--;
}

// Startup
void main()
{
  // Wait for robot start time while we initialize everything
  EA = 0;
  RTC2CON = 0;
  RTC2CMP0 = _waitLSB;
  RTC2CMP1 = _waitMSB;
  RTC2CON = RTC_COMPARE | RTC_ENABLE;

  // Clear timing variables (there's no static init in patches)
  _beatCount = _hop = _shakeWait = 0;

  // Power up the accelerometer before LEDs - this takes at least 2ms/70 ticks
  AccelInit();
  DebugPrint('s', SyncPkt, ADV_LEN);    // Print the start/sync packet
  RadioSetup(SETUP_TX_ADDRESS);         // Transmit on private address

  // Sleep until robot is ready
  EA = 0;   // XXX: Can clean this up in production cubes
  RFF = WUF = 0;
  PWRDWN = STANDBY;
  WUF = 0;
  RTC2CON = 0;
  EA = 1;

  // XXX legacy:  Old non-hopping robots park on a single channel and sync on first packet
  if (!_hopIndex)
    RadioLegacyStart();   // Listen for first packet, adjust timing to match its arrival
  else
    _shakeWait = 1;       // Skip first/runt beat in LED timer
  
  // Start beat counter - we get our first beat in 3 ticks, then properly spaced thereafter
  LedInit();

  // Keep running until watchdog times us out
  while (1)
    MainExecution();
}
