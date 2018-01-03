#ifndef _REG_EMI0_H_
#define _REG_EMI0_H_

#include <stdint.h>
#include "_reg_emi0.h"
#include "compiler.h"
#include "arch.h"
#include "reg_access.h"

#define REG_EMI0_COUNT 4

#define REG_EMI0_DECODING_MASK 0x0000000F

/**
 * @brief EXTMEMCNTL0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  11:10             IF_TYPE0   0x0
 *     09            SETUP_WR0   0
 *     08             HOLD_WR0   0
 *  07:05               WS_WR0   0x0
 *     04            SETUP_RD0   0
 *     03             HOLD_RD0   0
 *  02:00               WS_RD0   0x0
 * </pre>
 */
#define EMI0_EXTMEMCNTL0_ADDR   0x1000B000
#define EMI0_EXTMEMCNTL0_OFFSET 0x00000000
#define EMI0_EXTMEMCNTL0_INDEX  0x00000000
#define EMI0_EXTMEMCNTL0_RESET  0x00000000
#define EMI0_EXTMEMCNTL0_COUNT  2

__INLINE uint32_t emi0_extmemcntl0_get(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    return REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
}

__INLINE void emi0_extmemcntl0_set(int reg_idx, uint32_t value)
{
    ASSERT_ERR(reg_idx <= 1);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, value);
}

// field definitions
#define EMI0_IF_TYPE0_MASK    ((uint32_t)0x00000C00)
#define EMI0_IF_TYPE0_LSB     10
#define EMI0_IF_TYPE0_WIDTH   ((uint32_t)0x00000002)
#define EMI0_SETUP_WR0_BIT    ((uint32_t)0x00000200)
#define EMI0_SETUP_WR0_POS    9
#define EMI0_HOLD_WR0_BIT     ((uint32_t)0x00000100)
#define EMI0_HOLD_WR0_POS     8
#define EMI0_WS_WR0_MASK      ((uint32_t)0x000000E0)
#define EMI0_WS_WR0_LSB       5
#define EMI0_WS_WR0_WIDTH     ((uint32_t)0x00000003)
#define EMI0_SETUP_RD0_BIT    ((uint32_t)0x00000010)
#define EMI0_SETUP_RD0_POS    4
#define EMI0_HOLD_RD0_BIT     ((uint32_t)0x00000008)
#define EMI0_HOLD_RD0_POS     3
#define EMI0_WS_RD0_MASK      ((uint32_t)0x00000007)
#define EMI0_WS_RD0_LSB       0
#define EMI0_WS_RD0_WIDTH     ((uint32_t)0x00000003)

#define EMI0_IF_TYPE0_RST     0x0
#define EMI0_SETUP_WR0_RST    0x0
#define EMI0_HOLD_WR0_RST     0x0
#define EMI0_WS_WR0_RST       0x0
#define EMI0_SETUP_RD0_RST    0x0
#define EMI0_HOLD_RD0_RST     0x0
#define EMI0_WS_RD0_RST       0x0

__INLINE void emi0_extmemcntl0_pack(int reg_idx, uint8_t iftype0, uint8_t setupwr0, uint8_t holdwr0, uint8_t wswr0, uint8_t setuprd0, uint8_t holdrd0, uint8_t wsrd0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)iftype0 << 10) & ~((uint32_t)0x00000C00)) == 0);
    ASSERT_ERR((((uint32_t)setupwr0 << 9) & ~((uint32_t)0x00000200)) == 0);
    ASSERT_ERR((((uint32_t)holdwr0 << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)wswr0 << 5) & ~((uint32_t)0x000000E0)) == 0);
    ASSERT_ERR((((uint32_t)setuprd0 << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)holdrd0 << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)wsrd0 << 0) & ~((uint32_t)0x00000007)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4,  ((uint32_t)iftype0 << 10) | ((uint32_t)setupwr0 << 9) | ((uint32_t)holdwr0 << 8) | ((uint32_t)wswr0 << 5) | ((uint32_t)setuprd0 << 4) | ((uint32_t)holdrd0 << 3) | ((uint32_t)wsrd0 << 0));
}

__INLINE void emi0_extmemcntl0_unpack(int reg_idx, uint8_t* iftype0, uint8_t* setupwr0, uint8_t* holdwr0, uint8_t* wswr0, uint8_t* setuprd0, uint8_t* holdrd0, uint8_t* wsrd0)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);

    *iftype0 = (localVal & ((uint32_t)0x00000C00)) >> 10;
    *setupwr0 = (localVal & ((uint32_t)0x00000200)) >> 9;
    *holdwr0 = (localVal & ((uint32_t)0x00000100)) >> 8;
    *wswr0 = (localVal & ((uint32_t)0x000000E0)) >> 5;
    *setuprd0 = (localVal & ((uint32_t)0x00000010)) >> 4;
    *holdrd0 = (localVal & ((uint32_t)0x00000008)) >> 3;
    *wsrd0 = (localVal & ((uint32_t)0x00000007)) >> 0;
}

