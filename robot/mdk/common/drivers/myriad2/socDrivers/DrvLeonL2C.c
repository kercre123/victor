#include <DrvLeonL2C.h>

void DrvLL2CSetMTRR(u32 base, struct LL2CMemoryRange *ranges, u32 nr_ranges)
{
    u32 i;
    assert(nr_ranges <= DrvLL2CGetNumberOfMTRRRegs(base));
    for (i=0;i<nr_ranges;i++)
    {
        assert((ranges[i].address & 0x3ffff) == 0);
        assert((ranges[i].mask & 0xffff0003) == 0);
        u32 reg = (base) + (L2C_LEON_OS_MTRR0 - L2C_LEON_OS_BASE_ADR) + (i << 2);
        u32 val = ranges[i].address & 0xfffc0000;
        val |= (ranges[i].mask >> 16) & 0x0000fffc;
        val |= (ranges[i].access_mode & 3) << 16;
        val |= ((ranges[i].write_protected) ? 1 : 0) << 1;
        val |= 1; // enabled
        SET_REG_WORD(reg, val);
    }
    for (;i<DrvLL2CGetNumberOfMTRRRegs(base);i++)
    {
        u32 reg = (base) + (L2C_LEON_OS_MTRR0 - L2C_LEON_OS_BASE_ADR) + (i << 2);
        SET_REG_WORD(reg, 0); // disable unused
    }
}
