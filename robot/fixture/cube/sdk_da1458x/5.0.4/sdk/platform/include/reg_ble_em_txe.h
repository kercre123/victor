#ifndef _REG_BLE_EM_TXE_H_
#define _REG_BLE_EM_TXE_H_

#include <stdint.h>
#include "_reg_ble_em_txe.h"
#include "compiler.h"
#include "arch.h"
#include "em_map_ble.h"
#include "reg_access.h"

#define REG_BLE_EM_TXE_COUNT 2

#define REG_BLE_EM_TXE_DECODING_MASK 0x00000003

//#define REG_BLE_EM_TXE_ADDR_GET(idx) (EM_BLE_TXE_OFFSET + (idx) * REG_BLE_EM_TXE_SIZE)
#define REG_BLE_EM_TXE_ADDR_GET(idx) (jump_table_struct[offset_em_txe] + (idx) * REG_BLE_EM_TXE_SIZE)


/**
 * @brief TXECNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15              TXEDONE   0
 *     14               TXERDY   0
 *  13:00           TXENEXTPTR   0x0
 * </pre>
 */
//#define BLE_TXECNTL_ADDR   (0x50008000 + EM_BLE_TXE_OFFSET)
#define BLE_TXECNTL_ADDR   (0x80000 + jump_table_struct[offset_em_txe])
#define BLE_TXECNTL_INDEX  0x00000000
#define BLE_TXECNTL_RESET  0x00000000

__INLINE uint16_t ble_txecntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
}

__INLINE void ble_txecntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, value);
}

// field definitions
#define BLE_TXEDONE_BIT       ((uint16_t)0x00008000)
#define BLE_TXEDONE_POS       15
#define BLE_TXERDY_BIT        ((uint16_t)0x00004000)
#define BLE_TXERDY_POS        14
#define BLE_TXENEXTPTR_MASK   ((uint16_t)0x00003FFF)
#define BLE_TXENEXTPTR_LSB    0
#define BLE_TXENEXTPTR_WIDTH  ((uint16_t)0x0000000E)

#define BLE_TXEDONE_RST       0x0
#define BLE_TXERDY_RST        0x0
#define BLE_TXENEXTPTR_RST    0x0

__INLINE void ble_txecntl_pack(int elt_idx, uint8_t txedone, uint8_t txerdy, uint16_t txenextptr)
{
    ASSERT_ERR((((uint16_t)txedone << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)txerdy << 14) & ~((uint16_t)0x00004000)) == 0);
    ASSERT_ERR((((uint16_t)txenextptr << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE,  ((uint16_t)txedone << 15) | ((uint16_t)txerdy << 14) | ((uint16_t)txenextptr << 0));
}

__INLINE void ble_txecntl_unpack(int elt_idx, uint8_t* txedone, uint8_t* txerdy, uint16_t* txenextptr)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);

    *txedone = (localVal & ((uint16_t)0x00008000)) >> 15;
    *txerdy = (localVal & ((uint16_t)0x00004000)) >> 14;
    *txenextptr = (localVal & ((uint16_t)0x00003FFF)) >> 0;
}

__INLINE uint8_t ble_txedone_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void ble_txedone_setf(int elt_idx, uint8_t txedone)
{
    ASSERT_ERR((((uint16_t)txedone << 15) & ~((uint16_t)0x00008000)) == 0);
    EM_BLE_WR(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)txedone << 15));
}

__INLINE uint8_t ble_txerdy_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00004000)) >> 14);
}

__INLINE void ble_txerdy_setf(int elt_idx, uint8_t txerdy)
{
    ASSERT_ERR((((uint16_t)txerdy << 14) & ~((uint16_t)0x00004000)) == 0);
    EM_BLE_WR(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00004000)) | ((uint16_t)txerdy << 14));
}

__INLINE uint16_t ble_txenextptr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00003FFF)) >> 0);
}

__INLINE void ble_txenextptr_setf(int elt_idx, uint16_t txenextptr)
{
    ASSERT_ERR((((uint16_t)txenextptr << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXECNTL_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00003FFF)) | ((uint16_t)txenextptr << 0));
}

/**
 * @brief TXEPHCE register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  12:08               TXELEN   0x0
 *     04                TXEMD   0
 *     03                TXESN   0
 *     02              TXENESN   0
 *  01:00              TXELLID   0x0
 * </pre>
 */
#define BLE_TXEPHCE_ADDR   (0x80002 + jump_table_struct[offset_em_txe])
#define BLE_TXEPHCE_INDEX  0x00000001
#define BLE_TXEPHCE_RESET  0x00000000

__INLINE uint16_t ble_txephce_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
}

