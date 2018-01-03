#ifndef _REG_CLKRST_H_
#define _REG_CLKRST_H_

#include <stdint.h>
#include "_reg_clkrst.h"
#include "compiler.h"
#include "arch.h"
#include "reg_access.h"

#define REG_CLKRST_COUNT 13

#define REG_CLKRST_DECODING_MASK 0x0000001F

/**
 * @brief SYSRESET register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     02             WD_RESET   0
 *     01             SW_RESET   0
 *     00          PWRON_RESET   0
 * </pre>
 */
#define CLKRST_SYSRESET_ADDR   0x10000000
#define CLKRST_SYSRESET_OFFSET 0x00000000
#define CLKRST_SYSRESET_INDEX  0x00000000
#define CLKRST_SYSRESET_RESET  0x00000000

__INLINE uint16_t clkrst_sysreset_get(void)
{
    return REG_PL_RD(CLKRST_SYSRESET_ADDR);
}

// field definitions
#define CLKRST_WD_RESET_BIT       ((uint16_t)0x00000004)
#define CLKRST_WD_RESET_POS       2
#define CLKRST_SW_RESET_BIT       ((uint16_t)0x00000002)
#define CLKRST_SW_RESET_POS       1
#define CLKRST_PWRON_RESET_BIT    ((uint16_t)0x00000001)
#define CLKRST_PWRON_RESET_POS    0

#define CLKRST_WD_RESET_RST       0x0
#define CLKRST_SW_RESET_RST       0x0
#define CLKRST_PWRON_RESET_RST    0x0

__INLINE void clkrst_sysreset_unpack(uint8_t* wdreset, uint8_t* swreset, uint8_t* pwronreset)
{
    uint16_t localVal = REG_PL_RD(CLKRST_SYSRESET_ADDR);

    *wdreset = (localVal & ((uint16_t)0x00000004)) >> 2;
    *swreset = (localVal & ((uint16_t)0x00000002)) >> 1;
    *pwronreset = (localVal & ((uint16_t)0x00000001)) >> 0;
}

__INLINE uint8_t clkrst_sysreset_wd_reset_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_SYSRESET_ADDR);
    return ((localVal & ((uint16_t)0x00000004)) >> 2);
}

__INLINE uint8_t clkrst_sysreset_sw_reset_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_SYSRESET_ADDR);
    return ((localVal & ((uint16_t)0x00000002)) >> 1);
}

__INLINE uint8_t clkrst_sysreset_pwron_reset_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_SYSRESET_ADDR);
    return ((localVal & ((uint16_t)0x00000001)) >> 0);
}

/**
 * @brief SOFTRESET register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     02          RWBTRFRESET   0
 *     01            RWBTRESET   0
 *     00            SOFTRESET   0
 * </pre>
 */
#define CLKRST_SOFTRESET_ADDR   0x10000004
#define CLKRST_SOFTRESET_OFFSET 0x00000004
#define CLKRST_SOFTRESET_INDEX  0x00000002
#define CLKRST_SOFTRESET_RESET  0x00000000

__INLINE void clkrst_softreset_set(uint16_t value)
{
    REG_PL_WR(CLKRST_SOFTRESET_ADDR, value);
}

// field definitions
#define CLKRST_RWBTRFRESET_BIT    ((uint16_t)0x00000004)
#define CLKRST_RWBTRFRESET_POS    2
#define CLKRST_RWBTRESET_BIT      ((uint16_t)0x00000002)
#define CLKRST_RWBTRESET_POS      1
#define CLKRST_SOFTRESET_BIT      ((uint16_t)0x00000001)
#define CLKRST_SOFTRESET_POS      0

#define CLKRST_RWBTRFRESET_RST    0x0
#define CLKRST_RWBTRESET_RST      0x0
#define CLKRST_SOFTRESET_RST      0x0

__INLINE void clkrst_softreset_pack(uint8_t rwbtrfreset, uint8_t rwbtreset, uint8_t softreset)
{
    ASSERT_ERR((((uint16_t)rwbtrfreset << 2) & ~((uint16_t)0x00000004)) == 0);
    ASSERT_ERR((((uint16_t)rwbtreset << 1) & ~((uint16_t)0x00000002)) == 0);
    ASSERT_ERR((((uint16_t)softreset << 0) & ~((uint16_t)0x00000001)) == 0);
    REG_PL_WR(CLKRST_SOFTRESET_ADDR,  ((uint16_t)rwbtrfreset << 2) | ((uint16_t)rwbtreset << 1) | ((uint16_t)softreset << 0));
}

