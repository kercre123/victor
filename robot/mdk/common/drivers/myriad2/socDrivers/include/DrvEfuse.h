#ifndef DRV_EFUSE_H_
#define DRV_EFUSE_H_

#include <mv_types.h>

// warning bitAddress is the same that gets on the eFuse address bus,
// so for example the address of word4.bit3 will be
// 4 | (3 << 5) = 0x64, and the address of FB[2]_A1
// will be 7 << 4 = 0x70. Bits of the same word are not
// at continuous addresses
void DrvEfuseProgramBit(int redundancy, int bitAddress);

// warning bitAddress is the same that gets on the eFuse address bus,
// so for example the address of word4.bit3 will be
// 4 | (3 << 5) = 0x64, and the address of FB[2]_A1
// will be 7 << 4 = 0x70. Bits of the same word are not
// at continuous addresses
void DrvEfuseProgramBits(int redundancy, int n, int bitAddresses[]);

void DrvEfuseARead(u32 *dst, int margin);
void DrvEfuseRRead(u32 *dst, int margin);
void DrvEfusePowerDown();
void DrvEfusePowerUp();

#endif
