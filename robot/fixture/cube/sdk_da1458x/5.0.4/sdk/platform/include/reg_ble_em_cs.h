#ifndef _REG_BLE_EM_CS_H_
#define _REG_BLE_EM_CS_H_

#include <stdint.h>
#include "_reg_ble_em_cs.h"
#include "compiler.h"
#include "arch.h"
#include "em_map_ble.h"
#include "reg_access.h"

#define REG_BLE_EM_CS_COUNT 39

#define REG_BLE_EM_CS_DECODING_MASK 0x0000007F

//#define REG_BLE_EM_CS_ADDR_GET(idx) (jump_table_struct[offset_em_cs]  + (idx) * REG_BLE_EM_CS_SIZE)
#define REG_BLE_EM_CS_ADDR_GET(idx) (jump_table_struct[offset_em_cs] + (idx) * REG_BLE_EM_CS_SIZE)
/**
 * @brief CNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:12                  PTI   0x0
 *     10             TXBSY_EN   0
 *     09             RXBSY_EN   0
 *     08              DNABORT   0
 *     07           TXCRYPT_EN   0
 *     06           RXCRYPT_EN   0
 *  04:00               FORMAT   0x0
 * </pre>
 */
//#define BLE_CNTL_ADDR   (0x50008000 + jump_table_struct[offset_em_cs] )
#define BLE_CNTL_ADDR   (0x80000 + jump_table_struct[offset_em_cs] )
#define BLE_CNTL_INDEX  0x00000000
#define BLE_CNTL_RESET  0x00000000

__INLINE uint16_t ble_cntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_cntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_PTI_MASK          ((uint16_t)0x0000F000)
#define BLE_PTI_LSB           12
#define BLE_PTI_WIDTH         ((uint16_t)0x00000004)
#define BLE_TXBSY_EN_BIT      ((uint16_t)0x00000400)
#define BLE_TXBSY_EN_POS      10
#define BLE_RXBSY_EN_BIT      ((uint16_t)0x00000200)
#define BLE_RXBSY_EN_POS      9
#define BLE_DNABORT_BIT       ((uint16_t)0x00000100)
#define BLE_DNABORT_POS       8
#define BLE_TXCRYPT_EN_BIT    ((uint16_t)0x00000080)
#define BLE_TXCRYPT_EN_POS    7
#define BLE_RXCRYPT_EN_BIT    ((uint16_t)0x00000040)
#define BLE_RXCRYPT_EN_POS    6
#define BLE_FORMAT_MASK       ((uint16_t)0x0000001F)
#define BLE_FORMAT_LSB        0
#define BLE_FORMAT_WIDTH      ((uint16_t)0x00000005)

#define BLE_PTI_RST           0x0
#define BLE_TXBSY_EN_RST      0x0
#define BLE_RXBSY_EN_RST      0x0
#define BLE_DNABORT_RST       0x0
#define BLE_TXCRYPT_EN_RST    0x0
#define BLE_RXCRYPT_EN_RST    0x0
#define BLE_FORMAT_RST        0x0

__INLINE void ble_cntl_pack(int elt_idx, uint8_t pti, uint8_t txbsyen, uint8_t rxbsyen, uint8_t dnabort, uint8_t txcrypten, uint8_t rxcrypten, uint8_t format)
{
    ASSERT_ERR((((uint16_t)pti << 12) & ~((uint16_t)0x0000F000)) == 0);
    ASSERT_ERR((((uint16_t)txbsyen << 10) & ~((uint16_t)0x00000400)) == 0);
    ASSERT_ERR((((uint16_t)rxbsyen << 9) & ~((uint16_t)0x00000200)) == 0);
    ASSERT_ERR((((uint16_t)dnabort << 8) & ~((uint16_t)0x00000100)) == 0);
    ASSERT_ERR((((uint16_t)txcrypten << 7) & ~((uint16_t)0x00000080)) == 0);
    ASSERT_ERR((((uint16_t)rxcrypten << 6) & ~((uint16_t)0x00000040)) == 0);
    ASSERT_ERR((((uint16_t)format << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)pti << 12) | ((uint16_t)txbsyen << 10) | ((uint16_t)rxbsyen << 9) | ((uint16_t)dnabort << 8) | ((uint16_t)txcrypten << 7) | ((uint16_t)rxcrypten << 6) | ((uint16_t)format << 0));
}

__INLINE void ble_cntl_unpack(int elt_idx, uint8_t* pti, uint8_t* txbsyen, uint8_t* rxbsyen, uint8_t* dnabort, uint8_t* txcrypten, uint8_t* rxcrypten, uint8_t* format)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *pti = (localVal & ((uint16_t)0x0000F000)) >> 12;
    *txbsyen = (localVal & ((uint16_t)0x00000400)) >> 10;
    *rxbsyen = (localVal & ((uint16_t)0x00000200)) >> 9;
    *dnabort = (localVal & ((uint16_t)0x00000100)) >> 8;
    *txcrypten = (localVal & ((uint16_t)0x00000080)) >> 7;
    *rxcrypten = (localVal & ((uint16_t)0x00000040)) >> 6;
    *format = (localVal & ((uint16_t)0x0000001F)) >> 0;
}

__INLINE uint8_t ble_pti_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000F000)) >> 12);
}

__INLINE void ble_pti_setf(int elt_idx, uint8_t pti)
{
    ASSERT_ERR((((uint16_t)pti << 12) & ~((uint16_t)0x0000F000)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000F000)) | ((uint16_t)pti << 12));
}

__INLINE uint8_t ble_txbsy_en_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000400)) >> 10);
}

__INLINE void ble_txbsy_en_setf(int elt_idx, uint8_t txbsyen)
{
    ASSERT_ERR((((uint16_t)txbsyen << 10) & ~((uint16_t)0x00000400)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000400)) | ((uint16_t)txbsyen << 10));
}

__INLINE uint8_t ble_rxbsy_en_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000200)) >> 9);
}

__INLINE void ble_rxbsy_en_setf(int elt_idx, uint8_t rxbsyen)
{
    ASSERT_ERR((((uint16_t)rxbsyen << 9) & ~((uint16_t)0x00000200)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000200)) | ((uint16_t)rxbsyen << 9));
}

__INLINE uint8_t ble_dnabort_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000100)) >> 8);
}

__INLINE void ble_dnabort_setf(int elt_idx, uint8_t dnabort)
{
    ASSERT_ERR((((uint16_t)dnabort << 8) & ~((uint16_t)0x00000100)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000100)) | ((uint16_t)dnabort << 8));
}

__INLINE uint8_t ble_txcrypt_en_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000080)) >> 7);
}

__INLINE void ble_txcrypt_en_setf(int elt_idx, uint8_t txcrypten)
{
    ASSERT_ERR((((uint16_t)txcrypten << 7) & ~((uint16_t)0x00000080)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000080)) | ((uint16_t)txcrypten << 7));
}

__INLINE uint8_t ble_rxcrypt_en_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000040)) >> 6);
}

__INLINE void ble_rxcrypt_en_setf(int elt_idx, uint8_t rxcrypten)
{
    ASSERT_ERR((((uint16_t)rxcrypten << 6) & ~((uint16_t)0x00000040)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000040)) | ((uint16_t)rxcrypten << 6));
}

