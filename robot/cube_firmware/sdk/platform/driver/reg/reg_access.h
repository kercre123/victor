/**
 ****************************************************************************************
 *
 * @file reg_access.h
 *
 * @brief File implementing the basic primitives for register accesses
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

#ifndef REG_ACCESS_H_
#define REG_ACCESS_H_

/**
 ****************************************************************************************
 * @addtogroup REG REG_ACCESS
 * @ingroup DRIVERS
 *
 * @brief Basic primitives for register access
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>            // string functions

#if defined(CFG_BLE)
#include "_reg_ble_em_et.h"    // BLE exchange table
#endif //CFG_BLE
#if defined(CFG_BT)
#include "_reg_bt_em_et.h"     // BT exchange table
#endif //CFG_BT

/*
 * DEFINES
 ****************************************************************************************
 */

#if defined(CFG_BLE)
/// Base address of the exchange memory
#define REG_BLE_EM_BASE_ADDR         REG_BLE_EM_ET_BASE_ADDR
#endif //CFG_BLE
#if defined(CFG_BT)
#define REG_BT_EM_BASE_ADDR          REG_BT_EM_ET_BASE_ADDR
#endif //CFG_BT

/*
 * MACROS
 ****************************************************************************************
 */
/// Macro to read a platform register
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(addr))

/// Macro to write a platform register
#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BLE register
#define REG_BLE_RD(addr)             (*(volatile uint32_t *)(addr))

/// Macro to write a BLE register
#define REG_BLE_WR(addr, value)      (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BLE control structure field (16-bit wide)
#define EM_BLE_RD(addr)              (*(volatile uint16_t *)(addr))

/// Macro to write a BLE control structure field (16-bit wide)
#define EM_BLE_WR(addr, value)       (*(volatile uint16_t *)(addr)) = (value)

/// Macro to read a BT register
#define REG_BT_RD(addr)              (*(volatile uint32_t *)(addr))

/// Macro to write a BT register
#define REG_BT_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)

/// Macro to read a BT control structure field (16-bit wide)
#define EM_BT_RD(addr)               (*(volatile uint16_t *)(addr))

/// Macro to write a BT control structure field (16-bit wide)
#define EM_BT_WR(addr, value)        (*(volatile uint16_t *)(addr)) = (value)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if defined(CFG_BLE)
/// BLE read burst
__INLINE void em_ble_burst_rd(void *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy(sys_addr, (void *)(em_addr + REG_BLE_EM_BASE_ADDR), len);
}
/// BLE write burst
__INLINE void em_ble_burst_wr(void const *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy((void *)(em_addr + REG_BLE_EM_BASE_ADDR), sys_addr, len);
}
#endif // CFG_BLE

#if defined(CFG_BT)
/// EM setting
__INLINE void em_bt_set(int value, uint16_t em_addr, uint16_t len)
{
    memset((void *)(em_addr + REG_BT_EM_ET_BASE_ADDR), value, len);
}
#endif // CFG_BT
/// @} REG

#endif // REG_ACCESS_H_
