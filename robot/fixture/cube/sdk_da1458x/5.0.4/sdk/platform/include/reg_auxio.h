#ifndef _REG_AUXIO_H_
#define _REG_AUXIO_H_

#include <stdint.h>
#include "_reg_auxio.h"
#include "compiler.h"
#include "arch.h"
#include "reg_access.h"

#define REG_AUXIO_COUNT 8

#define REG_AUXIO_DECODING_MASK 0x0000001F

/**
 * @brief AUXIOA_OUT register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00               AUXIOA   0x0
 * </pre>
 */
#define AUXIO_AUXIOA_OUT_ADDR   0x1000A000
#define AUXIO_AUXIOA_OUT_OFFSET 0x00000000
#define AUXIO_AUXIOA_OUT_INDEX  0x00000000
#define AUXIO_AUXIOA_OUT_RESET  0x00000000

__INLINE uint32_t auxio_auxioa_out_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOA_OUT_ADDR);
}

__INLINE void auxio_auxioa_out_set(uint32_t value)
{
    REG_PL_WR(AUXIO_AUXIOA_OUT_ADDR, value);
}

// field definitions
#define AUXIO_AUXIOA_MASK   ((uint32_t)0x0000FFFF)
#define AUXIO_AUXIOA_LSB    0
#define AUXIO_AUXIOA_WIDTH  ((uint32_t)0x00000010)

#define AUXIO_AUXIOA_RST    0x0

__INLINE uint16_t auxio_auxioa_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOA_OUT_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void auxio_auxioa_setf(uint16_t auxioa)
{
    ASSERT_ERR((((uint32_t)auxioa << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(AUXIO_AUXIOA_OUT_ADDR, (uint32_t)auxioa << 0);
}

/**
 * @brief AUXIOB_IN register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  07:00               AUXIOB   0x0
 * </pre>
 */
#define AUXIO_AUXIOB_IN_ADDR   0x1000A008
#define AUXIO_AUXIOB_IN_OFFSET 0x00000008
#define AUXIO_AUXIOB_IN_INDEX  0x00000002
#define AUXIO_AUXIOB_IN_RESET  0x00000000

__INLINE uint32_t auxio_auxiob_in_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOB_IN_ADDR);
}

// field definitions
#define AUXIO_AUXIOB_MASK   ((uint32_t)0x000000FF)
#define AUXIO_AUXIOB_LSB    0
#define AUXIO_AUXIOB_WIDTH  ((uint32_t)0x00000008)

#define AUXIO_AUXIOB_RST    0x0

__INLINE uint8_t auxio_auxiob_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOB_IN_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x000000FF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief AUXIOC_OUT register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     00               AUXIOC   0
 * </pre>
 */
#define AUXIO_AUXIOC_OUT_ADDR   0x1000A010
#define AUXIO_AUXIOC_OUT_OFFSET 0x00000010
#define AUXIO_AUXIOC_OUT_INDEX  0x00000004
#define AUXIO_AUXIOC_OUT_RESET  0x00000000

__INLINE uint32_t auxio_auxioc_out_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOC_OUT_ADDR);
}

__INLINE void auxio_auxioc_out_set(uint32_t value)
{
    REG_PL_WR(AUXIO_AUXIOC_OUT_ADDR, value);
}

// field definitions
#define AUXIO_AUXIOC_BIT    ((uint32_t)0x00000001)
#define AUXIO_AUXIOC_POS    0

#define AUXIO_AUXIOC_RST    0x0

__INLINE uint8_t auxio_auxioc_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOC_OUT_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x00000001)) == 0);
    return (localVal >> 0);
}

