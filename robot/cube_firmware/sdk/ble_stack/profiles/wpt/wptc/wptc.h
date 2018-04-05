/**
 ****************************************************************************************
 *
 * @file wptc.h
 *
* @brief WPT Client
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef WPTC_H_
#define WPTC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_WPT_CLIENT)

#include "gattc_task.h"
#include "prf_utils.h"
#include "prf_utils_128.h"
#include "gap.h"
#include "gapc.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Pointer to the connection clean-up function
#define WPTC_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Service information
struct svc_content_wpts
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    /// - PRU Control
    struct prf_char_inf pru_control_charact;
    /// - PTU Static Parameter
    struct prf_char_inf ptu_static_charact;
    /// - PRU Static Parameter
    struct prf_char_inf pru_static_charact;
    /// - PRU Dynamic Pramater
    struct prf_char_inf pru_dynamic_charact;

    /// - PRU Alert Value
    struct prf_char_inf pru_alert_charact;

    struct prf_char_desc_inf alert_conf_desc;

};

/// Wireless Power Transfer Client environment variable
struct wptc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Used to store current discovery mode (eg. discovering primary services,
    /// characteristics, .. etc)
    uint8_t discovery_mode;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    /// Wireless Power Transfer Service information recovered from discovery
    struct svc_content_wpts wpts;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct wptc_env_tag **wptc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the WPTC module.
 * This function performs all the initializations of the WPTC module.
 ****************************************************************************************
 */
void wptc_init(void);

/**
 ****************************************************************************************
 * @brief Send enable confirmation to application.
 * @param status Status of the enable: either OK or error encountered while discovery.
 ****************************************************************************************
 */
void wptc_enable_cfm_send(struct wptc_env_tag *wptc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a WPTC_WR_CHAR_RSP message to the application
 *
 * @param status WPTC Error code.
 ****************************************************************************************
 */
void wptc_write_char_rsp_send(struct wptc_env_tag *wptc_env, uint8_t status);


#endif //BLE_WPT_CLIENT

#endif // WPTC_H_