__INLINE uint8_t ble_format_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000001F)) >> 0);
}

__INLINE void ble_format_setf(int elt_idx, uint8_t format)
{
    ASSERT_ERR((((uint16_t)format << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000001F)) | ((uint16_t)format << 0));
}

/**
 * @brief SYNCWL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            SYNCWORDL   0x0
 * </pre>
 */
#define BLE_SYNCWL_ADDR   (0x80002 + jump_table_struct[offset_em_cs] )
#define BLE_SYNCWL_INDEX  0x00000001
#define BLE_SYNCWL_RESET  0x00000000

__INLINE uint16_t ble_syncwl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_SYNCWL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_syncwl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_SYNCWL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_SYNCWORDL_MASK   ((uint16_t)0x0000FFFF)
#define BLE_SYNCWORDL_LSB    0
#define BLE_SYNCWORDL_WIDTH  ((uint16_t)0x00000010)

#define BLE_SYNCWORDL_RST    0x0

__INLINE uint16_t ble_syncwordl_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_SYNCWL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_syncwordl_setf(int elt_idx, uint16_t syncwordl)
{
    ASSERT_ERR((((uint16_t)syncwordl << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_SYNCWL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)syncwordl << 0);
}

/**
 * @brief SYNCWH register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            SYNCWORDH   0x0
 * </pre>
 */
#define BLE_SYNCWH_ADDR   (0x80004 + jump_table_struct[offset_em_cs] )
#define BLE_SYNCWH_INDEX  0x00000002
#define BLE_SYNCWH_RESET  0x00000000

__INLINE uint16_t ble_syncwh_get(int elt_idx)
{
    return EM_BLE_RD(BLE_SYNCWH_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_syncwh_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_SYNCWH_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_SYNCWORDH_MASK   ((uint16_t)0x0000FFFF)
#define BLE_SYNCWORDH_LSB    0
#define BLE_SYNCWORDH_WIDTH  ((uint16_t)0x00000010)

#define BLE_SYNCWORDH_RST    0x0

__INLINE uint16_t ble_syncwordh_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_SYNCWH_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_syncwordh_setf(int elt_idx, uint16_t syncwordh)
{
    ASSERT_ERR((((uint16_t)syncwordh << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_SYNCWH_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)syncwordh << 0);
}

/**
 * @brief CRCINIT0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             CRCINIT0   0x0
 * </pre>
 */
#define BLE_CRCINIT0_ADDR   (0x80006 + jump_table_struct[offset_em_cs] )
#define BLE_CRCINIT0_INDEX  0x00000003
#define BLE_CRCINIT0_RESET  0x00000000

__INLINE uint16_t ble_crcinit0_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CRCINIT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_crcinit0_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CRCINIT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_CRCINIT0_MASK   ((uint16_t)0x0000FFFF)
#define BLE_CRCINIT0_LSB    0
#define BLE_CRCINIT0_WIDTH  ((uint16_t)0x00000010)

#define BLE_CRCINIT0_RST    0x0

__INLINE uint16_t ble_crcinit0_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CRCINIT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_crcinit0_setf(int elt_idx, uint16_t crcinit0)
{
    ASSERT_ERR((((uint16_t)crcinit0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_CRCINIT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)crcinit0 << 0);
}

/**
 * @brief CRCINIT1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:08        FILTER_POLICY   0x0
 *  07:00             CRCINIT1   0x0
 * </pre>
 */
#define BLE_CRCINIT1_ADDR   (0x80008 + jump_table_struct[offset_em_cs] )
#define BLE_CRCINIT1_INDEX  0x00000004
#define BLE_CRCINIT1_RESET  0x00000000

__INLINE uint16_t ble_crcinit1_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_crcinit1_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_FILTER_POLICY_MASK   ((uint16_t)0x0000FF00)
#define BLE_FILTER_POLICY_LSB    8
#define BLE_FILTER_POLICY_WIDTH  ((uint16_t)0x00000008)
#define BLE_CRCINIT1_MASK        ((uint16_t)0x000000FF)
#define BLE_CRCINIT1_LSB         0
#define BLE_CRCINIT1_WIDTH       ((uint16_t)0x00000008)

#define BLE_FILTER_POLICY_RST    0x0
#define BLE_CRCINIT1_RST         0x0

__INLINE void ble_crcinit1_pack(int elt_idx, uint8_t filterpolicy, uint8_t crcinit1)
{
    ASSERT_ERR((((uint16_t)filterpolicy << 8) & ~((uint16_t)0x0000FF00)) == 0);
    ASSERT_ERR((((uint16_t)crcinit1 << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BLE_WR(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)filterpolicy << 8) | ((uint16_t)crcinit1 << 0));
}

__INLINE void ble_crcinit1_unpack(int elt_idx, uint8_t* filterpolicy, uint8_t* crcinit1)
{
    uint16_t localVal = EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *filterpolicy = (localVal & ((uint16_t)0x0000FF00)) >> 8;
    *crcinit1 = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t ble_filter_policy_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000FF00)) >> 8);
}

__INLINE void ble_filter_policy_setf(int elt_idx, uint8_t filterpolicy)
{
    ASSERT_ERR((((uint16_t)filterpolicy << 8) & ~((uint16_t)0x0000FF00)) == 0);
    EM_BLE_WR(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000FF00)) | ((uint16_t)filterpolicy << 8));
}

__INLINE uint8_t ble_crcinit1_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

__INLINE void ble_crcinit1_setf(int elt_idx, uint8_t crcinit1)
{
    ASSERT_ERR((((uint16_t)crcinit1 << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BLE_WR(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CRCINIT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x000000FF)) | ((uint16_t)crcinit1 << 0));
}

/**
 * @brief HOPCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15                FH_EN   0
 *  12:08              HOP_INT   0x0
 *  05:00               CH_IDX   0x0
 * </pre>
 */
#define BLE_HOPCNTL_ADDR   (0x8000A + jump_table_struct[offset_em_cs] )
#define BLE_HOPCNTL_INDEX  0x00000005
#define BLE_HOPCNTL_RESET  0x00000000

__INLINE uint16_t ble_hopcntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_hopcntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_FH_EN_BIT      ((uint16_t)0x00008000)
#define BLE_FH_EN_POS      15
#define BLE_HOP_INT_MASK   ((uint16_t)0x00001F00)
#define BLE_HOP_INT_LSB    8
#define BLE_HOP_INT_WIDTH  ((uint16_t)0x00000005)
#define BLE_CH_IDX_MASK    ((uint16_t)0x0000003F)
#define BLE_CH_IDX_LSB     0
#define BLE_CH_IDX_WIDTH   ((uint16_t)0x00000006)

#define BLE_FH_EN_RST      0x0
#define BLE_HOP_INT_RST    0x0
#define BLE_CH_IDX_RST     0x0

__INLINE void ble_hopcntl_pack(int elt_idx, uint8_t fhen, uint8_t hopint, uint8_t chidx)
{
    ASSERT_ERR((((uint16_t)fhen << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)hopint << 8) & ~((uint16_t)0x00001F00)) == 0);
    ASSERT_ERR((((uint16_t)chidx << 0) & ~((uint16_t)0x0000003F)) == 0);
    EM_BLE_WR(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)fhen << 15) | ((uint16_t)hopint << 8) | ((uint16_t)chidx << 0));
}

