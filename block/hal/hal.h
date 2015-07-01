#ifndef HAL_H__
#define HAL_H__

#include "nrf24le1.h"
#include "hal_nrf.h"
#include "hal_clk.h"
#include "hal_delay.h"
#include "portable.h"
//#include "hal_uart.h"

#define BLOCK_ID 0xC0

//#define DO_SIMPLE_LED_TEST
//#define DO_LED_TEST
//#define DO_TAPS_TEST
//#define DO_MISSED_PACKET_TEST
//#define DO_RADIO_LED_TEST

//#define LISTEN_FOREVER
//#define DO_TRANSMITTER_BEHAVIOR

// lights.c
void SetLedValues(char *newValues);
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

// tests.c
void RunTests();

// radio.c
static const u8 RADIO_TIMEOUT_MS = 2;
static const u8 ADDRESS[5] = {BLOCK_ID, 0xC2, 0xC2, 0xC2, 0xC2};
static const u8 TIMER30HZ_L = 0x9C;
static const u8 TIMER30HZ_H = 0xAD;
static const u8 WAKEUP_OFFSET = 0x02; // 0.75 us per tick, 191.25 on H byte

typedef enum eRadioTimerState
{
  radioSleep,
  radioWakeup
};

void InitTimer0();
void ReceiveData(u8 msTimeout);
void TransmitData();

#endif /* HAL_H__ */