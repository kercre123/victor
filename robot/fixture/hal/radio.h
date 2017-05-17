#ifndef __RADIO_H
#define __RADIO_H
#include "portable.h"

// Put the radio into a specific test mode
void SetRadioMode(char mode, bool forceupdate = false );

// Process incoming bytes from the radio - must call at least 12,000 times/second
void RadioProcess();

//Read RSSI channel data
void RadioGetRssi( int8_t out_rssi[9] );

//Get the most recent cubescan id/type
//@return true if new data written to out ptrs, false if no new data
bool RadioGetCubeScan(u32 *out_id, u8 *out_type);

//purge the radio rx buffer (uart) & reset cmd parser
void RadioPurgeBuffer(void);

#endif