__INLINE void auxio_auxioc_setf(uint8_t auxioc)
{
    ASSERT_ERR((((uint32_t)auxioc << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_PL_WR(AUXIO_AUXIOC_OUT_ADDR, (uint32_t)auxioc << 0);
}

/**
 * @brief AUXIOD_OUT register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00           AUXIOD_OUT   0x0
 * </pre>
 */
#define AUXIO_AUXIOD_OUT_ADDR   0x1000A014
#define AUXIO_AUXIOD_OUT_OFFSET 0x00000014
#define AUXIO_AUXIOD_OUT_INDEX  0x00000005
#define AUXIO_AUXIOD_OUT_RESET  0x00000000

__INLINE uint32_t auxio_auxiod_out_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOD_OUT_ADDR);
}

__INLINE void auxio_auxiod_out_set(uint32_t value)
{
    REG_PL_WR(AUXIO_AUXIOD_OUT_ADDR, value);
}

// field definitions
#define AUXIO_AUXIOD_OUT_MASK   ((uint32_t)0x0000FFFF)
#define AUXIO_AUXIOD_OUT_LSB    0
#define AUXIO_AUXIOD_OUT_WIDTH  ((uint32_t)0x00000010)

#define AUXIO_AUXIOD_OUT_RST    0x0

__INLINE uint16_t auxio_auxiod_out_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOD_OUT_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void auxio_auxiod_out_setf(uint16_t auxiodout)
{
    ASSERT_ERR((((uint32_t)auxiodout << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(AUXIO_AUXIOD_OUT_ADDR, (uint32_t)auxiodout << 0);
}

/**
 * @brief AUXIOD_IN register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            AUXIOD_IN   0x0
 * </pre>
 */
#define AUXIO_AUXIOD_IN_ADDR   0x1000A018
#define AUXIO_AUXIOD_IN_OFFSET 0x00000018
#define AUXIO_AUXIOD_IN_INDEX  0x00000006
#define AUXIO_AUXIOD_IN_RESET  0x00000000

__INLINE uint32_t auxio_auxiod_in_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOD_IN_ADDR);
}

// field definitions
#define AUXIO_AUXIOD_IN_MASK   ((uint32_t)0x0000FFFF)
#define AUXIO_AUXIOD_IN_LSB    0
#define AUXIO_AUXIOD_IN_WIDTH  ((uint32_t)0x00000010)

#define AUXIO_AUXIOD_IN_RST    0x0

__INLINE uint16_t auxio_auxiod_in_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOD_IN_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief AUXIOD_DIR register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00           AUXIOD_DIR   0x0
 * </pre>
 */
#define AUXIO_AUXIOD_DIR_ADDR   0x1000A01C
#define AUXIO_AUXIOD_DIR_OFFSET 0x0000001C
#define AUXIO_AUXIOD_DIR_INDEX  0x00000007
#define AUXIO_AUXIOD_DIR_RESET  0x00000000

__INLINE uint32_t auxio_auxiod_dir_get(void)
{
    return REG_PL_RD(AUXIO_AUXIOD_DIR_ADDR);
}

__INLINE void auxio_auxiod_dir_set(uint32_t value)
{
    REG_PL_WR(AUXIO_AUXIOD_DIR_ADDR, value);
}

// field definitions
#define AUXIO_AUXIOD_DIR_MASK   ((uint32_t)0x0000FFFF)
#define AUXIO_AUXIOD_DIR_LSB    0
#define AUXIO_AUXIOD_DIR_WIDTH  ((uint32_t)0x00000010)

#define AUXIO_AUXIOD_DIR_RST    0x0

__INLINE uint16_t auxio_auxiod_dir_getf(void)
{
    uint32_t localVal = REG_PL_RD(AUXIO_AUXIOD_DIR_ADDR);
    ASSERT_ERR((localVal & ~((uint32_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void auxio_auxiod_dir_setf(uint16_t auxioddir)
{
    ASSERT_ERR((((uint32_t)auxioddir << 0) & ~((uint32_t)0x0000FFFF)) == 0);
    REG_PL_WR(AUXIO_AUXIOD_DIR_ADDR, (uint32_t)auxioddir << 0);
}


#endif // _REG_AUXIO_H_

