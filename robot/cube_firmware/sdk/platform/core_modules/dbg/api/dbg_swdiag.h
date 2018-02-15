/**
****************************************************************************************
*
* @file dbg_swdiag.h
*
* @brief SW profiling module
*
* Copyright (C) RivieraWaves 2009-2013
*
*
****************************************************************************************
*/

#ifndef DBG_SWDIAG_H_
#define DBG_SWDIAG_H_

/**
 ****************************************************************************************
 * @addtogroup DBGSWDIAG Diag
 * @ingroup DBG
 * @brief Debug SW profiling module
 *
 * SW profiling is a debug feature that provides user a configurable way to analyze SW execution performance or
 * behavior, such as timings, state machines, bit field values and so on.
 * It manages the SW state representation over HW digital diagnostic signals (diagports), from the signals selection to
 * the toggling of real HW signals.
 * HW signals representing the SW execution could then be watched on a logic analyzer in parallel with HW internal
 * signals, that could be very useful for low level debugging.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"       // stack configuration

#if (!RW_SWDIAG)

/// Macro used to set a SW diag to specific value
#define DBG_SWDIAG(bank , field , value)

#else //RW_SWDIAG

#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"         // ble core registers
#endif // BLE_EMB_PRESENT

/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of HW diagport banks
#define DBG_SWDIAG_NB_HW_BANKS        4

/// Number of SW profiles
#define DBG_SWDIAG_NB_PROFILES        16

/// Undefined configuration
#define DBG_SWDIAG_UNDEFINED        0xFF


/*
 * MACROS
 ****************************************************************************************
 */
#if (BLE_EMB_PRESENT)
#define SW_DIAG_SET     ble_swprofiling_set
#define SW_DIAG_GET     ble_swprofiling_get
#endif //BLE_EMB_PRESENT

