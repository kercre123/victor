/**
 ****************************************************************************************
 *
 * @file rwble_hl_config.h
 *
 * @brief Configuration of the BLE protocol stack (max number of supported connections,
 * type of partitioning, etc.)
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef RWBLE_HL_CONFIG_H_
#define RWBLE_HL_CONFIG_H_

#include "user_profiles_config.h"
/**
 ****************************************************************************************
 * @addtogroup ROOT
 * @{
 * @name BLE stack configuration
 * @{
 ****************************************************************************************
 */

/******************************************************************************************/
/* -------------------------   BLE PARTITIONING      -------------------------------------*/
/******************************************************************************************/


/******************************************************************************************/
/* --------------------------   INTERFACES        ----------------------------------------*/
/******************************************************************************************/

#if BLE_EMB_PRESENT
#define EMB_LLM_TASK      TASK_LLM
#define EMB_LLC_TASK      KE_BUILD_ID(TASK_LLC, idx)
#else // BLE_EMB_PRESENT
#define EMB_LLM_TASK      TASK_HCIH
#define EMB_LLC_TASK      TASK_HCIH
#endif // BLE_EMB_PRESENT

#if BLE_APP_PRESENT
#include "arch.h"
#define APP_MAIN_TASK       jump_table_struct[FLAG_FULLEMB_POS]//TASK_APP
#else // BLE_APP_PRESENT
#define APP_MAIN_TASK       jump_table_struct[FLAG_FULLEMB_POS]//TASK_GTL
#endif // BLE_APP_PRESENT

// Host Controller Interface (Host side)
#define BLEHL_HCIH_ITF            HCIH_ITF

/******************************************************************************************/
/* --------------------------   COEX SETUP        ----------------------------------------*/
/******************************************************************************************/

///WLAN coex
#define BLEHL_WLAN_COEX          RW_WLAN_COEX
///WLAN test mode
#define BLEHL_WLAN_COEX_TEST     RW_WLAN_COEX_TEST

/******************************************************************************************/
/* --------------------------   HOST MODULES      ----------------------------------------*/
/******************************************************************************************/

#define BLE_GAPM                    1
#if (BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_GAPC                    1
#define BLE_GAPC_HEAP_ENV_SIZE      (sizeof(struct gapc_env_tag)  + KE_HEAP_MEM_RESERVED)
#else //(BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_GAPC                    0
#define BLE_GAPC_HEAP_ENV_SIZE      0
#endif //(BLE_CENTRAL || BLE_PERIPHERAL)

#if (BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_L2CM      1
#define BLE_L2CC      1
#define BLE_ATTM      1
#define BLE_GATTM     1
#define BLE_GATTC     1
#define BLE_GATTC_HEAP_ENV_SIZE     (sizeof(struct gattc_env_tag)  + KE_HEAP_MEM_RESERVED)
#define BLE_L2CC_HEAP_ENV_SIZE      (sizeof(struct l2cc_env_tag)   + KE_HEAP_MEM_RESERVED)
#else //(BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_L2CM      0
#define BLE_L2CC      0
#define BLE_ATTC                    0
#define BLE_ATTS                    0
#define BLE_ATTM      0
#define BLE_GATTM     0
#define BLE_GATTC     0
#define BLE_GATTC_HEAP_ENV_SIZE     0
#define BLE_L2CC_HEAP_ENV_SIZE      0
#endif //(BLE_CENTRAL || BLE_PERIPHERAL)


#define BLE_SMPM      1
#if (BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_SMPC      1
#define BLE_SMPC_HEAP_ENV_SIZE      (sizeof(struct smpc_env_tag) + KE_HEAP_MEM_RESERVED)
#else //(BLE_CENTRAL || BLE_PERIPHERAL)
#define BLE_SMPC      0
#define BLE_SMPC_HEAP_ENV_SIZE      0
#endif //(BLE_CENTRAL || BLE_PERIPHERAL)


/******************************************************************************************/
/* --------------------------   ATT DB            ----------------------------------------*/
/******************************************************************************************/

#if !defined (BLE_CLIENT_PRF)
   #define  BLE_CLIENT_PRF         0
#endif

#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          0
#endif

//Force ATT parts depending on profile roles or compile options
/// Attribute Client
#if (BLE_CLIENT_PRF || BLE_CENTRAL || defined(CFG_ATTC))
    #define BLE_ATTC                    1
#else
    #define BLE_ATTC                    0
#endif //(BLE_CLIENT_PRF || defined(CFG_ATTC))

/// Attribute Server
#if (BLE_SERVER_PRF || BLE_PERIPHERAL || defined(CFG_ATTS))
    #define BLE_ATTS                    1
    #define BLE_ATTS_HEAP_ENV_SIZE      (sizeof(struct atts_env_tag) + KE_HEAP_MEM_RESERVED)
#else
    #define BLE_ATTS                    0
    #define BLE_ATTS_HEAP_ENV_SIZE      0