__INLINE void ble_hopcntl_unpack(int elt_idx, uint8_t* fhen, uint8_t* hopint, uint8_t* chidx)
{
    uint16_t localVal = EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *fhen = (localVal & ((uint16_t)0x00008000)) >> 15;
    *hopint = (localVal & ((uint16_t)0x00001F00)) >> 8;
    *chidx = (localVal & ((uint16_t)0x0000003F)) >> 0;
}

__INLINE uint8_t ble_fh_en_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void ble_fh_en_setf(int elt_idx, uint8_t fhen)
{
    ASSERT_ERR((((uint16_t)fhen << 15) & ~((uint16_t)0x00008000)) == 0);
    EM_BLE_WR(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)fhen << 15));
}

__INLINE uint8_t ble_hop_int_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00001F00)) >> 8);
}

__INLINE void ble_hop_int_setf(int elt_idx, uint8_t hopint)
{
    ASSERT_ERR((((uint16_t)hopint << 8) & ~((uint16_t)0x00001F00)) == 0);
    EM_BLE_WR(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00001F00)) | ((uint16_t)hopint << 8));
}

__INLINE uint8_t ble_ch_idx_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000003F)) >> 0);
}

__INLINE void ble_ch_idx_setf(int elt_idx, uint8_t chidx)
{
    ASSERT_ERR((((uint16_t)chidx << 0) & ~((uint16_t)0x0000003F)) == 0);
    EM_BLE_WR(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_HOPCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000003F)) | ((uint16_t)chidx << 0));
}

/**
 * @brief TXRXCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15          RXBUFF_FULL   0
 *     14            LASTEMPTY   0
 *     13                   SN   0
 *     12                 NESN   0
 *  11:08                RXTHR   0x0
 *  07:00                TXPWR   0x0
 * </pre>
 */
#define BLE_TXRXCNTL_ADDR   (0x8000C + jump_table_struct[offset_em_cs] )
#define BLE_TXRXCNTL_INDEX  0x00000006
#define BLE_TXRXCNTL_RESET  0x00000000

__INLINE uint16_t ble_txrxcntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_txrxcntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXBUFF_FULL_BIT    ((uint16_t)0x00008000)
#define BLE_RXBUFF_FULL_POS    15
#define BLE_LASTEMPTY_BIT      ((uint16_t)0x00004000)
#define BLE_LASTEMPTY_POS      14
#define BLE_SN_BIT             ((uint16_t)0x00002000)
#define BLE_SN_POS             13
#define BLE_NESN_BIT           ((uint16_t)0x00001000)
#define BLE_NESN_POS           12
#define BLE_RXTHR_MASK         ((uint16_t)0x00000F00)
#define BLE_RXTHR_LSB          8
#define BLE_RXTHR_WIDTH        ((uint16_t)0x00000004)
#define BLE_TXPWR_MASK         ((uint16_t)0x000000FF)
#define BLE_TXPWR_LSB          0
#define BLE_TXPWR_WIDTH        ((uint16_t)0x00000008)

#define BLE_RXBUFF_FULL_RST    0x0
#define BLE_LASTEMPTY_RST      0x0
#define BLE_SN_RST             0x0
#define BLE_NESN_RST           0x0
#define BLE_RXTHR_RST          0x0
#define BLE_TXPWR_RST          0x0

__INLINE void ble_txrxcntl_pack(int elt_idx, uint8_t rxbufffull, uint8_t lastempty, uint8_t sn, uint8_t nesn, uint8_t rxthr, uint8_t txpwr)
{
    ASSERT_ERR((((uint16_t)rxbufffull << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)lastempty << 14) & ~((uint16_t)0x00004000)) == 0);
    ASSERT_ERR((((uint16_t)sn << 13) & ~((uint16_t)0x00002000)) == 0);
    ASSERT_ERR((((uint16_t)nesn << 12) & ~((uint16_t)0x00001000)) == 0);
    ASSERT_ERR((((uint16_t)rxthr << 8) & ~((uint16_t)0x00000F00)) == 0);
    ASSERT_ERR((((uint16_t)txpwr << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)rxbufffull << 15) | ((uint16_t)lastempty << 14) | ((uint16_t)sn << 13) | ((uint16_t)nesn << 12) | ((uint16_t)rxthr << 8) | ((uint16_t)txpwr << 0));
}

__INLINE void ble_txrxcntl_unpack(int elt_idx, uint8_t* rxbufffull, uint8_t* lastempty, uint8_t* sn, uint8_t* nesn, uint8_t* rxthr, uint8_t* txpwr)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *rxbufffull = (localVal & ((uint16_t)0x00008000)) >> 15;
    *lastempty = (localVal & ((uint16_t)0x00004000)) >> 14;
    *sn = (localVal & ((uint16_t)0x00002000)) >> 13;
    *nesn = (localVal & ((uint16_t)0x00001000)) >> 12;
    *rxthr = (localVal & ((uint16_t)0x00000F00)) >> 8;
    *txpwr = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t ble_rxbuff_full_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void ble_rxbuff_full_setf(int elt_idx, uint8_t rxbufffull)
{
    ASSERT_ERR((((uint16_t)rxbufffull << 15) & ~((uint16_t)0x00008000)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)rxbufffull << 15));
}

__INLINE uint8_t ble_lastempty_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00004000)) >> 14);
}

__INLINE void ble_lastempty_setf(int elt_idx, uint8_t lastempty)
{
    ASSERT_ERR((((uint16_t)lastempty << 14) & ~((uint16_t)0x00004000)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00004000)) | ((uint16_t)lastempty << 14));
}

__INLINE uint8_t ble_sn_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00002000)) >> 13);
}

__INLINE void ble_sn_setf(int elt_idx, uint8_t sn)
{
    ASSERT_ERR((((uint16_t)sn << 13) & ~((uint16_t)0x00002000)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00002000)) | ((uint16_t)sn << 13));
}

__INLINE uint8_t ble_nesn_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00001000)) >> 12);
}

__INLINE void ble_nesn_setf(int elt_idx, uint8_t nesn)
{
    ASSERT_ERR((((uint16_t)nesn << 12) & ~((uint16_t)0x00001000)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00001000)) | ((uint16_t)nesn << 12));
}

__INLINE uint8_t ble_rxthr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000F00)) >> 8);
}

__INLINE void ble_rxthr_setf(int elt_idx, uint8_t rxthr)
{
    ASSERT_ERR((((uint16_t)rxthr << 8) & ~((uint16_t)0x00000F00)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000F00)) | ((uint16_t)rxthr << 8));
}

__INLINE uint8_t ble_txpwr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

__INLINE void ble_txpwr_setf(int elt_idx, uint8_t txpwr)
{
    ASSERT_ERR((((uint16_t)txpwr << 0) & ~((uint16_t)0x000000FF)) == 0);
    EM_BLE_WR(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_TXRXCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x000000FF)) | ((uint16_t)txpwr << 0));
}

/**
 * @brief RXWINCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15               RXWIDE   0
 *  13:00              RXWINSZ   0x0
 * </pre>
 */
#define BLE_RXWINCNTL_ADDR   (0x8000E + jump_table_struct[offset_em_cs] )
#define BLE_RXWINCNTL_INDEX  0x00000007
#define BLE_RXWINCNTL_RESET  0x00000000

