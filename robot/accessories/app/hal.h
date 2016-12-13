// Function prototypes for shared hal functions
#ifndef HAL_H__
#define HAL_H__

// #define DEBUG    // Uncomment this for debug prints on 2m baud UART on P1.0

#include "reg31512.h"
#include "portable.h"
#include "board.h"
#include "messages.h"

// Temporary holding area for radio payloads (inbound and outbound)
extern u8 data _radioIn[HAND_LEN];
extern u8 data _radioOut[HAND_LEN];
#define RXTX_TICKS (((RF_START_US+(1+5+HAND_LEN+2)*8)+29)/30)  // Time spent in radio

// The sync (connect) packet starts at this address so OTA patches can pick it up
#define SyncPkt 0x76
#define SyncLen 10

// Drain the entire accelerometer FIFO into an outbound packet
void AccelRead();
void AccelInit();

// Power down to various degrees
void Sleep();       // Less than 3 RTC ticks or radio
void DeepSleep();   // 3 or more RTC ticks, no radio
void LightLEDs();   // Light up LEDs for the rest of the beat

// Complete a handshake with the robot - hop, broadcast, listen, timeout
void RadioHandshake();
void RadioLegacyStart();

// At 2mbaud, UART-print a 1 character note followed by len bytes of hex data
#ifdef DEBUG
#define DebugPrint ((void (code *) (u8 note, u8 idata *hex, u8 len)) 0x3800)
#else
#define DebugPrint(note, hex, len)
#endif

/**
 * These functions are included from the Pilot bootloader
 * Revise these pointers for the production bootloader!
 * NOTE:  You must collect the pointers from the "Public Symbols" section later in the map file
 */
 
// Read bytes from the radio FIFO
#define RadioRead ((void (code *) (u8 idata *buf, u8 len)) 0x3A2C)

// Send a packet to radio FIFO
#define RadioSend ((void (code *) (u8 idata *buf, u8 len)) 0x3DE4)
  
// Configure the radio with a zero-terminated list of radio registers and values
#define RadioSetup ((void (code *) (u8 code *conf)) 0x39D2)
#define SETUP_TX_ADDRESS 0x395D   // Accessory's private address

// Hop to the selected channel
#define RadioHopTo ((void (code *) (u8 channel)) 0x3DBD)

// Set RGB LED values from _radioPayload[1..12]
#define LED_START 0x80
#define LedSetValues ((void (code *) (void idata *start)) 0x3BD4)
#define LedInit ((void (code *) (void)) 0x3BB3)

// Return the model/type from the advertising packet
#define GetModel ((u8 (code *) (void)) 0x3B76)

#define LightOn ((u8 (code *) (u8 i)) 0x3B87)

// Am I a charger?
#define IsCharger() (!GetModel())

// Pet the watchdog - reset after 0.5 seconds (16384 ticks) by default
#define PetWatchdog() do { WDSV = 64; WDSV = 0; } while(0)

// Handy macros to start/monitor/stop a microsecond timer - uses T2 - not reentrant!
#define TIMER_CONVERT(x) (-(x)*16/24)
#define TimerStart(x) do { TimerStop(); TH2 = TIMER_CONVERT(x)>>8; TL2 = TIMER_CONVERT(x); T2CON = T2_24 | T2_RUN; } while(0)
#define TimerStop() T2I0 = 0
#define TimerMSB() TH2
#define TimerLSB() TL2

#endif /* HAL_H__ */
