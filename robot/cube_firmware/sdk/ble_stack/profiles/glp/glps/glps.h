/**
 ****************************************************************************************
 *
 * @file glps.h
 *
 * @brief Header file - Glucose Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _GLPS_H_
#define _GLPS_H_

/**
 ****************************************************************************************
 * @addtogroup GLPS Glucose Profile Sensor
 * @ingroup GLP
 * @brief Glucose Profile Sensor
 *
 * Glucose sensor (GLS) profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Glucose sensor.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Notify peer device during Glucose measurement (from APP)
 *  - Indicate measurements performed to peer device (from APP)
 *
 * Profile didn't manages multiple users configuration and storage of offline measurements.
 * This must be handled by application.
 *
 * Glucose Profile Sensor. (GLPS): A GLPS (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can perform Glucose
 * measurement and notify about on-going measurement and indicate final result to a peer
 * BLE device.
 *
 * @{
 ****************************************************************************************
 */

/// GLPS Glucose Profile Sensor Role
#define BLE_GL_SENSOR           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_GL_SENSOR)

#include "glp_common.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */


///GLS Configuration Flag Masks
#define GLPS_MANDATORY_MASK                (0x1F8F)
#define GLPS_MEAS_CTX_PRES_MASK            (0x0070)

/*
 * MACROS
 ****************************************************************************************
 */

/// check if flag enable
#define GLPS_IS(FLAG) ((glps_env.flags & (1<< (GLPS_##FLAG))) == (1<< (GLPS_##FLAG)))

/// Set flag enable
#define GLPS_SET(FLAG) (glps_env.flags |= (1<< (GLPS_##FLAG)))

/// Set flag disable
#define GLPS_CLEAR(FLAG) (glps_env.flags &= ~(1<< (GLPS_##FLAG)))


/// Get database attribute handle
#define GLPS_HANDLE(idx) \
    (glps_env.shdl + (idx) - \
        ((!(GLPS_IS(MEAS_CTX_SUPPORTED)) && ((idx) > GLS_IDX_MEAS_CTX_NTF_CFG))? (3) : (0)))

/// Get database attribute index
#define GLPS_IDX(hdl) \
    (((((hdl) - glps_env.shdl) > GLS_IDX_MEAS_CTX_NTF_CFG) && !(GLPS_IS(MEAS_CTX_SUPPORTED))) ?\
        ((hdl) - glps_env.shdl + 3) : ((hdl) - glps_env.shdl))

/// Get event Indication/notification configuration bit
#define GLPS_IND_NTF_EVT(idx) (1 << ((idx - 1) / 3))
/*
 * ENUMS
 ****************************************************************************************
 */

///Attributes State Machine
enum
{
    /// Glucose Service
    GLS_IDX_SVC,
    /// Glucose Measurement
    GLS_IDX_MEAS_CHAR,
    GLS_IDX_MEAS_VAL,
    GLS_IDX_MEAS_NTF_CFG,
    /// Glucose Measurement Context
    GLS_IDX_MEAS_CTX_CHAR,
    GLS_IDX_MEAS_CTX_VAL,
    GLS_IDX_MEAS_CTX_NTF_CFG,
    ///Glucose Feature
    GLS_IDX_FEATURE_CHAR,
    GLS_IDX_FEATURE_VAL,
    ///Record Access Control Point
    GLS_IDX_REC_ACCESS_CTRL_CHAR,
    GLS_IDX_REC_ACCESS_CTRL_VAL,
    GLS_IDX_REC_ACCESS_CTRL_IND_CFG,

    GLS_IDX_NB,
};

///Characteristic Codes
enum
{
    /// Glucose Measurement
    GLS_MEAS_CHAR,
    /// Glucose Measurement Context
    GLS_MEAS_CTX_CHAR,
    /// Glucose Feature
    GLS_FEATURE_CHAR,
    /// Record Access Control Point
    GLS_REC_ACCESS_CTRL_CHAR,
};

///Database Configuration Bit Field Flags
enum
{
    /// support of Glucose Measurement Context
    GLPS_MEAS_CTX_SUP        = 0x01,
};