__INLINE uint16_t ble_rxwincntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_rxwincntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXWIDE_BIT     ((uint16_t)0x00008000)
#define BLE_RXWIDE_POS     15
#define BLE_RXWINSZ_MASK   ((uint16_t)0x00003FFF)
#define BLE_RXWINSZ_LSB    0
#define BLE_RXWINSZ_WIDTH  ((uint16_t)0x0000000E)

#define BLE_RXWIDE_RST     0x0
#define BLE_RXWINSZ_RST    0x0

__INLINE void ble_rxwincntl_pack(int elt_idx, uint8_t rxwide, uint16_t rxwinsz)
{
    ASSERT_ERR((((uint16_t)rxwide << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)rxwinsz << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)rxwide << 15) | ((uint16_t)rxwinsz << 0));
}

__INLINE void ble_rxwincntl_unpack(int elt_idx, uint8_t* rxwide, uint16_t* rxwinsz)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *rxwide = (localVal & ((uint16_t)0x00008000)) >> 15;
    *rxwinsz = (localVal & ((uint16_t)0x00003FFF)) >> 0;
}

__INLINE uint8_t ble_rxwide_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void ble_rxwide_setf(int elt_idx, uint8_t rxwide)
{
    ASSERT_ERR((((uint16_t)rxwide << 15) & ~((uint16_t)0x00008000)) == 0);
    EM_BLE_WR(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)rxwide << 15));
}

__INLINE uint16_t ble_rxwinsz_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00003FFF)) >> 0);
}

__INLINE void ble_rxwinsz_setf(int elt_idx, uint16_t rxwinsz)
{
    ASSERT_ERR((((uint16_t)rxwinsz << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_RXWINCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00003FFF)) | ((uint16_t)rxwinsz << 0));
}

/**
 * @brief TXDESCPTR register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  13:00            TXDESCPTR   0x0
 * </pre>
 */
#define BLE_TXDESCPTR_ADDR   (0x80010 + jump_table_struct[offset_em_cs] )
#define BLE_TXDESCPTR_INDEX  0x00000008
#define BLE_TXDESCPTR_RESET  0x00000000

__INLINE uint16_t ble_txdescptr_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_txdescptr_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_TXDESCPTR_MASK   ((uint16_t)0x00003FFF)
#define BLE_TXDESCPTR_LSB    0
#define BLE_TXDESCPTR_WIDTH  ((uint16_t)0x0000000E)

#define BLE_TXDESCPTR_RST    0x0

__INLINE uint16_t ble_txdescptr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x00003FFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_txdescptr_setf(int elt_idx, uint16_t txdescptr)
{
    ASSERT_ERR((((uint16_t)txdescptr << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_TXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)txdescptr << 0);
}

/**
 * @brief RXDESCPTR register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     15               RXNRDY   0
 *  13:00            RXDESCPTR   0x0
 * </pre>
 */
#define BLE_RXDESCPTR_ADDR   (0x80012 + jump_table_struct[offset_em_cs] )
#define BLE_RXDESCPTR_INDEX  0x00000009
#define BLE_RXDESCPTR_RESET  0x00000000

__INLINE uint16_t ble_rxdescptr_get(int elt_idx)
{
    return EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_rxdescptr_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXNRDY_BIT       ((uint16_t)0x00008000)
#define BLE_RXNRDY_POS       15
#define BLE_RXDESCPTR_MASK   ((uint16_t)0x00003FFF)
#define BLE_RXDESCPTR_LSB    0
#define BLE_RXDESCPTR_WIDTH  ((uint16_t)0x0000000E)

#define BLE_RXNRDY_RST       0x0
#define BLE_RXDESCPTR_RST    0x0

__INLINE void ble_rxdescptr_pack(int elt_idx, uint8_t rxnrdy, uint16_t rxdescptr)
{
    ASSERT_ERR((((uint16_t)rxnrdy << 15) & ~((uint16_t)0x00008000)) == 0);
    ASSERT_ERR((((uint16_t)rxdescptr << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)rxnrdy << 15) | ((uint16_t)rxdescptr << 0));
}

__INLINE void ble_rxdescptr_unpack(int elt_idx, uint8_t* rxnrdy, uint16_t* rxdescptr)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *rxnrdy = (localVal & ((uint16_t)0x00008000)) >> 15;
    *rxdescptr = (localVal & ((uint16_t)0x00003FFF)) >> 0;
}

__INLINE uint8_t ble_rxnrdy_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00008000)) >> 15);
}

__INLINE void ble_rxnrdy_setf(int elt_idx, uint8_t rxnrdy)
{
    ASSERT_ERR((((uint16_t)rxnrdy << 15) & ~((uint16_t)0x00008000)) == 0);
    EM_BLE_WR(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00008000)) | ((uint16_t)rxnrdy << 15));
}

__INLINE uint16_t ble_rxdescptr_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00003FFF)) >> 0);
}

__INLINE void ble_rxdescptr_setf(int elt_idx, uint16_t rxdescptr)
{
    ASSERT_ERR((((uint16_t)rxdescptr << 0) & ~((uint16_t)0x00003FFF)) == 0);
    EM_BLE_WR(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_RXDESCPTR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00003FFF)) | ((uint16_t)rxdescptr << 0));
}

/**
 * @brief WINOFFSET register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            WINOFFSET   0x0
 * </pre>
 */
#define BLE_WINOFFSET_ADDR   (0x80014 + jump_table_struct[offset_em_cs] )
#define BLE_WINOFFSET_INDEX  0x0000000A
#define BLE_WINOFFSET_RESET  0x00000000

