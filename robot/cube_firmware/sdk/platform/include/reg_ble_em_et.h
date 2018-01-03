#ifndef _REG_BLE_EM_ET_H_
#define _REG_BLE_EM_ET_H_

#include <stdint.h>
#include "_reg_ble_em_et.h"
#include "compiler.h"
#include "arch.h"
#include "em_map_ble.h"
#include "reg_access.h"

#define REG_BLE_EM_ET_COUNT 1

#define REG_BLE_EM_ET_DECODING_MASK 0x00000000

//#define REG_BLE_EM_ET_ADDR_GET(idx) (jump_table_struct[offset_em_et] + (idx) * REG_BLE_EM_ET_SIZE)
#define REG_BLE_EM_ET_ADDR_GET(idx) (jump_table_struct[offset_em_et] + (idx) * REG_BLE_EM_ET_SIZE)  


/**
 * @brief EXTAB register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:14             ETSTATUS   0x0
 *     13           EXCPTRNRDY   0
 *  12:00                CSPTR   0x0
 * </pre>
 */
//#define BLE_EXTAB_ADDR   (0x50008000 + jump_table_struct[offset_em_et])
#define BLE_EXTAB_ADDR   (0x80000 + jump_table_struct[offset_em_et])
#define BLE_EXTAB_INDEX  0x00000000
#define BLE_EXTAB_RESET  0x00000000

__INLINE uint16_t ble_extab_get(int elt_idx)
{
    return EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE);
}

__INLINE void ble_extab_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE, value);
}

// field definitions
#define BLE_ETSTATUS_MASK     ((uint16_t)0x0000C000)
#define BLE_ETSTATUS_LSB      14
#define BLE_ETSTATUS_WIDTH    ((uint16_t)0x00000002)
#define BLE_EXCPTRNRDY_BIT    ((uint16_t)0x00002000)
#define BLE_EXCPTRNRDY_POS    13
#define BLE_CSPTR_MASK        ((uint16_t)0x00001FFF)
#define BLE_CSPTR_LSB         0
#define BLE_CSPTR_WIDTH       ((uint16_t)0x0000000D)

#define BLE_ETSTATUS_RST      0x0
#define BLE_EXCPTRNRDY_RST    0x0
#define BLE_CSPTR_RST         0x0

__INLINE void ble_extab_pack(int elt_idx, uint8_t etstatus, uint8_t excptrnrdy, uint16_t csptr)
{
    ASSERT_ERR((((uint16_t)etstatus << 14) & ~((uint16_t)0x0000C000)) == 0);
    ASSERT_ERR((((uint16_t)excptrnrdy << 13) & ~((uint16_t)0x00002000)) == 0);
    ASSERT_ERR((((uint16_t)csptr << 0) & ~((uint16_t)0x00001FFF)) == 0);
    EM_BLE_WR(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE,  ((uint16_t)etstatus << 14) | ((uint16_t)excptrnrdy << 13) | ((uint16_t)csptr << 0));
}

__INLINE void ble_extab_unpack(int elt_idx, uint8_t* etstatus, uint8_t* excptrnrdy, uint16_t* csptr)
{
    uint16_t localVal = EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE);

    *etstatus = (localVal & ((uint16_t)0x0000C000)) >> 14;
    *excptrnrdy = (localVal & ((uint16_t)0x00002000)) >> 13;
    *csptr = (localVal & ((uint16_t)0x00001FFF)) >> 0;
}

__INLINE uint8_t ble_etstatus_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE);
    return ((localVal & ((uint16_t)0x0000C000)) >> 14);
}

__INLINE void ble_etstatus_setf(int elt_idx, uint8_t etstatus)
{
    ASSERT_ERR((((uint16_t)etstatus << 14) & ~((uint16_t)0x0000C000)) == 0);
    EM_BLE_WR(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE, (EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE) & ~((uint16_t)0x0000C000)) | ((uint16_t)etstatus << 14));
}

__INLINE uint8_t ble_excptrnrdy_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE);
    return ((localVal & ((uint16_t)0x00002000)) >> 13);
}

__INLINE void ble_excptrnrdy_setf(int elt_idx, uint8_t excptrnrdy)
{
    ASSERT_ERR((((uint16_t)excptrnrdy << 13) & ~((uint16_t)0x00002000)) == 0);
    EM_BLE_WR(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE, (EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE) & ~((uint16_t)0x00002000)) | ((uint16_t)excptrnrdy << 13));
}

__INLINE uint16_t ble_csptr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE);
    return ((localVal & ((uint16_t)0x00001FFF)) >> 0);
}

__INLINE void ble_csptr_setf(int elt_idx, uint16_t csptr)
{
    ASSERT_ERR((((uint16_t)csptr << 0) & ~((uint16_t)0x00001FFF)) == 0);
    EM_BLE_WR(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE, (EM_BLE_RD(BLE_EXTAB_ADDR + elt_idx * REG_BLE_EM_ET_SIZE) & ~((uint16_t)0x00001FFF)) | ((uint16_t)csptr << 0));
}


#endif // _REG_BLE_EM_ET_H_

