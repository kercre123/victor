#ifndef __RADIO_H
#define __RADIO_H
#include "portable.h"

// Put the radio into a specific test mode
void SetRadioMode(char mode, bool forceupdate = false );

// Process incoming bytes from the radio - must call at least 12,000 times/second
void RadioProcess();

//Read RSSI channel data
void RadioGetRssi( int8_t out_rssi[9] );

#endif
