#ifndef TESTPORT_H
#define TESTPORT_H

#include "hal/portable.h"

// Initialize the test port
void InitTestPort(int baudRate);

// Execute "test" with "param" and expect a reply up to buflen bytes
// Returns the number of bytes received, or throws an error if unsuccessful
int SendCommand(u8 test, s8 param, u8 buflen, u8* buf);
int SendCommandNoReply(u8 test, s8 param); //same as SendCommand, but skip reply wait

// Returns a pointer to a global 5KB buffer
u8* GetGlobalBuffer(void);

#endif
