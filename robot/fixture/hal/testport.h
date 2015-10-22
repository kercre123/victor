#ifndef TESTPORT_H
#define TESTPORT_H

#include "hal/portable.h"

// Initialize the test port
void InitTestPort(int baudRate);

// Enable the test port
void TestEnable(void);

// Disable the test port
void TestDisable(void);

// Clear the test port outgoing data buffer
void TestClear(void);

// Send a character over the test port
void TestPutChar(u8 c);

// Send a string over the test port
void TestPutString(char *s);

// Receive a byte from the test port, blocking until it arrives
int TestGetChar(void);

// Receive a byte from the test port, or -1 if no byte is available
int TestGetCharNoWait(void);

// Receive a byte from the test port, blocking until it arrives or there is a timeout
int TestGetCharWait(int timeout);

// Switch the test port between transmit and receive modes
void TestEnableTx(void);
void TestEnableRx(void);

// Wait until the transmission complete flag is set in the status register
void TestWaitForTransmissionComplete(void);

// Send a buffered command with data from Put methods
// Returns 0 on success, non-zero on failure
error_t SendCommand(u8* receiveBuffer, u32 receiveLength);

// Send a buffer that doesn't expect any receiving
error_t SendBuffer(u8 command, u8* buffer, u32 length);

// Buffer data to be sent for commands
void Put8(u8 value);
void Put16(u16 value);
void Put32(u32 value);

// Receive data over the test port over a predefined timeout
// Returns 0 on success, non-zero on failure
u8 Receive8(u8* value);
u8 Receive16(u16* value);
u8 Receive32(u32* value);

// Safely reconstruct 16-bit and 32-bit values from a data buffer
u16 Construct16(u8* data);
u32 Construct32(u8* data);

// Returns a pointer to a global 5KB buffer
u8* GetGlobalBuffer(void);

#endif