__INLINE uint16_t ble_winoffset_get(int elt_idx)
{
    return EM_BLE_RD(BLE_WINOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_winoffset_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_WINOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_WINOFFSET_MASK   ((uint16_t)0x0000FFFF)
#define BLE_WINOFFSET_LSB    0
#define BLE_WINOFFSET_WIDTH  ((uint16_t)0x00000010)

#define BLE_WINOFFSET_RST    0x0

__INLINE uint16_t ble_winoffset_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_WINOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_winoffset_setf(int elt_idx, uint16_t winoffset)
{
    ASSERT_ERR((((uint16_t)winoffset << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_WINOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)winoffset << 0);
}

/**
 * @brief MINEVTIME register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            MINEVTIME   0x0
 * </pre>
 */
#define BLE_MINEVTIME_ADDR   (0x80014 + jump_table_struct[offset_em_cs] )
#define BLE_MINEVTIME_INDEX  0x0000000A
#define BLE_MINEVTIME_RESET  0x00000000

__INLINE uint16_t ble_minevtime_get(int elt_idx)
{
    return EM_BLE_RD(BLE_MINEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_minevtime_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_MINEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_MINEVTIME_MASK   ((uint16_t)0x0000FFFF)
#define BLE_MINEVTIME_LSB    0
#define BLE_MINEVTIME_WIDTH  ((uint16_t)0x00000010)

#define BLE_MINEVTIME_RST    0x0

__INLINE uint16_t ble_minevtime_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_MINEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_minevtime_setf(int elt_idx, uint16_t minevtime)
{
    ASSERT_ERR((((uint16_t)minevtime << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_MINEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)minevtime << 0);
}

/**
 * @brief MAXEVTIME register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00            MAXEVTIME   0x0
 * </pre>
 */
#define BLE_MAXEVTIME_ADDR   (0x80016 + jump_table_struct[offset_em_cs] )
#define BLE_MAXEVTIME_INDEX  0x0000000B
#define BLE_MAXEVTIME_RESET  0x00000000

__INLINE uint16_t ble_maxevtime_get(int elt_idx)
{
    return EM_BLE_RD(BLE_MAXEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_maxevtime_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_MAXEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_MAXEVTIME_MASK   ((uint16_t)0x0000FFFF)
#define BLE_MAXEVTIME_LSB    0
#define BLE_MAXEVTIME_WIDTH  ((uint16_t)0x00000010)

#define BLE_MAXEVTIME_RST    0x0

__INLINE uint16_t ble_maxevtime_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_MAXEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_maxevtime_setf(int elt_idx, uint16_t maxevtime)
{
    ASSERT_ERR((((uint16_t)maxevtime << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_MAXEVTIME_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)maxevtime << 0);
}

/**
 * @brief CHMAP0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             LLCHMAP0   0xFFFF
 * </pre>
 */
#define BLE_CHMAP0_ADDR   (0x80018 + jump_table_struct[offset_em_cs] )
#define BLE_CHMAP0_INDEX  0x0000000C
#define BLE_CHMAP0_RESET  0x0000FFFF

__INLINE uint16_t ble_chmap0_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CHMAP0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_chmap0_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CHMAP0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_LLCHMAP0_MASK   ((uint16_t)0x0000FFFF)
#define BLE_LLCHMAP0_LSB    0
#define BLE_LLCHMAP0_WIDTH  ((uint16_t)0x00000010)

#define BLE_LLCHMAP0_RST    0xFFFF

__INLINE uint16_t ble_llchmap0_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CHMAP0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_llchmap0_setf(int elt_idx, uint16_t llchmap0)
{
    ASSERT_ERR((((uint16_t)llchmap0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_CHMAP0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)llchmap0 << 0);
}

/**
 * @brief CHMAP1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00             LLCHMAP1   0xFFFF
 * </pre>
 */
#define BLE_CHMAP1_ADDR   (0x8001A + jump_table_struct[offset_em_cs] )
#define BLE_CHMAP1_INDEX  0x0000000D
#define BLE_CHMAP1_RESET  0x0000FFFF

__INLINE uint16_t ble_chmap1_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CHMAP1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_chmap1_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CHMAP1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_LLCHMAP1_MASK   ((uint16_t)0x0000FFFF)
#define BLE_LLCHMAP1_LSB    0
#define BLE_LLCHMAP1_WIDTH  ((uint16_t)0x00000010)

#define BLE_LLCHMAP1_RST    0xFFFF

__INLINE uint16_t ble_llchmap1_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CHMAP1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_llchmap1_setf(int elt_idx, uint16_t llchmap1)
{
    ASSERT_ERR((((uint16_t)llchmap1 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_CHMAP1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)llchmap1 << 0);
}

/**
 * @brief CHMAP2 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  13:08             NBCHGOOG   0x25
 *  04:00             LLCHMAP3   0x1F
 * </pre>
 */
#define BLE_CHMAP2_ADDR   (0x8001C + jump_table_struct[offset_em_cs] )
#define BLE_CHMAP2_INDEX  0x0000000E
#define BLE_CHMAP2_RESET  0x0000251F

__INLINE uint16_t ble_chmap2_get(int elt_idx)
{
    return EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_chmap2_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_NBCHGOOG_MASK   ((uint16_t)0x00003F00)
#define BLE_NBCHGOOG_LSB    8
#define BLE_NBCHGOOG_WIDTH  ((uint16_t)0x00000006)
#define BLE_LLCHMAP3_MASK   ((uint16_t)0x0000001F)
#define BLE_LLCHMAP3_LSB    0
#define BLE_LLCHMAP3_WIDTH  ((uint16_t)0x00000005)

#define BLE_NBCHGOOG_RST    0x25
#define BLE_LLCHMAP3_RST    0x1F

__INLINE void ble_chmap2_pack(int elt_idx, uint8_t nbchgoog, uint8_t llchmap3)
{
    ASSERT_ERR((((uint16_t)nbchgoog << 8) & ~((uint16_t)0x00003F00)) == 0);
    ASSERT_ERR((((uint16_t)llchmap3 << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)nbchgoog << 8) | ((uint16_t)llchmap3 << 0));
}

__INLINE void ble_chmap2_unpack(int elt_idx, uint8_t* nbchgoog, uint8_t* llchmap3)
{
    uint16_t localVal = EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *nbchgoog = (localVal & ((uint16_t)0x00003F00)) >> 8;
    *llchmap3 = (localVal & ((uint16_t)0x0000001F)) >> 0;
}

__INLINE uint8_t ble_nbchgoog_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00003F00)) >> 8);
}

__INLINE void ble_nbchgoog_setf(int elt_idx, uint8_t nbchgoog)
{
    ASSERT_ERR((((uint16_t)nbchgoog << 8) & ~((uint16_t)0x00003F00)) == 0);
    EM_BLE_WR(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00003F00)) | ((uint16_t)nbchgoog << 8));
}

__INLINE uint8_t ble_llchmap3_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000001F)) >> 0);
}

__INLINE void ble_llchmap3_setf(int elt_idx, uint8_t llchmap3)
{
    ASSERT_ERR((((uint16_t)llchmap3 << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_CHMAP2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000001F)) | ((uint16_t)llchmap3 << 0));
}

/**
 * @brief FCNTOFFSET register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  09:00             FCNTOFFS   0x0
 * </pre>
 */
#define BLE_FCNTOFFSET_ADDR   (0x8001E + jump_table_struct[offset_em_cs] )
#define BLE_FCNTOFFSET_INDEX  0x0000000F
#define BLE_FCNTOFFSET_RESET  0x00000000

__INLINE uint16_t ble_fcntoffset_get(int elt_idx)
{
    return EM_BLE_RD(BLE_FCNTOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_fcntoffset_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_FCNTOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_FCNTOFFS_MASK   ((uint16_t)0x000003FF)
#define BLE_FCNTOFFS_LSB    0
#define BLE_FCNTOFFS_WIDTH  ((uint16_t)0x0000000A)

#define BLE_FCNTOFFS_RST    0x0

__INLINE uint16_t ble_fcntoffs_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_FCNTOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x000003FF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_fcntoffs_setf(int elt_idx, uint16_t fcntoffs)
{
    ASSERT_ERR((((uint16_t)fcntoffs << 0) & ~((uint16_t)0x000003FF)) == 0);
    EM_BLE_WR(BLE_FCNTOFFSET_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)fcntoffs << 0);
}

/**
 * @brief SK register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00                   SK   0x0
 * </pre>
 */
#define BLE_SK_ADDR   (0x80020 + jump_table_struct[offset_em_cs] )
#define BLE_SK_INDEX  0x00000010
#define BLE_SK_RESET  0x00000000
#define BLE_SK_COUNT  8

__INLINE uint16_t ble_sk_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 7);
    return EM_BLE_RD(BLE_SK_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
}

__INLINE void ble_sk_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 7);
    EM_BLE_WR(BLE_SK_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, value);
}