#endif //(BLE_SERVER_PRF || defined(CFG_ATTS))

/// Size of the heap
#if (BLE_CENTRAL || BLE_PERIPHERAL)
    /// some heap must be reserved for attribute database
    #if (BLE_ATTS || BLE_ATTC)
        #define BLEHL_HEAP_DB_SIZE                 (3072)
    #else
        #define BLEHL_HEAP_DB_SIZE                 (0)
    #endif /* (BLE_ATTS || BLE_ATTC) */

    #define BLEHL_HEAP_MSG_SIZE                    (256 + 256 * BLE_CONNECTION_MAX)
#else
    #define BLEHL_HEAP_MSG_SIZE                    (256)
    #define BLEHL_HEAP_DB_SIZE                     (0)
#endif /* #if (BLE_CENTRAL || BLE_PERIPHERAL) */


/// Number of BLE Host stack tasks
#define BLE_HOST_TASK_SIZE  ( BLE_GAPM       +  \
                              BLE_GAPC       +  \
                              BLE_L2CM       +  \
                              BLE_L2CC       +  \
                              BLE_SMPM       +  \
                              BLE_SMPC       +  \
                              BLE_ATTM       +  \
                              BLE_ATTC       +  \
                              BLE_ATTS       +  \
                              BLE_GATTM      +  \
                              BLE_GATTC      )


/// Number of BLE profiles tasks
#define BLE_PRF_TASK_SIZE   ( BLE_PROX_MONITOR       +  \
                              BLE_PROX_REPORTER      +  \
                              BLE_BM_SERVER          +  \
                              BLE_BM_CLIENT          +  \
                              BLE_STREAMDATA_DEVICE  +  \
                              BLE_STREAMDATA_HOST    +  \
                              BLE_FINDME_LOCATOR     +  \
                              BLE_FINDME_TARGET      +  \
                              BLE_HT_COLLECTOR       +  \
                              BLE_HT_THERMOM         +  \
                              BLE_DIS_CLIENT         +  \
                              BLE_DIS_SERVER         +  \
                              BLE_BP_COLLECTOR       +  \
                              BLE_BP_SENSOR          +  \
                              BLE_TIP_CLIENT         +  \
                              BLE_TIP_SERVER         +  \
                              BLE_HR_COLLECTOR       +  \
                              BLE_HR_SENSOR          +  \
                              BLE_SP_CLIENT          +  \
                              BLE_SP_SERVER          +  \
                              BLE_BAS_CLIENT        +  \
                              BLE_BAS_SERVER        +  \
                              BLE_HID_DEVICE         +  \
                              BLE_HID_BOOT_HOST      +  \
                              BLE_HID_REPORT_HOST    +  \
                              BLE_GL_COLLECTOR       +  \
                              BLE_GL_SENSOR          +  \
                              BLE_NEB_SERVER         +  \
                              BLE_NEB_CLIENT         +  \
                              BLE_RSC_COLLECTOR      +  \
                              BLE_RSC_SENSOR         +  \
                              BLE_CSC_COLLECTOR      +  \
                              BLE_CSC_SENSOR         +  \
                              BLE_CP_COLLECTOR       +  \
                              BLE_CP_SENSOR          +  \
                              BLE_LN_COLLECTOR       +  \
                              BLE_LN_SENSOR          +  \
                              BLE_AN_CLIENT          +  \
                              BLE_AN_SERVER          +  \
							  BLE_ANC_CLIENT         +  \
                              BLE_PAS_CLIENT         +  \
                              BLE_PAS_SERVER         +  \
                              BLE_ACCEL              +  \
                              BLE_WPT_CLIENT         +  \
                              BLE_UDS_SERVER         +  \
                              BLE_UDS_CLIENT         +  \
                              BLE_BCS_SERVER         +  \
                              BLE_BCS_CLIENT         +  \
                              BLE_WSS_SERVER         +  \
                              BLE_WSS_COLLECTOR      +  \
                              BLE_CTS_SERVER         +  \
                              BLE_CTS_CLIENT)

/// Number of BLE HL tasks
#define BLEHL_TASK_SIZE     BLE_HOST_TASK_SIZE + BLE_PRF_TASK_SIZE
#if (!BLE_HOST_PRESENT)
#define BLEHL_HEAP_ENV_SIZE (10)
#else
/// Size of environment variable needed on BLE Host Stack for one link
#define BLEHL_HEAP_ENV_SIZE ( BLE_GAPC_HEAP_ENV_SIZE       +  \
                              BLE_SMPC_HEAP_ENV_SIZE       +  \
                              BLE_ATTS_HEAP_ENV_SIZE       +  \
                              BLE_GATTC_HEAP_ENV_SIZE      +  \
                              BLE_L2CC_HEAP_ENV_SIZE)
#endif
/// @} BLE stack configuration
/// @} ROOT

#endif // RWBLE_HL_CONFIG_H_
