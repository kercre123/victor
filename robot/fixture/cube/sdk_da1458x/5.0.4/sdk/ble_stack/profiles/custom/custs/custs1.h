/**
 ****************************************************************************************
 *
 * @file custs1.h
 *
 * @brief Custom Service profile header file.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _CUSTS1_H_
#define _CUSTS1_H_

/// Device Information Service Server Role
#define BLE_CUSTOM1_SERVER          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif 
#if !defined (BLE_CUSTOM_SERVER)
    #define BLE_CUSTOM_SERVER       1
#endif

#if (BLE_CUSTOM1_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "prf_types.h"
#include "attm_db_128.h"

//#include "custom_common.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// custs environment variable
struct custs1_env_tag
{
    /// Connection Information. DO NOT MOVE MUST BE FIRST!!
    struct prf_con_info con_info;
    /// Service Start Handle
    uint16_t shdl;
    /// To store the DB max number of attributes 
    uint8_t max_nb_att;
    /// Last nft request sent by the profile
    uint16_t ntf_handle;
    /// Last  request sent by the profile
    uint16_t ind_handle;

};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct custs1_env_tag custs1_env;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the 1st CUSTOM Service.
 * This function performs all the initializations of the CUSTOM Service.
 ****************************************************************************************
 */
void custs1_init(void);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 * @param[in] conhdl    Connection Handle
 ****************************************************************************************
 */
void custs1_disable(uint16_t conhdl);

#endif // (BLE_CUSTOM1_SERVER)

#endif // _CUSTS1_H_
