// Function prototypes for shared hal functions, defined in bootloader
#ifndef HAL_H__
#define HAL_H__

#include "reg31512.h"
#include "portable.h"
#include "board.h"
#include "messages.h"

// Temporary holding area for radio payloads (inbound and outbound)
extern u8 data _radioPayload[32];   // Largest packet supported by chip is 32 bytes

// The sync (connect) packet starts at this address, so OTA patches can pick it up
#define SyncPkt 0x76
#define SyncLen 10

// Complete a handshake with the robot - hop, broadcast, listen, timeout
bit RadioHandshake();

// Send a beacon to the robot and wait for a reply or timeout
bit RadioBeacon();

// Go into power saving mode and advertise until we get a sync packet
void Advertise();

// Write accelerometer setting
void AccelWrite(u8 reg, u8 val);

// Drain the entire accelerometer FIFO into an outbound packet
// Worst case timing is 146 32768Hz ticks (typical is about 50), so schedule carefully!
void AccelRead();

// Initialize accelerometer, test it (return 1 if okay), then leave it asleep for now
bit AccelInit();

// Set RGB LED values from _radioPayload[1..12]
void LedSetValues();

// Start LED ISR - this increases power consumption for connected mode
void LedInit();

// Turn on a single LED (0-15)
void LightOn(u8 i);
#define LightsOff() LEDDIR=DEFDIR

// Run startup self-tests for fixture, accelerometer, and LEDs
void RunTests();

// Decrypt the contents of PRAM (caller sets which page)
// If the signature is valid, burn it forever into OTP!
void OTABurn();

// Return the model/type from the advertising packet
u8 GetModel();

// Am I a charger?
#define IsCharger() (!GetModel())

// Pet the watchdog - reset after 0.5 seconds (runtime) or 4 seconds (for startup/advertising)
#define PetWatchdog() do { WDSV = 64; WDSV = 0; } while(0)
#define PetSlowWatchdog() do { WDSV = 0; WDSV = 2; } while(0)

// Convert microseconds to a tick count
#define US_TO_TICK(x) (((u32)(x)*1024+15625)/31250) // Ratio is 32768/1000000 (but less overflow)

#endif /* HAL_H__ */
