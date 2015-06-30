#ifndef HAL_H__
#define HAL_H__

#include "nrf24le1.h"

#include "hal_nrf.h"
#include "hal_clk.h"
#include "hal_delay.h"
#include "portable.h"
//#include "hal_uart.h"

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
#define RADIO_TIMEOUT_MS  2
void InitTimer0();
void InitPRX();
void InitPTX();
void ReceiveData(u8 msTimeout);
void TransmitData();

#endif /* HAL_H__ */