/// Indication/notification configuration (put in feature flag to optimize memory usage)
enum
{
    /// Bit used to know if Glucose measurement notification is enabled
    GLPS_MEAS_NTF_CFG      = GLPS_IND_NTF_EVT(GLS_IDX_MEAS_NTF_CFG),
    /// Bit used to know if Glucose measurement context notification is enabled
    GLPS_MEAS_CTX_NTF_CFG  = GLPS_IND_NTF_EVT(GLS_IDX_MEAS_CTX_NTF_CFG),
    /// Bit used to know if Glucose measurement context indication is enabled
    GLPS_RACP_IND_CFG      = GLPS_IND_NTF_EVT(GLS_IDX_REC_ACCESS_CTRL_IND_CFG),
};

/// Type of request completed
enum
{
    /// Glucose measurement notification sent completed
    GLPS_SEND_MEAS_REQ_NTF_CMP,
    /// Record Access Control Point Response Indication
    GLPS_SEND_RACP_RSP_IND_CMP
};

/// Glucose service processing flags
enum
{
    /// Glucose Profile Enable
    GLPS_ENABLE,
    /// Measurement context supported
    GLPS_MEAS_CTX_SUPPORTED,
    /// Flag used to know if there is an on-going record access control point request
    GLPS_RACP_ON_GOING,
    /// Service id sending a measurement
    GLPS_SENDING_MEAS,
    /// Measurement sent
    GLPS_MEAS_SENT,
    /// Measurement context sent
    GLPS_MEAS_CTX_SENT,
};

/// Pointer to the connection clean-up function
#define GLPS_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */


/// Glucose Profile Sensor environment variable
struct glps_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    ///Glucose Service Start Handle
    uint16_t shdl;

    ///Application that requests to send RACP response indication.
    ke_task_id_t racp_ind_src;

    ///Glucose service processing flags
    uint8_t flags;

    ///Event (notification/indication) configuration
    uint8_t evt_cfg;
};



/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
/// Glucose sensor environment variable
extern struct glps_env_tag glps_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GLPS module.
 * This function performs all the initializations of the GLPS module.
 ****************************************************************************************
 */
void glps_init(void);

/**
 ****************************************************************************************
 * @brief Pack Glucose measurement value
 *
 * @param[out] packed_meas Glucose measurement value packed
 * @param[in] meas_val Glucose measurement value
 * @param[in] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glps_pack_meas_value(uint8_t *packed_meas, const struct glp_meas* meas_val,
                             uint16_t seq_num);


/**
 ****************************************************************************************
 * @brief Pack Glucose measurement context value
 *
 * @param[out] packed_meas Glucose measurement context value packed
 * @param[in] meas_val Glucose measurement context value
 * @param[in] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glps_pack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                 const struct glp_meas_ctx* meas_ctx_val,
                                 uint16_t seq_num);



/**
 ****************************************************************************************
 * @brief Unpack Record Access Control request
 *
 * @param[in] packed_val Record Access Control Point value packed
 * @param[out] racp_req Record Access Control Request value
 *
 * @return status of unpacking
 ****************************************************************************************
 */
uint8_t glps_unpack_racp_req(uint8_t *packed_val, uint16_t length,
                             struct glp_racp_req* racp_req);


/**
 ****************************************************************************************
 * @brief Pack Record Access Control response
 *
 * @param[out] packed_val Record Access Control Point value packed
 * @param[in] racp_rsp Record Access Control Response value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glps_pack_racp_rsp(uint8_t *packed_val,
                           struct glp_racp_rsp* racp_rsp);

/**
 ****************************************************************************************
 * @brief Send Record Access Control Response Indication
 *
 * @param[in] racp_rsp Record Access Control Response value
 * @param[in] racp_ind_src Application that requests to send RACP response indication.
 *
 * @return PRF_ERR_IND_DISABLED if indication disabled by peer device, PRF_ERR_OK else.
 ****************************************************************************************
 */
uint8_t glps_send_racp_rsp(struct glp_racp_rsp * racp_rsp,
                           ke_task_id_t racp_ind_src);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 *
 * @param[in] status Reason why Disable request
 ****************************************************************************************
 */
void glps_disable(uint8_t status, uint16_t conhdl);

#endif /* #if (BLE_GL_SENSOR) */

/// @} GLPS

#endif /* _GLPS_H_ */
