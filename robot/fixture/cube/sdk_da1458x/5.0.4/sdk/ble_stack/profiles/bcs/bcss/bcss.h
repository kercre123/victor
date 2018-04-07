/**
 ****************************************************************************************
 *
 * @file bcss.h
 *
 * @brief Header file - Body Composition Service Server.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BCSS_H_
#define _BCSS_H_

/**
 ****************************************************************************************
 * @addtogroup BCSS Body Composition Service Server
 * @ingroup BCS
 * @brief Body Composition Service Server
 * @{
 ****************************************************************************************
 */

/// Body Composition Server Role
#define BLE_BCS_SERVER              1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BCS_SERVER)
#include "prf_types.h"

#include "bcs_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Body Composition Service Attributes Indexes
enum
{
    BCSS_IDX_SVC,

    BCSS_IDX_FEAT_CHAR,
    BCSS_IDX_FEAT_VAL,
    BCSS_IDX_MEAS_CHAR,
    BCSS_IDX_MEAS_VAL,
    BCSS_IDX_MEAS_IND_CFG,

    BCSS_IDX_NB,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
///Body Composition Service Server Environment Variable
struct bcss_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// Service Start HandleVAL
    uint16_t shdl;

    /// Supported features
    uint32_t features;

    /// Last measurement set by the App
    bcs_meas_t *meas;
    /// Features for measurements that needs to be indicated
    uint16_t ind_cont_feat;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct attm_desc bcss_att_db[BCSS_IDX_NB];

/// Body Composition Server Environment Variable
extern struct bcss_env_tag bcss_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BCSS module.
 * This function performs all the initializations of the BCSS module.
 ****************************************************************************************
 */
void bcss_init(void);

/**
 ****************************************************************************************
 * @brief Measurement Value Indication.
 * This function indicates the measurement value.
 ****************************************************************************************
 */
void bcss_indicate(const bcs_meas_t *meas, uint16_t features, uint16_t mtu);

/**
 ****************************************************************************************
 * @brief Send a BCSS_MEAS_VAL_IND_CFM message to the application to inform it about the
 * status of a indication that was send
 ****************************************************************************************
 */
void bcss_ind_cfm_send(uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void bcss_disable(uint16_t conhdl); ;

#endif /* #if (BLE_BCS_SERVER) */

/// @} BCSS

#endif /* _BCSS_H_ */