__INLINE void ble_txephce_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, value);
}

// field definitions
#define BLE_TXELEN_MASK    ((uint16_t)0x00001F00)
#define BLE_TXELEN_LSB     8
#define BLE_TXELEN_WIDTH   ((uint16_t)0x00000005)
#define BLE_TXEMD_BIT      ((uint16_t)0x00000010)
#define BLE_TXEMD_POS      4
#define BLE_TXESN_BIT      ((uint16_t)0x00000008)
#define BLE_TXESN_POS      3
#define BLE_TXENESN_BIT    ((uint16_t)0x00000004)
#define BLE_TXENESN_POS    2
#define BLE_TXELLID_MASK   ((uint16_t)0x00000003)
#define BLE_TXELLID_LSB    0
#define BLE_TXELLID_WIDTH  ((uint16_t)0x00000002)

#define BLE_TXELEN_RST     0x0
#define BLE_TXEMD_RST      0x0
#define BLE_TXESN_RST      0x0
#define BLE_TXENESN_RST    0x0
#define BLE_TXELLID_RST    0x0

__INLINE void ble_txephce_pack(int elt_idx, uint8_t txelen, uint8_t txemd, uint8_t txesn, uint8_t txenesn, uint8_t txellid)
{
    ASSERT_ERR((((uint16_t)txelen << 8) & ~((uint16_t)0x00001F00)) == 0);
    ASSERT_ERR((((uint16_t)txemd << 4) & ~((uint16_t)0x00000010)) == 0);
    ASSERT_ERR((((uint16_t)txesn << 3) & ~((uint16_t)0x00000008)) == 0);
    ASSERT_ERR((((uint16_t)txenesn << 2) & ~((uint16_t)0x00000004)) == 0);
    ASSERT_ERR((((uint16_t)txellid << 0) & ~((uint16_t)0x00000003)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE,  ((uint16_t)txelen << 8) | ((uint16_t)txemd << 4) | ((uint16_t)txesn << 3) | ((uint16_t)txenesn << 2) | ((uint16_t)txellid << 0));
}

__INLINE void ble_txephce_unpack(int elt_idx, uint8_t* txelen, uint8_t* txemd, uint8_t* txesn, uint8_t* txenesn, uint8_t* txellid)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);

    *txelen = (localVal & ((uint16_t)0x00001F00)) >> 8;
    *txemd = (localVal & ((uint16_t)0x00000010)) >> 4;
    *txesn = (localVal & ((uint16_t)0x00000008)) >> 3;
    *txenesn = (localVal & ((uint16_t)0x00000004)) >> 2;
    *txellid = (localVal & ((uint16_t)0x00000003)) >> 0;
}

__INLINE uint8_t ble_txelen_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00001F00)) >> 8);
}

__INLINE void ble_txelen_setf(int elt_idx, uint8_t txelen)
{
    ASSERT_ERR((((uint16_t)txelen << 8) & ~((uint16_t)0x00001F00)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00001F00)) | ((uint16_t)txelen << 8));
}

__INLINE uint8_t ble_txemd_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00000010)) >> 4);
}

__INLINE void ble_txemd_setf(int elt_idx, uint8_t txemd)
{
    ASSERT_ERR((((uint16_t)txemd << 4) & ~((uint16_t)0x00000010)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00000010)) | ((uint16_t)txemd << 4));
}

__INLINE uint8_t ble_txesn_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00000008)) >> 3);
}

__INLINE void ble_txesn_setf(int elt_idx, uint8_t txesn)
{
    ASSERT_ERR((((uint16_t)txesn << 3) & ~((uint16_t)0x00000008)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00000008)) | ((uint16_t)txesn << 3));
}

__INLINE uint8_t ble_txenesn_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00000004)) >> 2);
}

__INLINE void ble_txenesn_setf(int elt_idx, uint8_t txenesn)
{
    ASSERT_ERR((((uint16_t)txenesn << 2) & ~((uint16_t)0x00000004)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00000004)) | ((uint16_t)txenesn << 2));
}

__INLINE uint8_t ble_txellid_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE);
    return ((localVal & ((uint16_t)0x00000003)) >> 0);
}

__INLINE void ble_txellid_setf(int elt_idx, uint8_t txellid)
{
    ASSERT_ERR((((uint16_t)txellid << 0) & ~((uint16_t)0x00000003)) == 0);
    EM_BLE_WR(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE, (EM_BLE_RD(BLE_TXEPHCE_ADDR + elt_idx * REG_BLE_EM_TXE_SIZE) & ~((uint16_t)0x00000003)) | ((uint16_t)txellid << 0));
}


#endif // _REG_BLE_EM_TXE_H_

