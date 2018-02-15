/**
 ****************************************************************************************
 *
 * @file bass.h
 *
 * @brief Header file - Battery Service Server Role
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _BASS_H_
#define _BASS_H_

/**
 ****************************************************************************************
 * @addtogroup BAPS Battery 'Profile' Server
 * @ingroup BAP
 * @brief Battery 'Profile' Server
 * @{
 ****************************************************************************************
 */

/// Battery Profile Server Role
#define BLE_BAS_SERVER         1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BAS_SERVER)

#include "prf_types.h"
#include "attm.h"
//#include "atts.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximal number of BAS that can be added in the DB
#ifndef USE_ONE_BAS_INSTANCE
#define BASS_NB_BAS_INSTANCES_MAX         (2)
#else
#define BASS_NB_BAS_INSTANCES_MAX         (1)
#endif

#define BAS_CFG_FLAG_MANDATORY_MASK       (0x07)
#define BAS_CFG_FLAG_NTF_SUP_MASK         (0x08)
#define BAS_CFG_FLAG_MTP_BAS_MASK         (0x10)

#define BAS_BATTERY_LVL_MAX               (100)

#define BASS_FLAG_NTF_CFG_BIT             (0x02)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Battery Service Attributes Indexes
enum
{
    BAS_IDX_SVC,

    BAS_IDX_BATT_LVL_CHAR,
    BAS_IDX_BATT_LVL_VAL,
    BAS_IDX_BATT_LVL_NTF_CFG,
    BAS_IDX_BATT_LVL_PRES_FMT,

    BAS_IDX_NB,
};

/// Features Flag Masks
enum
{
    /// Battery Level Characteristic doesn't support notifications
    BAS_BATT_LVL_NTF_NOT_SUP,
    /// Battery Level Characteristic support notifications
    BAS_BATT_LVL_NTF_SUP,
};

/// Pointer to the connection clean-up function
#define BASS_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Battery 'Profile' Server environment variable
struct bass_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// BAS Start Handles
    uint16_t shdl[BASS_NB_BAS_INSTANCES_MAX];
    /// Database features
    uint8_t features[BASS_NB_BAS_INSTANCES_MAX];
    /// Number of BAS
    uint8_t bas_nb;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Full BAS Database Description
extern const struct attm_desc bas_att_db[BAS_IDX_NB];

///Battery Service
extern const att_svc_desc_t bas_svc;
///Battery Level Service Battery Level Characteristic
extern const struct att_char_desc bas_batt_level_char;

/// Battery 'Profile' Server Environment
extern struct bass_env_tag bass_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BAPS module.
 * This function performs all the initializations of the BAPS module.
 ****************************************************************************************
 */
void bass_init(void);

/**
 ****************************************************************************************
 * @brief Send a BASS_BATT_LEVEL_UPD_CFM message to the application to inform it about the
 * status of a notification sending
 ****************************************************************************************
 */
void bass_batt_level_upd_cfm_send(uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void bass_disable(uint16_t conhdl); ;

#endif /* #if (BLE_BAS_SERVER) */

/// @} BASS

#endif /* _BASS_H_ */
