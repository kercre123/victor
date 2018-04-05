#ifndef _REG_EMI1_H_
#define _REG_EMI1_H_

#include <stdint.h>
#include "_reg_emi1.h"
#include "compiler.h"
#include "arch.h"
#include "reg_access.h"

#define REG_EMI1_COUNT 4

#define REG_EMI1_DECODING_MASK 0x0000000F

/**
 * @brief EXTMEMCNTL1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  11:10             IF_TYPE1   0x0
 *     09            SETUP_WR1   0
 *     08             HOLD_WR1   0
 *  07:05               WS_WR1   0x0
 *     04            SETUP_RD1   0
 *     03             HOLD_RD1   0
 *  02:00               WS_RD1   0x0
 * </pre>
 */
#define EMI1_EXTMEMCNTL1_ADDR   0x1000C000
#define EMI1_EXTMEMCNTL1_OFFSET 0x00000000
#define EMI1_EXTMEMCNTL1_INDEX  0x00000000
#define EMI1_EXTMEMCNTL1_RESET  0x00000000
#define EMI1_EXTMEMCNTL1_COUNT  2

__INLINE uint32_t emi1_extmemcntl1_get(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    return REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
}

__INLINE void emi1_extmemcntl1_set(int reg_idx, uint32_t value)
{
    ASSERT_ERR(reg_idx <= 1);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, value);
}

// field definitions
#define EMI1_IF_TYPE1_MASK    ((uint32_t)0x00000C00)
#define EMI1_IF_TYPE1_LSB     10
#define EMI1_IF_TYPE1_WIDTH   ((uint32_t)0x00000002)
#define EMI1_SETUP_WR1_BIT    ((uint32_t)0x00000200)
#define EMI1_SETUP_WR1_POS    9
#define EMI1_HOLD_WR1_BIT     ((uint32_t)0x00000100)
#define EMI1_HOLD_WR1_POS     8
#define EMI1_WS_WR1_MASK      ((uint32_t)0x000000E0)
#define EMI1_WS_WR1_LSB       5
#define EMI1_WS_WR1_WIDTH     ((uint32_t)0x00000003)
#define EMI1_SETUP_RD1_BIT    ((uint32_t)0x00000010)
#define EMI1_SETUP_RD1_POS    4
#define EMI1_HOLD_RD1_BIT     ((uint32_t)0x00000008)
#define EMI1_HOLD_RD1_POS     3
#define EMI1_WS_RD1_MASK      ((uint32_t)0x00000007)
#define EMI1_WS_RD1_LSB       0
#define EMI1_WS_RD1_WIDTH     ((uint32_t)0x00000003)

#define EMI1_IF_TYPE1_RST     0x0
#define EMI1_SETUP_WR1_RST    0x0
#define EMI1_HOLD_WR1_RST     0x0
#define EMI1_WS_WR1_RST       0x0
#define EMI1_SETUP_RD1_RST    0x0
#define EMI1_HOLD_RD1_RST     0x0
#define EMI1_WS_RD1_RST       0x0

__INLINE void emi1_extmemcntl1_pack(int reg_idx, uint8_t iftype1, uint8_t setupwr1, uint8_t holdwr1, uint8_t wswr1, uint8_t setuprd1, uint8_t holdrd1, uint8_t wsrd1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)iftype1 << 10) & ~((uint32_t)0x00000C00)) == 0);
    ASSERT_ERR((((uint32_t)setupwr1 << 9) & ~((uint32_t)0x00000200)) == 0);
    ASSERT_ERR((((uint32_t)holdwr1 << 8) & ~((uint32_t)0x00000100)) == 0);
    ASSERT_ERR((((uint32_t)wswr1 << 5) & ~((uint32_t)0x000000E0)) == 0);
    ASSERT_ERR((((uint32_t)setuprd1 << 4) & ~((uint32_t)0x00000010)) == 0);
    ASSERT_ERR((((uint32_t)holdrd1 << 3) & ~((uint32_t)0x00000008)) == 0);
    ASSERT_ERR((((uint32_t)wsrd1 << 0) & ~((uint32_t)0x00000007)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4,  ((uint32_t)iftype1 << 10) | ((uint32_t)setupwr1 << 9) | ((uint32_t)holdwr1 << 8) | ((uint32_t)wswr1 << 5) | ((uint32_t)setuprd1 << 4) | ((uint32_t)holdrd1 << 3) | ((uint32_t)wsrd1 << 0));
}

__INLINE void emi1_extmemcntl1_unpack(int reg_idx, uint8_t* iftype1, uint8_t* setupwr1, uint8_t* holdwr1, uint8_t* wswr1, uint8_t* setuprd1, uint8_t* holdrd1, uint8_t* wsrd1)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);

    *iftype1 = (localVal & ((uint32_t)0x00000C00)) >> 10;
    *setupwr1 = (localVal & ((uint32_t)0x00000200)) >> 9;
    *holdwr1 = (localVal & ((uint32_t)0x00000100)) >> 8;
    *wswr1 = (localVal & ((uint32_t)0x000000E0)) >> 5;
    *setuprd1 = (localVal & ((uint32_t)0x00000010)) >> 4;
    *holdrd1 = (localVal & ((uint32_t)0x00000008)) >> 3;
    *wsrd1 = (localVal & ((uint32_t)0x00000007)) >> 0;
}

