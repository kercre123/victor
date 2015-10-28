#ifndef HAL_H__
#define HAL_H__

#include <string.h>
#include "nrf24le1.h"
#include "hal_clk.h"
#include "hal_delay.h"
#include "portable.h"

// XXX: if timeout, don't send data back

// A2-A5 = channel 84
// C2-C5 = channel 82
#define BLOCK_ID 0xC3
#define COMM_CHANNEL 82
#define TAP_THRESH 10

#define DISABLE_PINWHEEL
//#define DO_SIMPLE_LED_TEST
//#define DO_LED_TEST
//#define DO_TAPS_TEST
//#define DO_MISSED_PACKET_TEST
//#define USE_EVAL_BOARD

//#define VERIFY_TRANSMITTER
//#define TIMING_SCOPE_TRIGGER
//#define STREAM_ACCELEROMETER
//#define DEBUG_PAYLOAD
//#define LISTEN_FOREVER
//#define DO_TRANSMITTER_BEHAVIOR
//#define DO_LOSSY_TRANSMITTER


//#define USE_UART
#if defined(VERIFY_TRANSMITTER)
#define LISTEN_FOREVER
#endif

#if defined(STREAM_ACCELEROMETER) || defined(DO_TAPS_TEST) || defined(DEBUG_PAYLOAD) || defined(DO_MISSED_PACKET_TEST) || defined(VERIFY_TRANSMITTER)
#define USE_UART
#endif


// lights.c
void SetLedValues(char *newValues);
void SetLedValuesByDelta();
void SetLedValue(char led, char value);
void InitTimer2();
void LightsOff();
void LightOn(char i);
void StartTimer2();
void StopTimer2();

// acc.c
void InitAcc();
void ReadAcc(u8 *accData);
u8 GetTaps();

#ifdef USE_UART
// uart.c
void InitUart();
void PutDec(char i)reentrant;
char PutChar(char c);
void PutString(char *s);
void PutHex(char h);
#endif

// tests.c
void RunTests();

// radio.c
static const u8 RADIO_TIMEOUT_MS = 3;
static const u8 ADDRESS[5] = {BLOCK_ID, 0xC2, 0xC2, 0xC2, 0xC2};

#ifdef VERIFY_TRANSMITTER
static const u8 TIMER35MS_H = 0x10;
static const u8 TIMER35MS_L = 0x00;
static const u8 WAKEUP_OFFSET = 0x00; // 0.75 us per tick, 192 us on H byte
#else
static const u8 TIMER35MS_H = 0xB6;
static const u8 TIMER35MS_L = 0x4B;
static const u8 WAKEUP_OFFSET = 0x08; // 0.75 us per tick, 192 us on H byte
#endif

static const u8 MAX_MISSED_PACKETS = 3;

typedef enum eRadioTimerState
{
  radioSleep,
  radioWakeup
};

void InitTimer0();
void ReceiveDataSync();
void ReceiveData(u8 msTimeout, bool syncMode);
void TransmitData();

#endif /* HAL_H__ */