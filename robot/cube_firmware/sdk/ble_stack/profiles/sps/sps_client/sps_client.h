/**
 ****************************************************************************************
 *
 * @file sps_client.h
 *
 * @brief Header file - SPS_CLIENT.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef SPS_CLIENT_H_
#define SPS_CLIENT_H_

/**
 ****************************************************************************************
 * @addtogroup SPS_CLIENT 
 * @ingroup Profile
 * @brief StreamData  Profile role: Client
 *
 * The SPS_CLIENT is responsible for sending data over Bluetooth Low Energy
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_SPS_CLIENT)

#include "ke_task.h"
#include "prf_types.h"

/*
 * Defines
 ****************************************************************************************
 */

#define SPS_SERVICE_UUID 				{0xb7, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_SERVER_TX_UUID 				{0xb8, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_SERVER_RX_UUID				{0xba, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_FLOW_CTRL_UUID			 	{0xb9, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

#define SPS_CLIENT_CLEANUP_FNCT        (NULL)

enum
{
    SPS_SERVER_TX_DATA = 0,
    SPS_SERVER_RX_DATA,
    SPS_FLOW_CTRL,
    SPS_CLIENT_MAX
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

//client profile circular buffer principle
struct sps_circular_attributes
{		
		// Index value
		uint8_t idx;
		// atttribute state	
		uint8_t att_state[SPS_CLIENT_MAX-1];
};

/// Service information
struct sps_client_sps_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[SPS_CLIENT_MAX];
};

/// StreamData Host environment variable
struct sps_client_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
	
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
	/// Last service for which something was discovered
    uint8_t last_svc_req[ATT_UUID_128_LEN];
	
    /// Last char. code requested to read.
    uint8_t last_char_code;
    
	/// counter used to check service uniqueness
    uint8_t nb_streamdatrad_svc;
		
    /// StreamData Device Service information recovered from discovery
    struct sps_client_sps_content sps_server;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct sps_client_env_tag **sps_client_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the SPS_CLIENT module.
 * This function performs all the initializations of the SPS_CLIENT module.
 ****************************************************************************************
 */
void sps_client_init(void);

/**
 ****************************************************************************************
 * @brief When all handles received correctly it can send and receive data or flow control else it returns an error
 ****************************************************************************************
 */
void sps_client_enable_cfm_send(struct sps_client_env_tag *sps_client_env, struct prf_con_info *con_info, uint8_t status);

#endif //BLE_SPS_CLIENT

/// @} SPS_CLIENT

#endif // SPS_CLIENT_H_
