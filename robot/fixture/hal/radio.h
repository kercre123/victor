#ifndef __RADIO_H
#define __RADIO_H
#include "portable.h"

// Put the radio into a specific test mode
void SetRadioMode(char mode, bool forceupdate = false );

// Send a char to the radio to set test mode, without messing with SWD interface and FW checks.
void RadioPutChar(char c);

// Process incoming bytes from the radio - must call at least 12,000 times/second
void RadioProcess();

//Clear RSSI data (mark invalid)
void RadioRssiReset();

//Get latest RSSI data.
//@return true if valid, false if cleared by RadioRssiReset() - waiting for update
bool RadioGetRssi( int8_t out_rssi[9] );

#endif