__INLINE uint8_t emi1_if_type1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000C00)) >> 10);
}

__INLINE void emi1_if_type1_setf(int reg_idx, uint8_t iftype1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)iftype1 << 10) & ~((uint32_t)0x00000C00)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000C00)) | ((uint32_t)iftype1 << 10));
}

__INLINE uint8_t emi1_setup_wr1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000200)) >> 9);
}

__INLINE void emi1_setup_wr1_setf(int reg_idx, uint8_t setupwr1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)setupwr1 << 9) & ~((uint32_t)0x00000200)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000200)) | ((uint32_t)setupwr1 << 9));
}

__INLINE uint8_t emi1_hold_wr1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000100)) >> 8);
}

__INLINE void emi1_hold_wr1_setf(int reg_idx, uint8_t holdwr1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)holdwr1 << 8) & ~((uint32_t)0x00000100)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000100)) | ((uint32_t)holdwr1 << 8));
}

__INLINE uint8_t emi1_ws_wr1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x000000E0)) >> 5);
}

__INLINE void emi1_ws_wr1_setf(int reg_idx, uint8_t wswr1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)wswr1 << 5) & ~((uint32_t)0x000000E0)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x000000E0)) | ((uint32_t)wswr1 << 5));
}

__INLINE uint8_t emi1_setup_rd1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000010)) >> 4);
}

__INLINE void emi1_setup_rd1_setf(int reg_idx, uint8_t setuprd1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)setuprd1 << 4) & ~((uint32_t)0x00000010)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000010)) | ((uint32_t)setuprd1 << 4));
}

__INLINE uint8_t emi1_hold_rd1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000008)) >> 3);
}

__INLINE void emi1_hold_rd1_setf(int reg_idx, uint8_t holdrd1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)holdrd1 << 3) & ~((uint32_t)0x00000008)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000008)) | ((uint32_t)holdrd1 << 3));
}

__INLINE uint8_t emi1_ws_rd1_getf(int reg_idx)
{
    ASSERT_ERR(reg_idx <= 1);
    uint32_t localVal = REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4);
    return ((localVal & ((uint32_t)0x00000007)) >> 0);
}

__INLINE void emi1_ws_rd1_setf(int reg_idx, uint8_t wsrd1)
{
    ASSERT_ERR(reg_idx <= 1);
    ASSERT_ERR((((uint32_t)wsrd1 << 0) & ~((uint32_t)0x00000007)) == 0);
    REG_PL_WR(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4, (REG_PL_RD(EMI1_EXTMEMCNTL1_ADDR + reg_idx * 4) & ~((uint32_t)0x00000007)) | ((uint32_t)wsrd1 << 0));
}

/**
 * @brief REMAPMODE1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  01:00           REMAPMODE1   0x0
 * </pre>
 */
#define EMI1_REMAPMODE1_ADDR   0x1000C008
#define EMI1_REMAPMODE1_OFFSET 0x00000008
#define EMI1_REMAPMODE1_INDEX  0x00000002
#define EMI1_REMAPMODE1_RESET  0x00000000

__INLINE uint32_t emi1_remapmode1_get(void)
{
    return REG_PL_RD(EMI1_REMAPMODE1_ADDR);
}

__INLINE void emi1_remapmode1_set(uint32_t value)
{
    REG_PL_WR(EMI1_REMAPMODE1_ADDR, value);
}

// field definitions
#define EMI1_REMAPMODE1_MASK   ((uint32_t)0x00000003)
#define EMI1_REMAPMODE1_LSB    0
#define EMI1_REMAPMODE1_WIDTH  ((uint32_t)0x00000002)

#define EMI1_REMAPMODE1_RST    0x0

__INLINE uint8_t emi1_remapmode1_getf(void)
{
    uint32_t localVal = REG_PL_RD(EMI1_REMAPMODE1_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x00000003)) == 0);
    return (localVal >> 0);
}

__INLINE void emi1_remapmode1_setf(uint8_t remapmode1)
{
    ASSERT_ERR((((uint32_t)remapmode1 << 0) & ~((uint32_t)0x00000003)) == 0);
    REG_PL_WR(EMI1_REMAPMODE1_ADDR, (uint32_t)remapmode1 << 0);
}

/**
 * @brief EMIVERSION1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             VERSION1   0x3
 * </pre>
 */
#define EMI1_EMIVERSION1_ADDR   0x1000C00C
#define EMI1_EMIVERSION1_OFFSET 0x0000000C
#define EMI1_EMIVERSION1_INDEX  0x00000003
#define EMI1_EMIVERSION1_RESET  0x00000003

__INLINE uint32_t emi1_emiversion1_get(void)
{
    return REG_PL_RD(EMI1_EMIVERSION1_ADDR);
}

__INLINE void emi1_emiversion1_set(uint32_t value)
{
    REG_PL_WR(EMI1_EMIVERSION1_ADDR, value);
}

// field definitions
#define EMI1_VERSION1_MASK   ((uint32_t)0x0000FFFF)
#define EMI1_VERSION1_LSB    0
#define EMI1_VERSION1_WIDTH  ((uint32_t)0x00000010)

#define EMI1_VERSION1_RST    0x3

__INLINE uint16_t emi1_version1_getf(void)
{
    uint32_t localVal = REG_PL_RD(EMI1_EMIVERSION1_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_EMI1_H_