// field definitions
#define BLE_SK_MASK   ((uint16_t)0x0000FFFF)
#define BLE_SK_LSB    0
#define BLE_SK_WIDTH  ((uint16_t)0x00000010)

#define BLE_SK_RST    0x0

__INLINE uint16_t ble_sk_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 7);
    uint16_t localVal = EM_BLE_RD(BLE_SK_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_sk_setf(int elt_idx, int reg_idx, uint16_t sk)
{
    ASSERT_ERR(reg_idx <= 7);
    ASSERT_ERR((((uint16_t)sk << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_SK_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, (uint16_t)sk << 0);
}

/**
 * @brief ADV_BD_ADDR register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00          ADV_BD_ADDR   0x0
 * </pre>
 */
#define BLE_ADV_BD_ADDR_ADDR   (0x80020 + jump_table_struct[offset_em_cs] )
#define BLE_ADV_BD_ADDR_INDEX  0x00000010
#define BLE_ADV_BD_ADDR_RESET  0x00000000
#define BLE_ADV_BD_ADDR_COUNT  3

__INLINE uint16_t ble_adv_bd_addr_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 2);
    return EM_BLE_RD(BLE_ADV_BD_ADDR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
}

__INLINE void ble_adv_bd_addr_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 2);
    EM_BLE_WR(BLE_ADV_BD_ADDR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, value);
}

// field definitions
#define BLE_ADV_BD_ADDR_MASK   ((uint16_t)0x0000FFFF)
#define BLE_ADV_BD_ADDR_LSB    0
#define BLE_ADV_BD_ADDR_WIDTH  ((uint16_t)0x00000010)

#define BLE_ADV_BD_ADDR_RST    0x0

