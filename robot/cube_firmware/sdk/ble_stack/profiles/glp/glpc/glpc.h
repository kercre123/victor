/**
 ****************************************************************************************
 *
 * @file glpc.h
 *
 * @brief Header file - Glucose Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _GLPC_H_
#define _GLPC_H_

/**
 ****************************************************************************************
 * @addtogroup GLPC Glucose Profile Collector
 * @ingroup GLP
 * @brief Glucose Profile Collector
 *
 * The GLPC is responsible for providing Glucose Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Glucose Profile Collector.
 *
 * Glucose Profile Collector. (GLPC): A GLPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret Glucose
 * measurement in a way suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */


/// GLPC Glucose Profile Collector role
#define BLE_GL_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_GL_COLLECTOR)
#include "glp_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// 30 seconds record access control point timer
#define GLPC_RACP_TIMEOUT                  0x0BB8

/// Characteristics
enum
{
    /// Glucose Measurement
    GLPC_CHAR_MEAS,
    /// Glucose Measurement Context
    GLPC_CHAR_MEAS_CTX,
    /// Glucose Sensor Feature
    GLPC_CHAR_FEATURE,
    /// Record Access control point
    GLPC_CHAR_RACP,

    GLPC_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Glucose Measurement client config
    GLPC_DESC_MEAS_CLI_CFG,
    /// Glucose Measurement context client config
    GLPC_DESC_MEAS_CTX_CLI_CFG,
    /// Record Access control point client config
    GLPC_DESC_RACP_CLI_CFG,

    GLPC_DESC_MAX,
};

/// Pointer to the connection clean-up function
#define GLPC_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct gls_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Glucose Measurement
    ///  - Glucose Measurement Context
    ///  - Glucose Feature
    ///  - Record Access Control Point
    struct prf_char_inf chars[GLPC_CHAR_MAX];

    /// Descriptor handles:info:
    ///  - Glucose Measurement client cfg
    ///  - Glucose Measurement Context client cfg
    ///  - Record Access Control Point client cfg
    struct prf_char_desc_inf descs[GLPC_DESC_MAX];
};

/// Glucose Profile Collector environment variable
struct glpc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    ///Last Char. for which a read request has been sent
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    ///HTS characteristics
    struct gls_content gls;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern struct glpc_env_tag **glpc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GLPC module.
 * This function performs all the initializations of the GLPC module.
 ****************************************************************************************
 */
void glpc_init(void);

/**
 ****************************************************************************************
 * @brief Send Glucose ATT DB discovery results to GLPC host.
 ****************************************************************************************
 */
void glpc_enable_cfm_send(struct glpc_env_tag *glpc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Glucose measurement value
 *
 * @param[in] packed_meas Glucose measurement value packed
 * @param[out] meas_val Glucose measurement value
 * @param[out] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glpc_unpack_meas_value(uint8_t *packed_meas, struct glp_meas* meas_val,
                              uint16_t* seq_num);


/**
 ****************************************************************************************
 * @brief Unpack Glucose measurement context value
 *
 * @param[in] packed_meas Glucose measurement context value packed
 * @param[out] meas_val Glucose measurement context value
 * @param[out] seq_num Glucose measurement sequence number
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t glpc_unpack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                 struct glp_meas_ctx* meas_ctx_val,
                                 uint16_t* seq_num);



/**
 ****************************************************************************************
 * @brief Pack Record Access Control request
 *
 * @param[out] packed_val Record Access Control Point value packed
 * @param[in] racp_req Record Access Control Request value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glpc_pack_racp_req(uint8_t *packed_val,
                               const struct glp_racp_req* racp_req);


/**
 ****************************************************************************************
 * @brief Unpack Record Access Control response
 *
 * @param[in] packed_val Record Access Control Point value packed
 * @param[out] racp_rsp Record Access Control Response value
 *
 * @return size of packed data
 ****************************************************************************************
 */
uint8_t glpc_unpack_racp_rsp(uint8_t *packed_val,
                                      struct glp_racp_rsp* racp_rsp);


/**
 ****************************************************************************************
 * @brief Check if collector request is possible or not
 *
 * @param[in] conhdl Connection handle.
 * @param[in] atthdl Attribute handle.
 *
 * @return PRF_ERR_OK if request can be performed, error code else.
 ****************************************************************************************
 */
uint8_t glpc_validate_request(struct glpc_env_tag *glpc_env, uint16_t conhdl, uint8_t char_code);

#endif /* (BLE_GL_COLLECTOR) */

/// @} GLPC

#endif /* _GLPC_H_ */
