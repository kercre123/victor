#ifndef _REG_REMAP_H_
#define _REG_REMAP_H_

#include <stdint.h>
#include "_reg_remap.h"
#include "compiler.h"
#include "arch.h"
#include "reg_access.h"

#define REG_REMAP_COUNT 35

#define REG_REMAP_DECODING_MASK 0x0000007F

/**
 * @brief CLEARRESETMAP register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     01            REMAP_ROM   1
 *     00            REMAP_RAM   1
 * </pre>
 */
#define REMAP_CLEARRESETMAP_ADDR   0x10002020
#define REMAP_CLEARRESETMAP_OFFSET 0x00000020
#define REMAP_CLEARRESETMAP_INDEX  0x00000010
#define REMAP_CLEARRESETMAP_RESET  0x00000003

__INLINE uint16_t remap_clearresetmap_get(void)
{
    return REG_PL_RD(REMAP_CLEARRESETMAP_ADDR);
}

__INLINE void remap_clearresetmap_set(uint16_t value)
{
    REG_PL_WR(REMAP_CLEARRESETMAP_ADDR, value);
}

// field definitions
#define REMAP_REMAP_ROM_BIT    ((uint16_t)0x00000002)
#define REMAP_REMAP_ROM_POS    1
#define REMAP_REMAP_RAM_BIT    ((uint16_t)0x00000001)
#define REMAP_REMAP_RAM_POS    0

#define REMAP_REMAP_ROM_RST    0x1
#define REMAP_REMAP_RAM_RST    0x1

__INLINE void remap_clearresetmap_pack(uint8_t remaprom, uint8_t remapram)
{
    ASSERT_ERR((((uint16_t)remaprom << 1) & ~((uint16_t)0x00000002)) == 0);
    ASSERT_ERR((((uint16_t)remapram << 0) & ~((uint16_t)0x00000001)) == 0);
    REG_PL_WR(REMAP_CLEARRESETMAP_ADDR,  ((uint16_t)remaprom << 1) | ((uint16_t)remapram << 0));
}

__INLINE void remap_clearresetmap_unpack(uint8_t* remaprom, uint8_t* remapram)
{
    uint16_t localVal = REG_PL_RD(REMAP_CLEARRESETMAP_ADDR);

    *remaprom = (localVal & ((uint16_t)0x00000002)) >> 1;
    *remapram = (localVal & ((uint16_t)0x00000001)) >> 0;
}

__INLINE uint8_t remap_remap_rom_getf(void)
{
    uint16_t localVal = REG_PL_RD(REMAP_CLEARRESETMAP_ADDR);
    return ((localVal & ((uint16_t)0x00000002)) >> 1);
}

__INLINE void remap_remap_rom_setf(uint8_t remaprom)
{
    ASSERT_ERR((((uint16_t)remaprom << 1) & ~((uint16_t)0x00000002)) == 0);
    REG_PL_WR(REMAP_CLEARRESETMAP_ADDR, (REG_PL_RD(REMAP_CLEARRESETMAP_ADDR) & ~((uint16_t)0x00000002)) | ((uint16_t)remaprom << 1));
}

__INLINE uint8_t remap_remap_ram_getf(void)
{
    uint16_t localVal = REG_PL_RD(REMAP_CLEARRESETMAP_ADDR);
    return ((localVal & ((uint16_t)0x00000001)) >> 0);
}

__INLINE void remap_remap_ram_setf(uint8_t remapram)
{
    ASSERT_ERR((((uint16_t)remapram << 0) & ~((uint16_t)0x00000001)) == 0);
    REG_PL_WR(REMAP_CLEARRESETMAP_ADDR, (REG_PL_RD(REMAP_CLEARRESETMAP_ADDR) & ~((uint16_t)0x00000001)) | ((uint16_t)remapram << 0));
}

/**
 * @brief FPGAVERSION register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:08              REL_PLF   0x0
 *  07:00              UPG_PLF   0x0
 * </pre>
 */
#define REMAP_FPGAVERSION_ADDR   0x10002040
#define REMAP_FPGAVERSION_OFFSET 0x00000040
#define REMAP_FPGAVERSION_INDEX  0x00000020
#define REMAP_FPGAVERSION_RESET  0x00000000

__INLINE uint16_t remap_fpgaversion_get(void)
{
    return REG_PL_RD(REMAP_FPGAVERSION_ADDR);
}

// field definitions
#define REMAP_REL_PLF_MASK   ((uint16_t)0x0000FF00)
#define REMAP_REL_PLF_LSB    8
#define REMAP_REL_PLF_WIDTH  ((uint16_t)0x00000008)
#define REMAP_UPG_PLF_MASK   ((uint16_t)0x000000FF)
#define REMAP_UPG_PLF_LSB    0
#define REMAP_UPG_PLF_WIDTH  ((uint16_t)0x00000008)

#define REMAP_REL_PLF_RST    0x0
#define REMAP_UPG_PLF_RST    0x0

__INLINE void remap_fpgaversion_unpack(uint8_t* relplf, uint8_t* upgplf)
{
    uint16_t localVal = REG_PL_RD(REMAP_FPGAVERSION_ADDR);

    *relplf = (localVal & ((uint16_t)0x0000FF00)) >> 8;
    *upgplf = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t remap_rel_plf_getf(void)
{
    uint16_t localVal = REG_PL_RD(REMAP_FPGAVERSION_ADDR);
    return ((localVal & ((uint16_t)0x0000FF00)) >> 8);
}

__INLINE uint8_t remap_upg_plf_getf(void)
{
    uint16_t localVal = REG_PL_RD(REMAP_FPGAVERSION_ADDR);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

/**
 * @brief BUILD_PLF register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            BUILD_PLF   0x0
 * </pre>
 */
#define REMAP_BUILD_PLF_ADDR   0x10002044
#define REMAP_BUILD_PLF_OFFSET 0x00000044
#define REMAP_BUILD_PLF_INDEX  0x00000022
#define REMAP_BUILD_PLF_RESET  0x00000000

__INLINE uint16_t remap_build_plf_get(void)
{
    return REG_PL_RD(REMAP_BUILD_PLF_ADDR);
}

// field definitions
#define REMAP_BUILD_PLF_MASK   ((uint16_t)0x0000FFFF)
#define REMAP_BUILD_PLF_LSB    0
#define REMAP_BUILD_PLF_WIDTH  ((uint16_t)0x00000010)

#define REMAP_BUILD_PLF_RST    0x0

__INLINE uint16_t remap_build_plf_getf(void)
{
    uint16_t localVal = REG_PL_RD(REMAP_BUILD_PLF_ADDR);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}


#endif // _REG_REMAP_H_