__INLINE uint16_t ble_adv_bd_addr_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 2);
    uint16_t localVal = EM_BLE_RD(BLE_ADV_BD_ADDR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_adv_bd_addr_setf(int elt_idx, int reg_idx, uint16_t advbdaddr)
{
    ASSERT_ERR(reg_idx <= 2);
    ASSERT_ERR((((uint16_t)advbdaddr << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_ADV_BD_ADDR_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, (uint16_t)advbdaddr << 0);
}

/**
 * @brief ADV_BD_ADDR_TYPE register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *     00     ADV_BD_ADDR_TYPE   0
 * </pre>
 */
#define BLE_ADV_BD_ADDR_TYPE_ADDR   (0x80026 + jump_table_struct[offset_em_cs] )
#define BLE_ADV_BD_ADDR_TYPE_INDEX  0x00000013
#define BLE_ADV_BD_ADDR_TYPE_RESET  0x00000000

__INLINE uint16_t ble_adv_bd_addr_type_get(int elt_idx)
{
    return EM_BLE_RD(BLE_ADV_BD_ADDR_TYPE_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_adv_bd_addr_type_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_ADV_BD_ADDR_TYPE_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_ADV_BD_ADDR_TYPE_BIT    ((uint16_t)0x00000001)
#define BLE_ADV_BD_ADDR_TYPE_POS    0

#define BLE_ADV_BD_ADDR_TYPE_RST    0x0

__INLINE uint8_t ble_adv_bd_addr_type_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_ADV_BD_ADDR_TYPE_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x00000001)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_adv_bd_addr_type_setf(int elt_idx, uint8_t advbdaddrtype)
{
    ASSERT_ERR((((uint16_t)advbdaddrtype << 0) & ~((uint16_t)0x00000001)) == 0);
    EM_BLE_WR(BLE_ADV_BD_ADDR_TYPE_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)advbdaddrtype << 0);
}

/**
 * @brief IV register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00                   IV   0x0
 * </pre>
 */
#define BLE_IV_ADDR   (0x80030 + jump_table_struct[offset_em_cs] )
#define BLE_IV_INDEX  0x00000018
#define BLE_IV_RESET  0x00000000
#define BLE_IV_COUNT  4

__INLINE uint16_t ble_iv_get(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 3);
    return EM_BLE_RD(BLE_IV_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
}

__INLINE void ble_iv_set(int elt_idx, int reg_idx, uint16_t value)
{
    ASSERT_ERR(reg_idx <= 3);
    EM_BLE_WR(BLE_IV_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, value);
}

// field definitions
#define BLE_IV_MASK   ((uint16_t)0x0000FFFF)
#define BLE_IV_LSB    0
#define BLE_IV_WIDTH  ((uint16_t)0x00000010)

#define BLE_IV_RST    0x0

__INLINE uint16_t ble_iv_getf(int elt_idx, int reg_idx)
{
    ASSERT_ERR(reg_idx <= 3);
    uint16_t localVal = EM_BLE_RD(BLE_IV_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_iv_setf(int elt_idx, int reg_idx, uint16_t iv)
{
    ASSERT_ERR(reg_idx <= 3);
    ASSERT_ERR((((uint16_t)iv << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_IV_ADDR + elt_idx * REG_BLE_EM_CS_SIZE + reg_idx * 2, (uint16_t)iv << 0);
}

/**
 * @brief TXCCMPKTCNT0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00         TXCCMPKTCNT0   0x0
 * </pre>
 */
#define BLE_TXCCMPKTCNT0_ADDR   (0x80038 + jump_table_struct[offset_em_cs] )
#define BLE_TXCCMPKTCNT0_INDEX  0x0000001C
#define BLE_TXCCMPKTCNT0_RESET  0x00000000

__INLINE uint16_t ble_txccmpktcnt0_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_txccmpktcnt0_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_TXCCMPKTCNT0_MASK   ((uint16_t)0x0000FFFF)
#define BLE_TXCCMPKTCNT0_LSB    0
#define BLE_TXCCMPKTCNT0_WIDTH  ((uint16_t)0x00000010)

#define BLE_TXCCMPKTCNT0_RST    0x0

__INLINE uint16_t ble_txccmpktcnt0_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_txccmpktcnt0_setf(int elt_idx, uint16_t txccmpktcnt0)
{
    ASSERT_ERR((((uint16_t)txccmpktcnt0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_TXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)txccmpktcnt0 << 0);
}

/**
 * @brief TXCCMPKTCNT1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00         TXCCMPKTCNT1   0x0
 * </pre>
 */
#define BLE_TXCCMPKTCNT1_ADDR   (0x8003A + jump_table_struct[offset_em_cs] )
#define BLE_TXCCMPKTCNT1_INDEX  0x0000001D
#define BLE_TXCCMPKTCNT1_RESET  0x00000000

__INLINE uint16_t ble_txccmpktcnt1_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_txccmpktcnt1_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_TXCCMPKTCNT1_MASK   ((uint16_t)0x0000FFFF)
#define BLE_TXCCMPKTCNT1_LSB    0
#define BLE_TXCCMPKTCNT1_WIDTH  ((uint16_t)0x00000010)

#define BLE_TXCCMPKTCNT1_RST    0x0

__INLINE uint16_t ble_txccmpktcnt1_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_txccmpktcnt1_setf(int elt_idx, uint16_t txccmpktcnt1)
{
    ASSERT_ERR((((uint16_t)txccmpktcnt1 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_TXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)txccmpktcnt1 << 0);
}

/**
 * @brief TXCCMPKTCNT2 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  06:00         TXCCMPKTCNT2   0x0
 * </pre>
 */
#define BLE_TXCCMPKTCNT2_ADDR   (0x8003C + jump_table_struct[offset_em_cs] )
#define BLE_TXCCMPKTCNT2_INDEX  0x0000001E
#define BLE_TXCCMPKTCNT2_RESET  0x00000000

__INLINE uint16_t ble_txccmpktcnt2_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_txccmpktcnt2_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_TXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_TXCCMPKTCNT2_MASK   ((uint16_t)0x0000007F)
#define BLE_TXCCMPKTCNT2_LSB    0
#define BLE_TXCCMPKTCNT2_WIDTH  ((uint16_t)0x00000007)

#define BLE_TXCCMPKTCNT2_RST    0x0

__INLINE uint8_t ble_txccmpktcnt2_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000007F)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_txccmpktcnt2_setf(int elt_idx, uint8_t txccmpktcnt2)
{
    ASSERT_ERR((((uint16_t)txccmpktcnt2 << 0) & ~((uint16_t)0x0000007F)) == 0);
    EM_BLE_WR(BLE_TXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)txccmpktcnt2 << 0);
}

/**
 * @brief RXCCMPKTCNT0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00         RXCCMPKTCNT0   0x0
 * </pre>
 */
#define BLE_RXCCMPKTCNT0_ADDR   (0x8003E + jump_table_struct[offset_em_cs] )
#define BLE_RXCCMPKTCNT0_INDEX  0x0000001F
#define BLE_RXCCMPKTCNT0_RESET  0x00000000

__INLINE uint16_t ble_rxccmpktcnt0_get(int elt_idx)
{
    return EM_BLE_RD(BLE_RXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_rxccmpktcnt0_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_RXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXCCMPKTCNT0_MASK   ((uint16_t)0x0000FFFF)
#define BLE_RXCCMPKTCNT0_LSB    0
#define BLE_RXCCMPKTCNT0_WIDTH  ((uint16_t)0x00000010)

#define BLE_RXCCMPKTCNT0_RST    0x0

__INLINE uint16_t ble_rxccmpktcnt0_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_rxccmpktcnt0_setf(int elt_idx, uint16_t rxccmpktcnt0)
{
    ASSERT_ERR((((uint16_t)rxccmpktcnt0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_RXCCMPKTCNT0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)rxccmpktcnt0 << 0);
}

/**
 * @brief RXCCMPKTCNT1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00         RXCCMPKTCNT1   0x0
 * </pre>
 */
#define BLE_RXCCMPKTCNT1_ADDR   (0x80040 + jump_table_struct[offset_em_cs] )
#define BLE_RXCCMPKTCNT1_INDEX  0x00000020
#define BLE_RXCCMPKTCNT1_RESET  0x00000000

__INLINE uint16_t ble_rxccmpktcnt1_get(int elt_idx)
{
    return EM_BLE_RD(BLE_RXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_rxccmpktcnt1_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_RXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXCCMPKTCNT1_MASK   ((uint16_t)0x0000FFFF)
#define BLE_RXCCMPKTCNT1_LSB    0
#define BLE_RXCCMPKTCNT1_WIDTH  ((uint16_t)0x00000010)

#define BLE_RXCCMPKTCNT1_RST    0x0

__INLINE uint16_t ble_rxccmpktcnt1_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_rxccmpktcnt1_setf(int elt_idx, uint16_t rxccmpktcnt1)
{
    ASSERT_ERR((((uint16_t)rxccmpktcnt1 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
    EM_BLE_WR(BLE_RXCCMPKTCNT1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)rxccmpktcnt1 << 0);
}

/**
 * @brief RXCCMPKTCNT2 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  05:00         RXCCMPKTCNT2   0x0
 * </pre>
 */
#define BLE_RXCCMPKTCNT2_ADDR   (0x80042 + jump_table_struct[offset_em_cs] )
#define BLE_RXCCMPKTCNT2_INDEX  0x00000021
#define BLE_RXCCMPKTCNT2_RESET  0x00000000

__INLINE uint16_t ble_rxccmpktcnt2_get(int elt_idx)
{
    return EM_BLE_RD(BLE_RXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_rxccmpktcnt2_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_RXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_RXCCMPKTCNT2_MASK   ((uint16_t)0x0000003F)
#define BLE_RXCCMPKTCNT2_LSB    0
#define BLE_RXCCMPKTCNT2_WIDTH  ((uint16_t)0x00000006)

#define BLE_RXCCMPKTCNT2_RST    0x0

__INLINE uint8_t ble_rxccmpktcnt2_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_RXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000003F)) == 0);
    return (localVal >> 0);
}

__INLINE void ble_rxccmpktcnt2_setf(int elt_idx, uint8_t rxccmpktcnt2)
{
    ASSERT_ERR((((uint16_t)rxccmpktcnt2 << 0) & ~((uint16_t)0x0000003F)) == 0);
    EM_BLE_WR(BLE_RXCCMPKTCNT2_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (uint16_t)rxccmpktcnt2 << 0);
}

/**
 * @brief BTCNTSYNC0 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:00           BTCNTSYNC0   0x0
 * </pre>
 */
#define BLE_BTCNTSYNC0_ADDR   (0x80044 + jump_table_struct[offset_em_cs] )
#define BLE_BTCNTSYNC0_INDEX  0x00000022
#define BLE_BTCNTSYNC0_RESET  0x00000000

__INLINE uint16_t ble_btcntsync0_get(int elt_idx)
{
    return EM_BLE_RD(BLE_BTCNTSYNC0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

// field definitions
#define BLE_BTCNTSYNC0_MASK   ((uint16_t)0x0000FFFF)
#define BLE_BTCNTSYNC0_LSB    0
#define BLE_BTCNTSYNC0_WIDTH  ((uint16_t)0x00000010)

#define BLE_BTCNTSYNC0_RST    0x0

__INLINE uint16_t ble_btcntsync0_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_BTCNTSYNC0_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief BTCNTSYNC1 register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  06:00           BTCNTSYNC1   0x0
 * </pre>
 */
#define BLE_BTCNTSYNC1_ADDR   (0x80046 + jump_table_struct[offset_em_cs] )
#define BLE_BTCNTSYNC1_INDEX  0x00000023
#define BLE_BTCNTSYNC1_RESET  0x00000000

__INLINE uint16_t ble_btcntsync1_get(int elt_idx)
{
    return EM_BLE_RD(BLE_BTCNTSYNC1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

// field definitions
#define BLE_BTCNTSYNC1_MASK   ((uint16_t)0x0000007F)
#define BLE_BTCNTSYNC1_LSB    0
#define BLE_BTCNTSYNC1_WIDTH  ((uint16_t)0x00000007)

#define BLE_BTCNTSYNC1_RST    0x0

__INLINE uint8_t ble_btcntsync1_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_BTCNTSYNC1_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x0000007F)) == 0);
    return (localVal >> 0);
}

/**
 * @brief FCNTSYNC register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  09:00           FCNTRXSYNC   0x0
 * </pre>
 */
#define BLE_FCNTSYNC_ADDR   (0x80048 + jump_table_struct[offset_em_cs] )
#define BLE_FCNTSYNC_INDEX  0x00000024
#define BLE_FCNTSYNC_RESET  0x00000000

__INLINE uint16_t ble_fcntsync_get(int elt_idx)
{
    return EM_BLE_RD(BLE_FCNTSYNC_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

// field definitions
#define BLE_FCNTRXSYNC_MASK   ((uint16_t)0x000003FF)
#define BLE_FCNTRXSYNC_LSB    0
#define BLE_FCNTRXSYNC_WIDTH  ((uint16_t)0x0000000A)

#define BLE_FCNTRXSYNC_RST    0x0

__INLINE uint16_t ble_fcntrxsync_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_FCNTSYNC_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    ASSERT_ERR((localVal & ~((uint16_t)0x000003FF)) == 0);
    return (localVal >> 0);
}

/**
 * @brief TXRXDESCCNT register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:08            RXDESCCNT   0x0
 *  07:00            TXDESCCNT   0x0
 * </pre>
 */
#define BLE_TXRXDESCCNT_ADDR   (0x8004A + jump_table_struct[offset_em_cs] )
#define BLE_TXRXDESCCNT_INDEX  0x00000025
#define BLE_TXRXDESCCNT_RESET  0x00000000

__INLINE uint16_t ble_txrxdesccnt_get(int elt_idx)
{
    return EM_BLE_RD(BLE_TXRXDESCCNT_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

// field definitions
#define BLE_RXDESCCNT_MASK   ((uint16_t)0x0000FF00)
#define BLE_RXDESCCNT_LSB    8
#define BLE_RXDESCCNT_WIDTH  ((uint16_t)0x00000008)
#define BLE_TXDESCCNT_MASK   ((uint16_t)0x000000FF)
#define BLE_TXDESCCNT_LSB    0
#define BLE_TXDESCCNT_WIDTH  ((uint16_t)0x00000008)

#define BLE_RXDESCCNT_RST    0x0
#define BLE_TXDESCCNT_RST    0x0

__INLINE void ble_txrxdesccnt_unpack(int elt_idx, uint8_t* rxdesccnt, uint8_t* txdesccnt)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXDESCCNT_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *rxdesccnt = (localVal & ((uint16_t)0x0000FF00)) >> 8;
    *txdesccnt = (localVal & ((uint16_t)0x000000FF)) >> 0;
}

__INLINE uint8_t ble_rxdesccnt_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXDESCCNT_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000FF00)) >> 8);
}

__INLINE uint8_t ble_txdesccnt_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_TXRXDESCCNT_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x000000FF)) >> 0);
}

/**
 * @brief DMPRIOCNTL register definition
 * <pre>
 *   Bits           Field Name   Reset Value
 *  -----   ------------------   -----------
 *  15:13          PRIOINCSTEP   0x0
 *  12:08              MINPRIO   0x0
 *     07             CONFLICT   0
 *  04:00          CURRENTPRIO   0x0
 * </pre>
 */
#define BLE_DMPRIOCNTL_ADDR   (0x8004C + jump_table_struct[offset_em_cs] )
#define BLE_DMPRIOCNTL_INDEX  0x00000026
#define BLE_DMPRIOCNTL_RESET  0x00000000

__INLINE uint16_t ble_dmpriocntl_get(int elt_idx)
{
    return EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
}

__INLINE void ble_dmpriocntl_set(int elt_idx, uint16_t value)
{
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, value);
}

// field definitions
#define BLE_PRIOINCSTEP_MASK   ((uint16_t)0x0000E000)
#define BLE_PRIOINCSTEP_LSB    13
#define BLE_PRIOINCSTEP_WIDTH  ((uint16_t)0x00000003)
#define BLE_MINPRIO_MASK       ((uint16_t)0x00001F00)
#define BLE_MINPRIO_LSB        8
#define BLE_MINPRIO_WIDTH      ((uint16_t)0x00000005)
#define BLE_CONFLICT_BIT       ((uint16_t)0x00000080)
#define BLE_CONFLICT_POS       7
#define BLE_CURRENTPRIO_MASK   ((uint16_t)0x0000001F)
#define BLE_CURRENTPRIO_LSB    0
#define BLE_CURRENTPRIO_WIDTH  ((uint16_t)0x00000005)

#define BLE_PRIOINCSTEP_RST    0x0
#define BLE_MINPRIO_RST        0x0
#define BLE_CONFLICT_RST       0x0
#define BLE_CURRENTPRIO_RST    0x0

__INLINE void ble_dmpriocntl_pack(int elt_idx, uint8_t prioincstep, uint8_t minprio, uint8_t conflict, uint8_t currentprio)
{
    ASSERT_ERR((((uint16_t)prioincstep << 13) & ~((uint16_t)0x0000E000)) == 0);
    ASSERT_ERR((((uint16_t)minprio << 8) & ~((uint16_t)0x00001F00)) == 0);
    ASSERT_ERR((((uint16_t)conflict << 7) & ~((uint16_t)0x00000080)) == 0);
    ASSERT_ERR((((uint16_t)currentprio << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE,  ((uint16_t)prioincstep << 13) | ((uint16_t)minprio << 8) | ((uint16_t)conflict << 7) | ((uint16_t)currentprio << 0));
}

__INLINE void ble_dmpriocntl_unpack(int elt_idx, uint8_t* prioincstep, uint8_t* minprio, uint8_t* conflict, uint8_t* currentprio)
{
    uint16_t localVal = EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);

    *prioincstep = (localVal & ((uint16_t)0x0000E000)) >> 13;
    *minprio = (localVal & ((uint16_t)0x00001F00)) >> 8;
    *conflict = (localVal & ((uint16_t)0x00000080)) >> 7;
    *currentprio = (localVal & ((uint16_t)0x0000001F)) >> 0;
}

__INLINE uint8_t ble_prioincstep_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000E000)) >> 13);
}

__INLINE void ble_prioincstep_setf(int elt_idx, uint8_t prioincstep)
{
    ASSERT_ERR((((uint16_t)prioincstep << 13) & ~((uint16_t)0x0000E000)) == 0);
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000E000)) | ((uint16_t)prioincstep << 13));
}

