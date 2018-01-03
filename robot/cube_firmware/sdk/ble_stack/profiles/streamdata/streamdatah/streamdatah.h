/**
 ****************************************************************************************
 *
 * @file streamdatah.h
 *
 * @brief Header file - STREAMDATAH.
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

#ifndef STREAMDATAH_H_
#define STREAMDATAH_H_

/**
 ****************************************************************************************
 * @addtogroup STREAMDATA StreamData Profile
 * @ingroup PROFILE
 * @brief StreamData Profile
 *
 * The STREAMDATA module is the responsible block for implementing the StreamData Profile
 * functionalities in the BLE Host.
 *
 * The StreamData Profile defines the functionality required in a device that can
 * alert the user when the user's personal device moves further away or closer together
 * to another communicating device.
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup STREAMDATAH StreamData Host
 * @ingroup STREAMDATA
 * @brief StreamData Host Profile
 *
 * The STREAMDATAH is responsible for providing StreamData Host Profile functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of a StreamData Host role.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_STREAMDATA_HOST)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */


enum
{
	STREAMDATAD_ENABLE_CHAR,
    STREAMDATAH_D0_CHAR,
    STREAMDATAH_D1_CHAR,
    STREAMDATAH_D2_CHAR,
    STREAMDATAH_D3_CHAR,
    STREAMDATAH_D4_CHAR,
    STREAMDATAH_D5_CHAR,
    STREAMDATAH_D6_CHAR,
    STREAMDATAH_D7_CHAR,
    STREAMDATAH_D8_CHAR,
    STREAMDATAH_D9_CHAR,
    STREAMDATAH_CHAR_MAX,
};

/// Pointer to the connection clean-up function
#define STREAMDATAH_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Service information
struct streamdatah_streamdata_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    
    struct prf_char_inf chars[STREAMDATAH_CHAR_MAX];
};

/// StreamData Host environment variable
struct streamdatah_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
	
    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last service for which something was discovered
    uint16_t last_svc_req;
	
    /// Last char. code requested to read.
    uint8_t last_char_code;
    /// counter used to check service uniqueness
    uint8_t nb_streamdatrad_svc;
	
	uint16_t streamon;
	uint16_t next_attribute_idx;
	uint8_t receive_sequence_number;
	
    /// StreamData Device Service information recovered from discovery
    struct streamdatah_streamdata_content streamdatah;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct streamdatah_env_tag **streamdatah_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the STREAMDATAH module.
 * This function performs all the initializations of the STREAMDATAH module.
 ****************************************************************************************
 */
void streamdatah_init(void);

/**
 ****************************************************************************************
 * @brief Send enable confirmation to application.
 * @param status Status of the enable: either OK or error encountered while discovery.
 ****************************************************************************************
 */
void streamdatah_enable_cfm_send(struct streamdatah_env_tag *streamdatah_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a STREAMDATAH_RD_CHAR_RSP message to the application
 *
 * @param status STREAMDATAH Error code.
 ****************************************************************************************
 */
//void streamdatah_read_char_rsp_send(struct streamdatah_env_tag *streamdatah_env,
//                             struct gatt_read_char_resp *param, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a STREAMDATAH_WR_CHAR_RSP message to the application
 *
 * @param status STREAMDATAH Error code.
 ****************************************************************************************
 */
void streamdatah_write_char_rsp_send(struct streamdatah_env_tag *streamdatah_env, uint8_t status);

#endif //BLE_STREAMDATA_HOST

/// @} STREAMDATAH

#endif // STREAMDATAH_H_