__INLINE void clkrst_softreset_rwbtrfreset_setf(uint8_t rwbtrfreset)
{
    ASSERT_ERR((((uint16_t)rwbtrfreset << 2) & ~((uint16_t)0x00000004)) == 0);
    REG_PL_WR(CLKRST_SOFTRESET_ADDR, (REG_PL_RD(CLKRST_SOFTRESET_ADDR) & ~((uint16_t)0x00000004)) | ((uint16_t)rwbtrfreset << 2));
}

__INLINE void clkrst_softreset_rwbtreset_setf(uint8_t rwbtreset)
{
    ASSERT_ERR((((uint16_t)rwbtreset << 1) & ~((uint16_t)0x00000002)) == 0);
    REG_PL_WR(CLKRST_SOFTRESET_ADDR, (REG_PL_RD(CLKRST_SOFTRESET_ADDR) & ~((uint16_t)0x00000002)) | ((uint16_t)rwbtreset << 1));
}

__INLINE void clkrst_softreset_softreset_setf(uint8_t softreset)
{
    ASSERT_ERR((((uint16_t)softreset << 0) & ~((uint16_t)0x00000001)) == 0);
    REG_PL_WR(CLKRST_SOFTRESET_ADDR, (REG_PL_RD(CLKRST_SOFTRESET_ADDR) & ~((uint16_t)0x00000001)) | ((uint16_t)softreset << 0));
}

/**
 * @brief WDCONF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15             WDENABLE   0
 *     07                WDCLK   0
 *  06:00                WDDIV   0x1
 * </pre>
 */
#define CLKRST_WDCONF_ADDR   0x10000008
#define CLKRST_WDCONF_OFFSET 0x00000008
#define CLKRST_WDCONF_INDEX  0x00000004
#define CLKRST_WDCONF_RESET  0x00000001

__INLINE uint16_t clkrst_wdconf_get(void)
{
    return REG_PL_RD(CLKRST_WDCONF_ADDR);
}

__INLINE void clkrst_wdconf_set(uint16_t value)
{
    REG_PL_WR(CLKRST_WDCONF_ADDR, value);
}

// field definitions
#define CLKRST_WDENABLE_BIT    ((uint16_t)0x00008000)
#define CLKRST_WDENABLE_POS    15
#define CLKRST_WDCLK_BIT       ((uint16_t)0x00000080)
#define CLKRST_WDCLK_POS       7
#define CLKRST_WDDIV_MASK      ((uint16_t)0x0000007F)
#define CLKRST_WDDIV_LSB       0
#define CLKRST_WDDIV_WIDTH     ((uint16_t)0x00000007)

#define CLKRST_WDENABLE_RST    0x0
#define CLKRST_WDCLK_RST       0x0
#define CLKRST_WDDIV_RST       0x1

__INLINE void clkrst_wdconf_pack(uint8_t wdenable, uint8_t wdclk, uint8_t wddiv)
{
    ASSERT_ERR((((uint16_t)wdenable << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)wdclk << 7) & ~((uint16_t)0x00000080)) == 0);
    ASSERT_ERR((((uint16_t)wddiv << 0) & ~((uint16_t)0x0000007F)) == 0);
    REG_PL_WR(CLKRST_WDCONF_ADDR,  ((uint16_t)wdenable << 15) | ((uint16_t)wdclk << 7) | ((uint16_t)wddiv << 0));
}

__INLINE void clkrst_wdconf_unpack(uint8_t* wdenable, uint8_t* wdclk, uint8_t* wddiv)
{
    uint16_t localVal = REG_PL_RD(CLKRST_WDCONF_ADDR);

    *wdenable = (localVal & ((uint16_t)0x00008000)) >> 15;
    *wdclk = (localVal & ((uint16_t)0x00000080)) >> 7;
    *wddiv = (localVal & ((uint16_t)0x0000007F)) >> 0;
}

__INLINE uint8_t clkrst_wdconf_wdenable_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_WDCONF_ADDR);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void clkrst_wdconf_wdenable_setf(uint8_t wdenable)
{
    ASSERT_ERR((((uint16_t)wdenable << 15) & ~((uint16_t)0x00008000)) == 0);
    REG_PL_WR(CLKRST_WDCONF_ADDR, (REG_PL_RD(CLKRST_WDCONF_ADDR) & ~((uint16_t)0x00008000)) | ((uint16_t)wdenable << 15));
}

__INLINE uint8_t clkrst_wdconf_wdclk_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_WDCONF_ADDR);
    return ((localVal & ((uint16_t)0x00000080)) >> 7);
}

