/**
 ****************************************************************************************
 *
 * @file rwble_config.h
 *
 * @brief Configuration of the BLE protocol stack (max number of supported connections,
 * type of partitioning, etc.)
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef RWBLE_CONFIG_H_
#define RWBLE_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup ROOT
 * @{
 * @name BLE stack configuration
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"

/******************************************************************************************/
/* -------------------------   INTERFACES DEFINITIONS      -------------------------------*/
/******************************************************************************************/
#ifndef __DA14581__
#if BLE_HOST_PRESENT
#define HOST_TESTER_TASK   TASK_GTL
#define HOST_GAP_TASK      TASK_GAPM
#define HOST_L2CC_TASK     KE_BUILD_ID(TASK_L2CC, conhdl)
#define HOST_SMPC_TASK     KE_BUILD_ID(TASK_SMPC, conhdl)
#else // BLE_HOST_PRESENT
#define HOST_TESTER_TASK   TASK_HCI
#define HOST_GAP_TASK      TASK_HCI
#define HOST_L2CC_TASK     TASK_HCI
#define HOST_SMPC_TASK     TASK_HCI
#endif // BLE_HOST_PRESENT
#else
#define HOST_TESTER_TASK   jump_table_struct[host_tester_task_pos]
#define HOST_GAP_TASK      jump_table_struct[host_gap_task_pos]
#define HOST_L2CC_TASK     (jump_table_struct[host_tester_task_pos]==TASK_HCI)? TASK_HCI : KE_BUILD_ID(TASK_L2CC, conhdl)
#define HOST_SMPC_TASK     (jump_table_struct[host_tester_task_pos]==TASK_HCI)? TASK_HCI : KE_BUILD_ID(TASK_SMPC, conhdl)
#endif

/// BLE HCI Controller interface
#define BLE_HCIC_ITF           (RW_BLE_SUPPORT && HCIC_ITF)

/******************************************************************************************/
/* -------------------------   COEXISTENCE SETUP      ------------------------------------*/
/******************************************************************************************/

///WLAN coex
#define RW_BLE_WLAN_COEX        RW_WLAN_COEX
///WLAN test mode
#define RW_BLE_WLAN_COEX_TEST   RW_WLAN_COEX_TEST

/******************************************************************************************/
/* --------------------------   DEBUG SETUP       ----------------------------------------*/
/******************************************************************************************/

/// Flag indicating if debug mode is activated or not

#define BLE_DEBUG          1 // RW_DEBUG

#define BLE_SWDIAG              RW_SWDIAG

/******************************************************************************************/
/* -------------------------   BLE SETUP      --------------------------------------------*/
/******************************************************************************************/

/// Exchange memory presence
#define BLE_EM_PRESENT              BLE_EMB_PRESENT

#define BLE_TEST_MODE_SUPPORT           1

/// Number of devices in the white list
#define BLE_WHITELIST_MAX               (jump_table_struct[nb_links_user]  + 2)

/// Number of devices capacity for the scan filtering
#if (BLE_CENTRAL || BLE_OBSERVER)
    #define BLE_DUPLICATE_FILTER_MAX    10
#endif // BLE_OBSERVER

/** Size of the heap
 * - For KE messages: (N+1) x 256
 * - For LLC environment: N x 80 Bytes
 * - For LLD events/intervals: (2N+1) x (80 + 16)
 */
#if (BLE_CENTRAL || BLE_PERIPHERAL)
    /// Size required to allocate environment variable for one link
    #define BLE_HEAP_ENV_SIZE               (sizeof(struct llc_env_tag) + 4)
#else
    /// Size required to allocate environment variable for one link
    #define BLE_HEAP_ENV_SIZE               (4)
#endif /* #if (BLE_CENTRAL || BLE_PERIPHERAL) */

/// Number of BLE tasks
#define BLE_TASK_SIZE              3

/******************************************************************************************/
/* --------------------------   RADIO SETUP       ----------------------------------------*/
/******************************************************************************************/
/// Class of device
#define RF_CLASS1                            0 

/******************************************************************************************/
/* --------------------------   REGISTER SETUP       --------------------------------------*/
/******************************************************************************************/

#define RW_BLE_CUST1_INST                    0

/// @} BLE stack configuration
/// @} ROOT

#endif // RWBLE_CONFIG_H_
