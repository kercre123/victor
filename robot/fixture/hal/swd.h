#ifndef SWD_H
#define SWD_H

#include "hal/portable.h"

// Reboot the MCU, then load and execute a flash stub
// Stubs are bins (see binaries.h) with initialization code at the front that manage flashing a block at a time
// Stubs consist of a load address (usually 0x2000000), temp addr (load+0x1000), and status addr (temp+blocksize)
// Typical block sizes are 1KB or 2KB - see the other stubs (in fixture/flash_stubs) for examples
void SWDInitStub(u32 loadaddr, u32 cmdaddr, const u8* start, const u8* end);

// Be nice and stop driving power into turned-off boards
void SWDDeinit(void);

// Send a file to the stub, one block at a time
void SWDSend(u32 tempaddr, int blocklen, u32 flashaddr, const u8* start, const u8* end, u32 serialaddr, u32 serial, bool quickcheck = false);

#endif