__INLINE uint8_t emi0_if_type0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000C00)) >> 10);
}

__INLINE void emi0_if_type0_setf(int reg_idx, uint8_t iftype0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)iftype0 << 10) & ~((uint32_t)0x00000C00)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000C00)) | ((uint32_t)iftype0 << 10));
}

__INLINE uint8_t emi0_setup_wr0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000200)) >> 9);
}

__INLINE void emi0_setup_wr0_setf(int reg_idx, uint8_t setupwr0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)setupwr0 << 9) & ~((uint32_t)0x00000200)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000200)) | ((uint32_t)setupwr0 << 9));
}

__INLINE uint8_t emi0_hold_wr0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}

__INLINE void emi0_hold_wr0_setf(int reg_idx, uint8_t holdwr0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)holdwr0 << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000100)) | ((uint32_t)holdwr0 << 8));
}

__INLINE uint8_t emi0_ws_wr0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x000000E0)) >> 5);
}

__INLINE void emi0_ws_wr0_setf(int reg_idx, uint8_t wswr0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)wswr0 << 5) & ~((uint32_t)0x000000E0)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x000000E0)) | ((uint32_t)wswr0 << 5));
}

__INLINE uint8_t emi0_setup_rd0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}

__INLINE void emi0_setup_rd0_setf(int reg_idx, uint8_t setuprd0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)setuprd0 << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000010)) | ((uint32_t)setuprd0 << 4));
}

__INLINE uint8_t emi0_hold_rd0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}

__INLINE void emi0_hold_rd0_setf(int reg_idx, uint8_t holdrd0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)holdrd0 << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000008)) | ((uint32_t)holdrd0 << 3));
}

__INLINE uint8_t emi0_ws_rd0_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000007)) >> 0);
}

__INLINE void emi0_ws_rd0_setf(int reg_idx, uint8_t wsrd0)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)wsrd0 << 0) & ~((uint32_t)0x00000007)) == 0);
    REG_PL_WR(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4, (REG_PL_RD(EMI0_EXTMEMCNTL0_ADDR + reg_idx * 4) & ~((uint32_t)0x00000007)) | ((uint32_t)wsrd0 << 0));
}

/**
 * @brief REMAPMODE0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  01:00           REMAPMODE0   0x0
 * </pre>
 */
#define EMI0_REMAPMODE0_ADDR   0x1000B008
#define EMI0_REMAPMODE0_OFFSET 0x00000008
#define EMI0_REMAPMODE0_INDEX  0x00000002
#define EMI0_REMAPMODE0_RESET  0x00000000

__INLINE uint32_t emi0_remapmode0_get(void)
{
    return REG_PL_RD(EMI0_REMAPMODE0_ADDR);
}

__INLINE void emi0_remapmode0_set(uint32_t value)
{
    REG_PL_WR(EMI0_REMAPMODE0_ADDR, value);
}

// field definitions
#define EMI0_REMAPMODE0_MASK   ((uint32_t)0x00000003)
#define EMI0_REMAPMODE0_LSB    0
#define EMI0_REMAPMODE0_WIDTH  ((uint32_t)0x00000002)

#define EMI0_REMAPMODE0_RST    0x0

__INLINE uint8_t emi0_remapmode0_getf(void)
{
    uint32_t localVal = REG_PL_RD(EMI0_REMAPMODE0_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x00000003)) == 0);
    return (localVal >> 0);
}

__INLINE void emi0_remapmode0_setf(uint8_t remapmode0)
{
    ASSERT_ERR((((uint32_t)remapmode0 << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(EMI0_REMAPMODE0_ADDR, (uint32_t)remapmode0 << 0);
}

/**
 * @brief EMIVERSION0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             VERSION0   0x3
 * </pre>
 */
#define EMI0_EMIVERSION0_ADDR   0x1000B00C
#define EMI0_EMIVERSION0_OFFSET 0x0000000C
#define EMI0_EMIVERSION0_INDEX  0x00000003
#define EMI0_EMIVERSION0_RESET  0x00000003

__INLINE uint32_t emi0_emiversion0_get(void)
{
    return REG_PL_RD(EMI0_EMIVERSION0_ADDR);
}

__INLINE void emi0_emiversion0_set(uint32_t value)
{
    REG_PL_WR(EMI0_EMIVERSION0_ADDR, value);
}

// field definitions
#define EMI0_VERSION0_MASK   ((uint32_t)0x0000FFFF)
#define EMI0_VERSION0_LSB    0
#define EMI0_VERSION0_WIDTH  ((uint32_t)0x00000010)

#define EMI0_VERSION0_RST    0x3

__INLINE uint16_t emi0_version0_getf(void)
{
    uint32_t localVal = REG_PL_RD(EMI0_EMIVERSION0_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_EMI0_H_