/// Macro used to set a SW diag to specific value
#define DBG_SWDIAG(bank , field , value)                                              \
    if(sw_to_hw[DBG_SWDIAG_##bank] != DBG_SWDIAG_UNDEFINED)                           \
    {                                                                                 \
        SW_DIAG_SET( ((SW_DIAG_GET()) & (~(DBG_SWDIAG_##bank##_##field##_MASK << (8 * sw_to_hw[DBG_SWDIAG_##bank])))) | ((((value << DBG_SWDIAG_##bank##_##field##_OFFSET) & DBG_SWDIAG_##bank##_##field##_MASK) << (8*sw_to_hw[DBG_SWDIAG_##bank])))  );      \
    }


/*
 * SW DIAGS MAP
 ****************************************************************************************
 */

/**
******************************************************************************************
* @brief BANK 0 : BT_ISR
******************************************************************************************
*/
#define DBG_SWDIAG_BT_ISR                          0

#define DBG_SWDIAG_BT_ISR_CLKINT_OFFSET            0
#define DBG_SWDIAG_BT_ISR_CLKINT_MASK              0x01

#define DBG_SWDIAG_BT_ISR_RXINT_OFFSET             1
#define DBG_SWDIAG_BT_ISR_RXINT_MASK               0x02

#define DBG_SWDIAG_BT_ISR_SLPINT_OFFSET            2
#define DBG_SWDIAG_BT_ISR_SLPINT_MASK              0x04

#define DBG_SWDIAG_BT_ISR_GROSSTGTINT_OFFSET       3
#define DBG_SWDIAG_BT_ISR_GROSSTGTINT_MASK         0x08


/**
******************************************************************************************
* @brief BANK 1 : SLEEP
******************************************************************************************
*/
#define DBG_SWDIAG_SLEEP                           1

#define DBG_SWDIAG_SLEEP_SLEEP_OFFSET              0
#define DBG_SWDIAG_SLEEP_SLEEP_MASK                0x01

#define DBG_SWDIAG_SLEEP_ALGO_OFFSET               1
#define DBG_SWDIAG_SLEEP_ALGO_MASK                 0x0E

#define DBG_SWDIAG_SLEEP_PREVENT_OFFSET            4
#define DBG_SWDIAG_SLEEP_PREVENT_MASK              0xF0


/**
******************************************************************************************
* @brief BANK 2 : ISR
******************************************************************************************
*/
#define DBG_SWDIAG_ISR                             2

#define DBG_SWDIAG_ISR_UART_OFFSET                 0
#define DBG_SWDIAG_ISR_UART_MASK                   0x01

#define DBG_SWDIAG_ISR_BT_OFFSET                   1
#define DBG_SWDIAG_ISR_BT_MASK                     0x02

#define DBG_SWDIAG_ISR_BLE_OFFSET                  2
#define DBG_SWDIAG_ISR_BLE_MASK                    0x04

#define DBG_SWDIAG_ISR_APB2DPV_OFFSET              3
#define DBG_SWDIAG_ISR_APB2DPV_MASK                0x08

#define DBG_SWDIAG_ISR_GPIO_OFFSET                 4
#define DBG_SWDIAG_ISR_GPIO_MASK                   0x10

#define DBG_SWDIAG_ISR_RTC0_OFFSET                 5
#define DBG_SWDIAG_ISR_RTC0_MASK                   0x20

#define DBG_SWDIAG_ISR_PS2_OFFSET                  6
#define DBG_SWDIAG_ISR_PS2_MASK                    0x40

#define DBG_SWDIAG_ISR_WFI_OFFSET                  7
#define DBG_SWDIAG_ISR_WFI_MASK                    0x80


/**
******************************************************************************************
* @brief BANK 3 : BLEISR
******************************************************************************************
*/
#define DBG_SWDIAG_BLE_ISR                         3

#define DBG_SWDIAG_BLE_ISR_CSCNTINT_OFFSET         0
#define DBG_SWDIAG_BLE_ISR_CSCNTINT_MASK           0x01

#define DBG_SWDIAG_BLE_ISR_RXINT_OFFSET            1
#define DBG_SWDIAG_BLE_ISR_RXINT_MASK              0x02

#define DBG_SWDIAG_BLE_ISR_SLPINT_OFFSET           2
#define DBG_SWDIAG_BLE_ISR_SLPINT_MASK             0x04

#define DBG_SWDIAG_BLE_ISR_EVENTINT_OFFSET         3
#define DBG_SWDIAG_BLE_ISR_EVENTINT_MASK           0x08

#define DBG_SWDIAG_BLE_ISR_CRYPTINT_OFFSET         4
#define DBG_SWDIAG_BLE_ISR_CRYPTINT_MASK           0x10

#define DBG_SWDIAG_BLE_ISR_ERRORINT_OFFSET         5
#define DBG_SWDIAG_BLE_ISR_ERRORINT_MASK           0x20

#define DBG_SWDIAG_BLE_ISR_GROSSTGTIMINT_OFFSET    6
#define DBG_SWDIAG_BLE_ISR_GROSSTGTIMINT_MASK      0x40

#define DBG_SWDIAG_BLE_ISR_FINETGTIMINT_OFFSET     7
#define DBG_SWDIAG_BLE_ISR_FINETGTIMINT_MASK       0x80


/**
******************************************************************************************
* @brief BANK 8 : EXTAB
******************************************************************************************
*/
#define DBG_SWDIAG_EXTAB                             8

#define DBG_SWDIAG_EXTAB_ACCESS_OFFSET               0
#define DBG_SWDIAG_EXTAB_ACCESS_MASK                 0x03

#define DBG_SWDIAG_EXTAB_CLKINT_OFFSET               2
#define DBG_SWDIAG_EXTAB_CLKINT_MASK                 0x04

#define DBG_SWDIAG_EXTAB_NB_OFFSET                   4
#define DBG_SWDIAG_EXTAB_NB_MASK                     0xF0


/**
******************************************************************************************
* @brief BANK 9 : SWITCH
******************************************************************************************
*/
#define DBG_SWDIAG_SWITCH                            9

#define DBG_SWDIAG_SWITCH_LOOP_OFFSET                0
#define DBG_SWDIAG_SWITCH_LOOP_MASK                  0x01

#define DBG_SWDIAG_SWITCH_SM_OFFSET                  1
#define DBG_SWDIAG_SWITCH_SM_MASK                    0x06

#define DBG_SWDIAG_SWITCH_RSW_FAIL_OFFSET            3
#define DBG_SWDIAG_SWITCH_RSW_FAIL_MASK              0x08


/**
******************************************************************************************
* @brief BANK 11 : VALUE8
******************************************************************************************
*/
#define DBG_SWDIAG_VALUE8                            11

#define DBG_SWDIAG_VALUE8_VALUE_OFFSET               0
#define DBG_SWDIAG_VALUE8_VALUE_MASK                 0xFF

/**
******************************************************************************************
* @brief BANK 12 : VALUE16
******************************************************************************************
*/
#define DBG_SWDIAG_VALUE16                           12

#define DBG_SWDIAG_VALUE16_VALUE_OFFSET              0
#define DBG_SWDIAG_VALUE16_VALUE_MASK                0xFF


/**
******************************************************************************************
* @brief BANK 13 : SNIFF
******************************************************************************************
*/
#define DBG_SWDIAG_SNIFF                             13

#define DBG_SWDIAG_SNIFF_STATE_OFFSET                0
#define DBG_SWDIAG_SNIFF_STATE_MASK                  0x07

#define DBG_SWDIAG_SNIFF_PROG_ANCHOR_OFFSET          3
#define DBG_SWDIAG_SNIFF_PROG_ANCHOR_MASK            0x08

#define DBG_SWDIAG_SNIFF_PROG_ATT_OFFSET             4
#define DBG_SWDIAG_SNIFF_PROG_ATT_MASK               0x10

#define DBG_SWDIAG_SNIFF_MANAGE_OFFSET               5
#define DBG_SWDIAG_SNIFF_MANAGE_MASK                 0x20

#define DBG_SWDIAG_SNIFF_PROG_ACL_OFFSET             6
#define DBG_SWDIAG_SNIFF_PROG_ACL_MASK               0x40

#define DBG_SWDIAG_SNIFF_RX_SYNC_OFFSET               7
#define DBG_SWDIAG_SNIFF_RX_SYNC_MASK                 0x80


/**
******************************************************************************************
* @brief BANK 14 : SNIFF_CNT
******************************************************************************************
*/
#define DBG_SWDIAG_SNIFF_CNT                         14

#define DBG_SWDIAG_SNIFF_CNT_CNT_OFFSET              0
#define DBG_SWDIAG_SNIFF_CNT_CNT_MASK                0xFF


/**
******************************************************************************************
* @brief BANK 15 : CHNL_ASSESS (NB_CHNL)
******************************************************************************************
*/
#define DBG_SWDIAG_CHNL_ASSESS                       15

#define DBG_SWDIAG_CHNL_ASSESS_NB_CHNL_OFFSET        0
#define DBG_SWDIAG_CHNL_ASSESS_NB_CHNL_MASK          0x3F

#define DBG_SWDIAG_CHNL_ASSESS_REM_CHNL_OFFSET       6
#define DBG_SWDIAG_CHNL_ASSESS_REM_CHNL_MASK         0x40

#define DBG_SWDIAG_CHNL_ASSESS_ADD_CHNL_OFFSET       7
#define DBG_SWDIAG_CHNL_ASSESS_ADD_CHNL_MASK         0x80

/**
******************************************************************************************
* @brief BANK 16 : ASSESS_MECH
******************************************************************************************
*/
#define DBG_SWDIAG_ASSESS_MECH                       16

#define DBG_SWDIAG_ASSESS_MECH_ATIMER_OFFSET         0
#define DBG_SWDIAG_ASSESS_MECH_ATIMER_MASK           0x01

#define DBG_SWDIAG_ASSESS_MECH_RTIMER_OFFSET         1
#define DBG_SWDIAG_ASSESS_MECH_RTIMER_MASK           0x02

#define DBG_SWDIAG_ASSESS_MECH_BAD_PKT_OFFSET        2
#define DBG_SWDIAG_ASSESS_MECH_BAD_PKT_MASK          0x04

#define DBG_SWDIAG_ASSESS_MECH_SYNC_ERR_OFFSET       3
#define DBG_SWDIAG_ASSESS_MECH_SYNC_ERR_MASK         0x08

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Allocate HW banks to SW profiles
extern uint8_t sw_to_hw[DBG_SWDIAG_NB_PROFILES];


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
******************************************************************************************
* @brief Initialize SW profiling module
******************************************************************************************
*/
void dbg_swdiag_init(void);

/**
******************************************************************************************
* @brief Read the SW profiling configuration.
*
* @return SW profile configuration
******************************************************************************************
*/
uint32_t dbg_swdiag_read(void);

/**
******************************************************************************************
* @brief Write the SW profiling configuration.
*
* @param[in] profile  SW profiling configuration to apply
******************************************************************************************
*/
void dbg_swdiag_write(uint32_t profile);

#endif //RW_SWDIAG

/// @} DBGSWDIAG

#endif // DBG_SWDIAG_H_
