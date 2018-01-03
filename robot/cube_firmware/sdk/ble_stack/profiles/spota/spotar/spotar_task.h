/**
 ****************************************************************************************
 *
 * @file spotar_task.h
 *
 * @brief Header file - SPOTARTASK.
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

#ifndef SPOTAR_TASK_H_
#define SPOTAR_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SPOTARTASK SPOTA Receiver Task
 * @ingroup SPOTAR
 * @brief SPOTA Reciver Task
 *
 * The SPOTARTASK is responsible for handling the API messages from the Application or internal
 * tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SPOTA_RECEIVER)

#include <stdint.h>
#include "ke_task.h"
#include "spotar.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of SPOTA Receiver task instances
#define SPOTAR_IDX_MAX                 (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the SPOTAR task
enum
{
    /// Disabled State
    SPOTAR_DISABLED,
    /// Idle state
    SPOTAR_IDLE,
    /// Connected state
    SPOTAR_ACTIVE,

    /// Number of defined states.
    SPOTAR_STATE_MAX,
};


/// Messages for SPOTA Receiver
enum
{
    /// Start the SPOTA Receiver
    SPOTAR_ENABLE_REQ = KE_FIRST_MSG(TASK_SPOTAR),

    /// Disable confirm will allow informing the APP to dealocate Patched 
    /// Data dynamically allocated mem space
    SPOTAR_DISABLE_IND,

    // Patch Memory Device type indication
    SPOTAR_PATCH_MEM_DEV_IND,
    
    // GPIO map type indication
    SPOTAR_GPIO_MAP_IND,
    
    /// Patch Data length Indication
    SPOTAR_PATCH_LEN_IND,
    
    /// New Patch Data Indication
    SPOTAR_PATCH_DATA_IND,    

    // Request to update memory info in the db
    SPOTAR_PATCH_MEM_INFO_UPDATE_REQ,
    
    // Request to update status info in the db
    SPOTAR_STATUS_UPDATE_REQ,
    
    ///Add a LLS instance and optionally a TXPS instance into the database
    SPOTAR_CREATE_DB_REQ,
    
    ///Inform APP of database creation status
    SPOTAR_CREATE_DB_CFM,

    /// Error Indication
    SPOTAR_ERROR_IND,
};


/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref SPOTAR_ENABLE_REQ message
struct spotar_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;
    /// MSbyte indicates Memory device to apply patch. The rest of the word is the base address of the patch area
    uint32_t mem_dev;
    /// Most sicnificant word (MSW) is the existing patch size, LSW is the entire memory 
    uint32_t  patch_mem_info;    
    uint16_t  patch_len ;
    uint8_t   patch_status;
};

/// Parameters of the @ref SPOTAR_DISABLE_IND message
struct spotar_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// remaining mem to save in APP
    uint8_t  pd_mem_size;
};

/// Parameters of the @ref SPOTAR_PATCH_DATA_IND message
struct spotar_patch_data_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Char Code - Indicate whether patch data written successfully
    uint8_t char_code;
    
    uint8_t len;
    
    uint8_t pd[SPOTA_PD_CHAR_SIZE];
};

/// Parameters of the @ref SPOTAR_PATCH_MEM_DEV_IND message
struct spotar_patch_mem_dev_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Char Code - Indicate whether patch data written successfully
    uint8_t char_code;
    
    uint32_t mem_dev;
};

/// Parameters of the @ref SPOTAR_GPIO_MAP_IND message
struct spotar_gpio_map_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Char Code - Indicate whether patch data written successfully
    uint8_t char_code;
    
    uint32_t gpio_map;
};

/// Parameters of the @ref SPOTAR_PATCH_LEN_IND message
struct spotar_patch_len_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Char Code - Indicate whether patch data written successfully
    uint8_t char_code;
    
    uint16_t len;
};

/// Parameters of the @ref SPOTAR_STATUS_UPDATE_REQ message
struct spotar_status_upadet_req
{
    /// Connection handle
    uint16_t conhdl;
    
    /// SPOTAR Status
    uint8_t status;
};

/// Parameters of the @ref SPOTAR_PATCH_MEM_INFO_UPDATE_REQ message
struct spotar_patch_mem_info_upadet_req
{
    /// Memory info: 16MSbits show number of patches, 16LSbits overall mem len
    uint32_t mem_info;
};

/// Parameters of the @ref SPOTAR_CREATE_DB_REQ message
struct spotar_create_db_req
{
    /// Indicate if ext mem is supported or not
    uint8_t features;
};

/// Parameters of the @ref SPOTAR_CREATE_DB_CFM message
struct spotar_create_db_cfm
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler spotar_state_handler[SPOTAR_STATE_MAX];
extern const struct ke_state_handler spotar_default_handler;
extern ke_state_t spotar_state[SPOTAR_IDX_MAX];

#endif //BLE_SPOTA_RECEIVER

/// @} SPOTARTASK

#endif // SPOTAR_TASK_H_
