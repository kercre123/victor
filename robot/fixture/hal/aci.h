#ifndef ACI_H
#define ACI_H

// Print each received byte while setting up
#define ACI_TRACE
#define MAX_ACI_BYTES                 32

// Initialize the ACI lines for communication with the Nordic 8001
void InitACI(void);

// Return the raw send/receive buffer
extern unsigned char g_acibuf[];
#define GetRadioBuf() (g_acibuf)

// Simultaneously sends any command loaded and receives new command (if any)
// from Nordic radio
// The Nordic may be unavailable for several milliseconds, so you should poll
// NeedAnotherSendReceive() after the first attempt, and try again if true
void SendReceiveRadioBuf(void);

// Return true if the Nordic was not ready in the last SendReceiveRadioBuf
unsigned char NeedAnotherSendReceive(void);

#endif
