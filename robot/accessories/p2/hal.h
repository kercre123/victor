// Function prototypes for shared hal functions
#ifndef HAL_H__
#define HAL_H__

#include "reg31512.h"
#include "portable.h"
#include "board.h"

// Temporary holding area for radio payloads (inbound and outbound)
extern u8 data _radioPayload[32];   // Largest packet supported by chip is 32 bytes

// The sync (connect) packet starts at this address so OTA patches can pick it up
#define SyncPkt 0x76
#define SyncLen 10

bit RadioHandshake();
bit RadioBeacon();
void Advertise();

// Write accelerometer setting
void AccelWrite(u8 reg, u8 val);
// Drain the entire accelerometer FIFO into an outbound packet
// Worst case timing is 146 32768Hz ticks (typical is about 50), so schedule carefully!
void AccelRead();

// Init and test accelerometer - returns 1 if okay
bit AccelInit();

// Set RGB LED values from _radioPayload[1..12]
void LedSetValues();

// Start LED ISR - this increases power consumption for connected mode
void LedInit();

// Turn on a single LED (0-15), or 16 for none
void LightOn(u8 i);
#define LightsOff() LightOn(16);

// Flash red, green, blue, IR, then the cube ID
void LedTest();

// Decrypt the contents of PRAM (caller sets which page)
// If the signature is valid, burn it forever into OTP!
void OTABurn();

// Am I a charger?  Written by fixture/cube.cpp (also see advertise.c)
#define IsCharger() (!*(u8 code*)(0x3fe4))

// Pet the watchdog - reset after 0.5 seconds (16384 ticks) by default
#define PetWatchdog() do { WDSV = 64; WDSV = 0; } while(0)
// 2 second version of the watchdog for advertising
#define PetSlowWatchdog() do { WDSV = 255; WDSV = 0; } while(0)

// Handy macros to start/monitor/stop a microsecond timer - uses T2 - not reentrant!
#define TIMER_CONVERT(x) (-x*16/12)
#define TimerStart(x) do { TL2 = TIMER_CONVERT(x); TH2 = TIMER_CONVERT(x)>>8; T2I0 = 1; } while(0)
#define TimerStop() T2I0 = 0
#define TimerMSB() TH2

#endif /* HAL_H__ */
