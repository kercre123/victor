#ifndef DRV_LEON_L2_CACHE_H
#define DRV_LEON_L2_CACHE_H

#include "DrvLeonL2CDefines.h"


static inline u32 DrvLL2CGetMyBase()
{
    switch (swcWhoAmI())
    {
        case PROCESS_LEON_OS:
            return L2C_LEON_OS_BASE_ADR;
        case PROCESS_LEON_RT:
            return L2C_LEON_RT_BASE_ADR; 
        default:
            return 0;
    }
}

static inline void DrvLL2CSetControl(u32 base, u32 value)
{
    SET_REG_WORD((base) + (L2C_LEON_OS_CONTROL - L2C_LEON_OS_BASE_ADR), (value));
}

static inline void DrvLL2COperationOnAll(u32 base, enum DevLL2COperation operation, u32 disable_cache);
static inline void DrvLL2CacheInit()
{
    u32 base = DrvLL2CGetMyBase();
    // Invalidate entire L2Cache before enabling it
    DrvLL2COperationOnAll(base, LL2C_OPERATION_INVALIDATE, /*disable cache?:*/ 0);

    DrvLL2CSetControl(base,
        ( LL2C_CTRL_EN
        | LL2C_CTRL_REPL__LRU
        | LL2C_CTRL_NR_LOCKED_WAYS(0)
        | LL2C_CTRL_WRITE_POLICY_COPY_BACK ));
}


static inline u32 DrvLL2CGetStatus(u32 base)
{
    u32 reg = (base) + (L2C_LEON_OS_STATUS - L2C_LEON_OS_BASE_ADR);
    return GET_REG_WORD_VAL(reg);
}

static inline u32 DrvLL2CGetLineSizeBytes(u32 base)
{
    if ((DrvLL2CGetStatus((base)) >> 24) & 1)
        return 64;
    else
        return 32;
}

static inline u32 DrvLL2CGetProtectionTimingSimulated(u32 base)
{
    return (((DrvLL2CGetStatus((base)) >> 23) & 1));
}

static inline u32 DrvLL2CGetMemoryProtectionImplemented(u32 base)
{
    return (((DrvLL2CGetStatus((base)) >> 22) & 1));
}

static inline u32 DrvLL2CGetNumberOfMTRRRegs(u32 base)
{
    return (((DrvLL2CGetStatus((base)) >> 16) & 0x3f));
}

static inline u32 DrvLL2CGetBackendBusWidthBits(u32 base)
{
    return (128/((DrvLL2CGetStatus((base)) >> 13) & 0x7));
}

static inline u32 DrvLL2CGetSetSizeKiB(u32 base)
{
    return (((DrvLL2CGetStatus((base)) >> 2) & 0x7ff));
}

static inline u32 DrvLL2CGetMultiWayConfiguration(u32 base)
{
    return (((DrvLL2CGetStatus((base)) >> 0) & 0x3));
}

static inline u32 DrvLL2CGetNumberOfWays(u32 base)
{
    return (DrvLL2CGetMultiWayConfiguration((base)) + 1);
}

static inline void DrvLL2COperationOnAddress(u32 base, enum DevLL2COperation operation, u32 disable_cache, u32 address)
{
    u32 reg = (base) + (L2C_LEON_OS_FLUSH_ADR - L2C_LEON_OS_BASE_ADR);
    u32 val = ((address) & 0xfffffe00) |
              (((disable_cache) & 1) << 3) |
              ((operation) & 3);
    SET_REG_WORD(reg, val);
}

static inline void DrvLL2COperationOnAll(u32 base, enum DevLL2COperation operation, u32 disable_cache)
{
    u32 reg = (base) + (L2C_LEON_OS_FLUSH_ADR - L2C_LEON_OS_BASE_ADR);
    u32 val = (((disable_cache) & 1) << 3) |
               ((operation) & 3) |
                (1 << 2);
    SET_REG_WORD(reg, val);
}

static inline void DrvLL2COperationOnWayAndLine(u32 base, enum DevLL2COperation operation, u32 disable_cache, u32 way, u32 line_index)
{
    u32 reg = (base) + (L2C_LEON_OS_FLUSH_INDEX - L2C_LEON_OS_BASE_ADR);
    u32 val = (((disable_cache) & 1) << 3) |
               ((operation) & 3) |
              (((way) & 3) << 4) |
              (((line_index) & 0xffff)<<16);
    SET_REG_WORD(reg, val);
}

static inline void DrvLL2COperationOnWayAndTag(
                    u32 base,
                    enum DevLL2CWayAndTagOperation operation,
                    u32 disable_cache,
                    u32 way,
                    u32 tag, ///< is MSB-aligned
                    u32 fetch,
                    u32 valid,
                    u32 dirty)
{
    tag &= 0xfffffc00;
    valid = valid ? 1 : 0;
    dirty = dirty ? 1 : 0;
    fetch = fetch ? 1 : 0;
    u32 reg = (base) + (L2C_LEON_OS_FLUSH_INDEX - L2C_LEON_OS_BASE_ADR);
    u32 val = (((disable_cache) & 1) << 3) |
               ((operation) & 3) |
              (((way) & 3) << 4) |
                (tag) |
               ((fetch) << 9) |
               ((valid) << 8) |
               ((dirty) << 7) |
                (1 << 2); // way-flush
    SET_REG_WORD(reg, val);
}

static inline u32 DrvLL2CDiagnosticGetTag(u32 base, u32 way, u32 line)
{
    return GET_REG_WORD_VAL(base + 0x80000 + line * 0x20 + way * 4);
}

static inline void DrvLL2CDiagnosticSetTag(u32 base, u32 way, u32 line, u32 tagValue)
{
    SET_REG_WORD(base + 0x80000 + line * 0x20 + way * 4, tagValue);
}

static inline void *DrvLL2CDiagnosticGetLinePointer(u32 base, u32 way, u32 line)
{
    return (void *)(base + 0x200000 + way * 0x80000 + line * DrvLL2CGetLineSizeBytes(base));
}

static inline enum LL2CErrorType DrvLL2CGetErrorType(u32 base)
{
    u32 reg = (base) + (L2C_LEON_OS_ERR_STAT_CTL - L2C_LEON_OS_BASE_ADR);
    u32 val = GET_REG_WORD_VAL(reg);
    if (!(val & (1 << 20))) return LL2C_ERROR_NO_ERROR;
    return (val >> 24) & 7;
}

static inline u32 DrvLL2CGetErrorAddress(u32 base)
{
    u32 reg = (base) + (L2C_LEON_OS_ERR_ADDR - L2C_LEON_OS_BASE_ADR);
    return GET_REG_WORD_VAL(reg);
}

void DrvLL2CSetMTRR(u32 base, struct LL2CMemoryRange *ranges, u32 nr_ranges);

#endif
