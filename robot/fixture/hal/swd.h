#ifndef SWD_H
#define SWD_H

#include "hal/portable.h"

// Reboot the MCU, then load and execute a flash stub
// Stubs are bins (see binaries.h) with initialization code at the front that manage flashing a block at a time
void SWDInitStub(u32 loadaddr, const u8* start, const u8* end);

// Send a file to the stub, one block at a time
void SWDSend(u32 tempaddr, int blocklen, u32 flashaddr, const u8* start, const u8* end);

// Ask the stub to reset
void SWDReset(int resetaddr);

#endif
