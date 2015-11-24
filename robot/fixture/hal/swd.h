#ifndef SWD_H
#define SWD_H

#include <stdint.h>

// Reinitialize SWD hardware
void InitSWD(void);

// Using the shim found in binaries.h, flash the code found in binaries.h, to the given start address in flash
// If serialAddress is non-zero, a unique 32-bit serial number will be placed at that address during the flash
void FlashSWD(const uint8_t* shim, const uint8_t* bin, int address, int serialAddress);

#endif