__INLINE uint8_t ble_minprio_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00001F00)) >> 8);
}

__INLINE void ble_minprio_setf(int elt_idx, uint8_t minprio)
{
    ASSERT_ERR((((uint16_t)minprio << 8) & ~((uint16_t)0x00001F00)) == 0);
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00001F00)) | ((uint16_t)minprio << 8));
}

__INLINE uint8_t ble_conflict_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x00000080)) >> 7);
}

__INLINE void ble_conflict_setf(int elt_idx, uint8_t conflict)
{
    ASSERT_ERR((((uint16_t)conflict << 7) & ~((uint16_t)0x00000080)) == 0);
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x00000080)) | ((uint16_t)conflict << 7));
}

__INLINE uint8_t ble_currentprio_getf(int elt_idx)
{
    uint16_t localVal = EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE);
    return ((localVal & ((uint16_t)0x0000001F)) >> 0);
}

__INLINE void ble_currentprio_setf(int elt_idx, uint8_t currentprio)
{
    ASSERT_ERR((((uint16_t)currentprio << 0) & ~((uint16_t)0x0000001F)) == 0);
    EM_BLE_WR(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE, (EM_BLE_RD(BLE_DMPRIOCNTL_ADDR + elt_idx * REG_BLE_EM_CS_SIZE) & ~((uint16_t)0x0000001F)) | ((uint16_t)currentprio << 0));
}


#endif // _REG_BLE_EM_CS_H_