__INLINE void clkrst_wdconf_wdclk_setf(uint8_t wdclk)
{
    ASSERT_ERR((((uint16_t)wdclk << 7) & ~((uint16_t)0x00000080)) == 0);
    REG_PL_WR(CLKRST_WDCONF_ADDR, (REG_PL_RD(CLKRST_WDCONF_ADDR) & ~((uint16_t)0x00000080)) | ((uint16_t)wdclk << 7));
}

__INLINE uint8_t clkrst_wdconf_wddiv_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_WDCONF_ADDR);
    return ((localVal & ((uint16_t)0x0000007F)) >> 0);
}

__INLINE void clkrst_wdconf_wddiv_setf(uint8_t wddiv)
{
    ASSERT_ERR((((uint16_t)wddiv << 0) & ~((uint16_t)0x0000007F)) == 0);
    REG_PL_WR(CLKRST_WDCONF_ADDR, (REG_PL_RD(CLKRST_WDCONF_ADDR) & ~((uint16_t)0x0000007F)) | ((uint16_t)wddiv << 0));
}

/**
 * @brief WDSTROKE register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             WDSTROKE   0x0
 * </pre>
 */
#define CLKRST_WDSTROKE_ADDR   0x1000000C
#define CLKRST_WDSTROKE_OFFSET 0x0000000C
#define CLKRST_WDSTROKE_INDEX  0x00000006
#define CLKRST_WDSTROKE_RESET  0x00000000

__INLINE void clkrst_wdstroke_set(uint16_t value)
{
    REG_PL_WR(CLKRST_WDSTROKE_ADDR, value);
}

// field definitions
#define CLKRST_WDSTROKE_MASK   ((uint16_t)0x0000FFFF)
#define CLKRST_WDSTROKE_LSB    0
#define CLKRST_WDSTROKE_WIDTH  ((uint16_t)0x00000010)

#define CLKRST_WDSTROKE_RST    0x0

__INLINE void clkrst_wdstroke_setf(uint16_t wdstroke)
{
    ASSERT_ERR((((uint16_t)wdstroke << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    REG_PL_WR(CLKRST_WDSTROKE_ADDR, (uint16_t)wdstroke << 0);
}

/**
 * @brief CLKCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  07:00               CLKSEL   0xD
 * </pre>
 */
#define CLKRST_CLKCNTL_ADDR   0x10000010
#define CLKRST_CLKCNTL_OFFSET 0x00000010
#define CLKRST_CLKCNTL_INDEX  0x00000008
#define CLKRST_CLKCNTL_RESET  0x0000000D

__INLINE uint16_t clkrst_clkcntl_get(void)
{
    return REG_PL_RD(CLKRST_CLKCNTL_ADDR);
}

__INLINE void clkrst_clkcntl_set(uint16_t value)
{
    REG_PL_WR(CLKRST_CLKCNTL_ADDR, value);
}

// field definitions
#define CLKRST_CLKSEL_MASK   ((uint16_t)0x000000FF)
#define CLKRST_CLKSEL_LSB    0
#define CLKRST_CLKSEL_WIDTH  ((uint16_t)0x00000008)

#define CLKRST_CLKSEL_RST    0xD

__INLINE uint8_t clkrst_clkcntl_clksel_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_CLKCNTL_ADDR);
    ASSERT_ERR((localVal & ~((uint16_t)0x000000FF)) == 0);
    return (localVal >> 0);
}

__INLINE void clkrst_clkcntl_clksel_setf(uint8_t clksel)
{
    ASSERT_ERR((((uint16_t)clksel << 0) & ~((uint16_t)0x000000FF)) == 0);
    REG_PL_WR(CLKRST_CLKCNTL_ADDR, (uint16_t)clksel << 0);
}

/**
 * @brief RF_ID register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  09:00                   ID   0x0
 * </pre>
 */
#define CLKRST_RF_ID_ADDR   0x10000018
#define CLKRST_RF_ID_OFFSET 0x00000018
#define CLKRST_RF_ID_INDEX  0x0000000C
#define CLKRST_RF_ID_RESET  0x00000000

__INLINE uint16_t clkrst_rf_id_get(void)
{
    return REG_PL_RD(CLKRST_RF_ID_ADDR);
}

// field definitions
#define CLKRST_ID_MASK   ((uint16_t)0x000003FF)
#define CLKRST_ID_LSB    0
#define CLKRST_ID_WIDTH  ((uint16_t)0x0000000A)

#define CLKRST_ID_RST    0x0

__INLINE uint16_t clkrst_rf_id_id_getf(void)
{
    uint16_t localVal = REG_PL_RD(CLKRST_RF_ID_ADDR);
    ASSERT_ERR((localVal & ~((uint16_t)0x000003FF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_CLKRST_H_

