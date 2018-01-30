/**
 ****************************************************************************************
 *
 * @file sps_server.h
 *
 * @brief SPS Device service attribute declarations
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

#ifndef SPS_SERVER_H_
#define SPS_SERVER_H_

/**
 ****************************************************************************************
 * @addtogroup SPS_SERVER Device
 * @ingroup PROFILE
 * @brief Stremadata Profile
 *
 * The SPS_SERVER module is the responsible block for receiving data an process it send it to the application
 *
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup SPS_SERVER_DEV SPS Device Device
 * @ingroup SPS_SERVER
 * @brief Definition of SPS Device attributes
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "prf_types.h"

#if (BLE_SPS_SERVER)

/*
 * UUID adress
 ****************************************************************************************
 */

// Proprietary UUIDs

#define SPS_SERVICE_UUID 				{0xb7, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_SERVER_TX_UUID 				{0xb8, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_SERVER_RX_UUID				{0xba, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}
#define SPS_FLOW_CTRL_UUID			 	{0xb9, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07}

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct sps_server_env_tag
{
    // connection information
		struct prf_con_info con_info;
		/// Application Task Id
    ke_task_id_t appid;
		/// Connection handle
    uint16_t conhdl;
		/// DB Handle
    uint16_t shdl;

};

/*
 * MACROS
 ****************************************************************************************
*/

#define SPS_SERVER_ENABLE_DESC        "Enable SPS Device"
#define SPS_SERVER_ENABLE_DESC_LEN    17
#define SPS_SERVER_TX_DESC            "Server TX Data"
#define SPS_SERVER_TX_DESC_LEN        14
#define SPS_SERVER_RX_DESC            "Server RX Data"
#define SPS_SERVER_RX_DESC_LEN        14
#define SPS_SERVER_FLOW_CTRL_DESC     "Flow Control"
#define SPS_SERVER_FLOW_CTRL_DESC_LEN 12

/* Serial Port Service Device service */
enum
{
    SPS_PRIM_SVC,
	
    SPS_SERVER_TX_DATA_CHAR,
    SPS_SERVER_TX_DATA_VAL,
    SPS_SERVER_TX_DATA_CFG,
    SPS_SERVER_TX_DATA_DESC,
		
    SPS_SERVER_RX_DATA_CHAR,
    SPS_SERVER_RX_DATA_VAL,
	SPS_SERVER_RX_DATA_CFG,
    SPS_SERVER_RX_DATA_DESC,
  
	SPS_FLOW_CTRL_CHAR,
    SPS_FLOW_CTRL_VAL,
	SPS_FLOW_CTRL_CFG,
	SPS_FLOW_CTRL_DESC,
	
    SPS_SERVER_IDX_NB   
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct sps_server_env_tag sps_server_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init of the SPS device
 ****************************************************************************************
 */
void sps_server_init(void);


#endif /* BLE_SPS_SERVER */

/// @} SPS_SERVER_DEV

#endif // SPS_SERVER_H_
