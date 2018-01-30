/**
 ****************************************************************************************
 *
 * @file spotar.h
 *
 * @brief Header file - SPOTAR.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef SPOTAR_H_
#define SPOTAR_H_

/**
 ****************************************************************************************
 * @addtogroup SPOTAR SPOTA Receiver
 * @ingroup SPOTAR
 * @brief SPOTA Profile Reciver.
 *
 * The SPOTAR is responsible for providing SPOTA reporter functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a SPOTA Receiver role.
 * @{
 ****************************************************************************************
 */
 
/// Software Patching Over the Air Receiver Role
#define BLE_SPOTA_RECEIVER      1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SPOTA_RECEIVER)

#include "ke_task.h"
//#include "atts.h"
#include "prf_types.h"

/*
 * SPOTAR defines
 ****************************************************************************************
 */
 
#define SPOTA_PD_CHAR_SIZE      20
#define SPOTA_NEW_PD_SIZE       0x50
#define SPOTA_OVERALL_PD_SIZE   0x100

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// SPOTA Handles offsets
enum
{
    SPOTA_IDX_SVC,

    SPOTA_IDX_PATCH_MEM_DEV_CHAR,
    SPOTA_IDX_PATCH_MEM_DEV_VAL,
//    SPOTA_IDX_PATCH_MEM_DEV_VAL_DESCR,
    
    SPOTA_IDX_GPIO_MAP_CHAR,
    SPOTA_IDX_GPIO_MAP_VAL,
//    SPOTA_IDX_GPIO_MAP_VAL_DESCR,
        
    SPOTA_IDX_PATCH_MEM_INFO_CHAR,
    SPOTA_IDX_PATCH_MEM_INFO_VAL,
//    SPOTA_IDX_PATCH_MEM_INFO_VAL_DESCR,
    
    SPOTA_IDX_PATCH_LEN_CHAR,
    SPOTA_IDX_PATCH_LEN_VAL,
//    SPOTA_IDX_PATCH_LEN_VAL_DESCR,
    
    SPOTA_IDX_PATCH_DATA_CHAR,
    SPOTA_IDX_PATCH_DATA_VAL,
//    SPOTA_IDX_PATCH_DATA_VAL_DESCR,
        
    SPOTA_IDX_PATCH_STATUS_CHAR,
    SPOTA_IDX_PATCH_STATUS_VAL,
    SPOTA_IDX_PATCH_STATUS_NTF_CFG,
//    SPOTA_IDX_PATCH_STATUS_VAL_DESCR,
        
    SPOTA_IDX_NB,
};



///Characteristics Code for Write Indications
enum
{
    SPOTAR_ERR_CHAR,
    SPOTAR_PATCH_MEM_DEV_CHAR,
    SPOTAR_GPIO_MAP_CHAR,
    SPOTAR_PATCH_LEN_CHAR,
    SPOTAR_PATCH_DATA_CHAR,
    SPOTA_PATCH_STATUS_NTF_CFG,    
};

///Alert Level Values, to blink LED when SPOTA is active
enum
{
    SPOTAR_ALERT_NONE,
    SPOTAR_ALERT_MILD,
    SPOTAR_ALERT_HIGH,
};

/// SPOTA Status values
enum
{
    // Value zero must not be used !! Notifications are sent when status changes.
    SPOTAR_SRV_STARTED      = 0x01,     // Valid memory device has been configured by initiator. No sleep state while in this mode
    SPOTAR_CMP_OK           = 0x02,     // SPOTA process completed successfully. 
    SPOTAR_SRV_EXIT         = 0x03,     // Forced exit of SPOTAR service. 
    SPOTAR_CRC_ERR          = 0x04,     // Overall Patch Data CRC failed
    SPOTAR_PATCH_LEN_ERR    = 0x05,     // Received patch Length not equal to PATCH_LEN characteristic value
    SPOTAR_EXT_MEM_WRITE_ERR= 0x06,     // External Mem Error (Writing to external device failed)
    SPOTAR_INT_MEM_ERR      = 0x07,     // Internal Mem Error (not enough space for Patch)
    SPOTAR_INVAL_MEM_TYPE   = 0x08,     // Invalid memory device
    SPOTAR_APP_ERROR        = 0x09,     // Application error
    
    // SUOTAR application specific error codes
    SPOTAR_IMG_STARTED      = 0x10,     // SPOTA started for downloading image (SUOTA application)
    SPOTAR_INVAL_IMG_BANK   = 0x11,     // Invalid image bank
    SPOTAR_INVAL_IMG_HDR    = 0x12,     // Invalid image header
    SPOTAR_INVAL_IMG_SIZE   = 0x13,     // Invalid image size
    SPOTAR_INVAL_PRODUCT_HDR= 0x14,     // Invalid product header
    SPOTAR_SAME_IMG_ERR     = 0x15,     // Same Image Error
    SPOTAR_EXT_MEM_READ_ERR = 0x16,     // Failed to read from external memory device
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// SPOTA receiver environment variable
struct spotar_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// SPOTA Start Handle
    uint16_t spota_shdl;    
};

/*
 * SPOTA PROFILE ATTRIBUTES DECLARATION
 ****************************************************************************************
 */

/// SPOTA Database Description
extern const struct attm_desc spotar_att_db[SPOTA_IDX_NB];


/*
 *  SPOTA PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

extern const uint8_t spotar_service[];

/// SPOTA Service - Patch Memory Device Characteristic
extern struct att_char128_desc spotar_patch_mem_dev_char;
extern const struct att_uuid_128 spotar_patch_mem_dev_val;

/// SPOTA Service - GPIO mapping Characteristic
extern struct att_char128_desc spotar_gpio_map_char;
extern const struct att_uuid_128 spotar_gpio_map_val;

/// SPOTA Service - Patch Memory Device Information Characteristic
extern struct att_char128_desc spotar_patch_mem_info_char;
extern const struct att_uuid_128 spotar_patch_mem_info_val;

/// SPOTA Service - Patch Length Characteristic
extern struct att_char128_desc spotar_patch_len_char;
extern const struct att_uuid_128 spotar_patch_len_val;

/// SPOTA Service - Patch Data Characteristic
extern struct att_char128_desc spotar_patch_data_char;
extern const struct att_uuid_128 spotar_patch_data_val;

/// SPOTA Service - Patch Status Characteristic
extern struct att_char128_desc spotar_patch_status_char;
extern const struct att_uuid_128 spotar_patch_status_val;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct spotar_env_tag spotar_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the SPOTAR module.
 * This function performs all the initializations of the SPOTAR module.
 ****************************************************************************************
 */
void spotar_init(void);

/**
 ****************************************************************************************
 * @brief Inform app that pached data have been received.
 ****************************************************************************************
 */
void spotar_patched_data_rcved(uint8_t char_code);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void spotar_disable(uint16_t conhdl);

#endif //BLE_SPOTA_RECEIVER

/// @} SPOTAR

#endif // SPOTAR_H_
