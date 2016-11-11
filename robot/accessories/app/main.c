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
u8 _jitterTicks _at_ SyncPkt+6;// Number of 32768Hz ticks of radio jitter (usually 5, +/- 150uS on RX -and- TX)

u8 _accelBeats  _at_ SyncPkt+7;// Number of beats per accelerometer reading (usually 4 for ~50Hz)
u8 _accelWait   _at_ SyncPkt+8;// Beats until next accelerometer reading (to synchronize cubes)

u8 _patchStart  _at_ SyncPkt+9;// MSB of the start address for the OTA patch to jump into

u8 _beatAdjust  _at_ SyncPkt-1;// Delay (positive) or advance (negative) the next beat by X ticks
u8 _beatCount   _at_ SyncPkt-2;// Incremented when a new beat starts

// Internal state tracking radio/accelerometer sync
u8 _hop;        // Current hopping channel (before hopBlackout)
u8 _shakeWait;  // How long until next handshake
u8 _nextBeat;   // How long before next beat

#ifdef DEBUG
// Variables used by DebugPrint (if needed)
u8 dp1 _at_ 0x23;
u8 dp2 _at_ 0x24;
#endif

extern u8 _tapTime;   // Hack until we get hopping sync in place
extern idata u8 _leds[];

/* Perform a radio handshake, then interpret and execute the message
 * WATCH OUT FOR "TIMING" COMMENTS BELOW!
 * To save on battery, spend more time in DeepSleep() and less time in radio/etc.
 * The overall timing is controlled by RTC2 and runs _beatTicks per loop (usually about 200Hz).
 * Each loop is split into CPU/radio time (RXTX_TICKS+jitterTicks)*2, and LED time (the remainder).
 * Radio does one RX (including jitter), plus one TX (including turnaround delay).
 */ 
void MainExecution()
{
  // Do this first:  If we're ready for a handshake, talk to the robot
  if (!_shakeWait)
  {
    _tapTime++;
    _shakeWait = _shakeBeats;
    
    // TIMING:  Listen timeout is in RTC2, set by main()/host on 1st iteration, then end of loop below
    RadioHandshake();
    Sleep();
    
  // Do CPU tasks on slices where there's no radio
  } else {
    RTC2CMP0 += RXTX_TICKS;   // TIMING: CPU gets exactly RXTX_TICKS*2+_jitterTicks*2
    
    // If we're ready for the accelerometer, drain its FIFO
    if (_accelWait & 0x80)
    {
      AccelRead();  // Takes about 200uS (we have 660-990uS depending on listenTicks)
      _accelWait += _accelBeats;
    }
    else if (R2A_BASIC_SETLEDS == _radioIn[0])
    {
      // Set up LEDs if they've changed
      LedSetValues(_leds);
      _radioIn[0] = 255;
      _tapTime = _radioIn[13];

      DebugPrint('r', _radioIn, HAND_LEN);
      DebugPrint('t', _radioOut, HAND_LEN);
      
      // Process future commands here
    }
    
    DeepSleep();
  }
  
  // Light the LEDs for the remainder of the cycle
  LightLEDs();
  
  // Start the next CPU/radio cycle time
  RTC2CMP0 = RXTX_TICKS + _jitterTicks*2 - 1;
  _shakeWait--;
  _accelWait--;
}

// Startup
void main()
{
  u8 i;
  
  // Init - rely on existing setup (WUCON) from bootloader
  CLKCTRL = XOSC_ON;        // Leave XOSC on in register retention mode (to feed RC clk)
  CLKLFCTRL = CLKLF_XOSC;   // Generate RC clk from XOSC for accuracy
  RTC2PCO = 1;              // Prestart in 1 cycle (w/XOSC RC) - XXX: Manual says should be 2?
  ADCCON3 = ADC_8BIT;       // 8-bit ADC (for battery test)

  // Start counting down the listen timeout while we initialize everything
  RTC2CMP0 = _waitLSB;
  RTC2CMP1 = _waitMSB;
  RTC2CON = RTC_COMPARE_RESET | RTC_ENABLE;

  // Power up the accelerometer before LEDs - this takes at least 2ms/70 ticks
  AccelInit();
  DebugPrint('s', SyncPkt, ADV_LEN);    // Print the start/sync packet
  RadioSetup(SETUP_TX_ADDRESS);         // Transmit on private address
  
  // Initialize variables (there's no static init in patches)
  _shakeWait = _hop = 0;  
  _beatTicks -= (RXTX_TICKS*2 + _jitterTicks*2);
  _nextBeat = _beatTicks;
  
  for (i = 0; i < HAND_LEN; i++)        
    _radioIn[i] = 0;
  LedSetValues(_leds);                  // Put LEDs in sane state
  
  // Keep running until watchdog times us out
  while (1)
    MainExecution();
}
