/**
 ****************************************************************************************
 *
 * @file smpc_task.h
 *
 * @brief Header file SMPC task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef SMPC_TASK_H_
#define SMPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SMPC_TASK Task
 * @ingroup SMPC
 * @brief Handles ALL messages to/from SMPC block.
 *
 * The SMPC task is responsible for all security protocol and secure connections handling.
 *
 * @{
 ****************************************************************************************
 */

#include "smp_common.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */



#include "gapc_task.h"
#include "gap.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of SMPC task instances
#define SMPC_IDX_MAX           (BLE_CONNECTION_MAX)

/// Pairing Operation Code
#define SMPC_PAIRING           (GAPC_BOND)
/// Security Request Code
#define SMPC_SECURITY          (GAPC_SECURITY_REQ)
/// Start Encryption Operation Code
#define SMPC_START_ENC         (GAPC_ENCRYPT)
/// Data Signing Operation Code
#define SMPC_SIGN              (GAPC_ENCRYPT + 1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// States of SMPC task
enum
{
    /// FREE state
    SMPC_FREE,
    /// IDLE state
    SMPC_IDLE,
    /// BUSY state
    SMPC_BUSY,
    /// Waiting for encryption state
    SMPC_ENCRYPT,

    // Number of defined states.
    SMPC_STATE_MAX,
};

/// SMPC Task Messages
enum
{
    SMPC_CMP_EVT = KE_FIRST_MSG(TASK_SMPC),

    /// Pairing Request
    SMPC_PAIRING_CMD,
    SMPC_PAIRING_REQ_IND,
    SMPC_PAIRING_CFM,
    SMPC_PAIRING_IND,

    /// Security Request
    SMPC_SECURITY_CMD,
    SMPC_SECURITY_REQ_IND,

    /// Encryption
    SMPC_START_ENC_CMD,
    SMPC_START_ENC_REQ_IND,
    SMPC_START_ENC_CFM,
    SMPC_START_ENC_IND,

    /// Data Signing
    SMPC_SIGN_CMD,
    SMPC_SIGN_IND,
    SMPC_SIGN_COUNTER_IND,

    /// Timers
    SMPC_TIMEOUT_TIMER_IND,
    SMPC_TIMEOUT_EVT,
    SMPC_REP_ATTEMPTS_TIMER_IND,
    SMPC_REP_ATTEMPTS_ATTACK_IND,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref SMPC_CMP_EVT message
struct smpc_cmp_evt
{
    /// Operation code
    uint8_t operation;
    /// Command status code
    uint8_t status;
};

#if (BLE_CENTRAL || BLE_PERIPHERAL)

/// Parameters of the @ref SMPC_PAIRING_CMD message
struct smpc_pairing_cmd
{
    /// Operation code (SMPC_PAIRING)
    uint8_t operation;
    /// Pairing Features
    struct gapc_pairing pairing_feat;
};

/// Parameters of the @ref SMPC_PAIRING_REQ_IND message
struct smpc_pairing_req_ind
{
    /// Request type
    uint8_t request;
    /// Data information
    union smpc_pairing_req_data
    {
        /// Master Pairing Authentication (request = SMPC_PAIR_REQ)
        uint8_t auth_req;
        /// LTK Key Size (request = SMPC_LTK_EXCH)
        uint8_t key_size;
        /// TK type (request = SMPC_TK_EXCH)
        uint8_t tk_type;
    } data;
};

/// Parameters of the @ref SMPC_PAIRING_CFM message
struct smpc_pairing_cfm
{
    /// Request type
    uint8_t request;
    /// Request accepted
    uint8_t accept;
    /// Data information
    union smpc_pairing_cfm_data
    {
        /// Pairing Features (request = SMPC_PAIRING_REQ)
        struct gapc_pairing pairing_feat;
        /// LTK (request = SMPC_LTK_EXCH)
        struct gapc_ltk ltk;
        /// CSRK (request = SMPC_CSRK_EXCH)
        struct gap_sec_key csrk;
        /// TK (request = SMPC_TK_EXCH)
        struct gap_sec_key tk;
    } data;
};


/// Parameters of the @ref SMPC_PAIRING_IND message
struct smpc_pairing_ind
{
    /// Bond Information type
    uint8_t info;

    /// Data information
    union smpc_pairing_ind_data
    {
        /// Authentication information (@see gap_auth)
        /// (if info = GAPC_PAIRING_SUCCEED)
        uint8_t auth;
        /// Status  (info = SMPC_PAIRING_REQ)
        uint8_t reason;
        /// LTK     (info = SMPC_LTK_EXCH)
        struct gapc_ltk ltk;
        /// CSRK    (info = SMPC_IRK_EXCH)
        struct gapc_irk irk;
        /// CSRK    (info = SMPC_CSRK_EXCH)
        struct gap_sec_key csrk;
    } data;
};

/// Parameters of the @ref SMPC_SECURITY_CMD message
struct smpc_security_cmd
{
    /// Operation
    uint8_t operation;
    /// Requested Authentication Level
    uint8_t auth;
};

/// Parameters of the @ref SMPC_SECURITY_REQ_IND message
struct smpc_security_req_ind
{
    /// Requested Authentication Level
    uint8_t auth;
};

/// Parameters of the @ref SMPC_START_ENC_CMD message
struct smpc_start_enc_cmd
{
    /// Request type (SMPC_START_ENC)
    uint8_t operation;
    /// LTK, EDIV and Rand value
    struct gapc_ltk ltk;
};

/// Parameters of the @ref SMPC_START_ENC_REQ_IND message
struct smpc_start_enc_req_ind
{
    /// Encryption Diversifier
    uint16_t ediv;
    /// Random Number
    struct rand_nb randnb;
};

/// Parameters of the @ref SMPC_START_ENC_CFM message
struct smpc_start_enc_cfm
{
    /// Indicate if a LTK has been found for the peer device
    bool accept;
    /// Found LTK
    struct gap_sec_key ltk;
};

#endif //(BLE_CENTRAL || BLE_PERIPHERAL)

/// Parameters of the @ref SMPC_SIGN_CMD message
struct smpc_sign_cmd
{
    /// Request type (SMPC_SIGN)
    uint8_t operation;
    /// Generation or Verification (SMPC_SIGN_GEN or SMPC_SIGN_VERIF)
    uint8_t flag;
    /// Data PDU length (Bytes)
    uint16_t byte_len;
    /// Data PDU + SignCounter if generation, Data PDU + SignCounter + MAC if verification
    uint8_t msg[__ARRAY_EMPTY];
};

/// Parameters of the @ref SMPC_SIGN_IND message
struct smpc_sign_ind
{
    /// Generation or Verification (SMPC_SIGN_GEN or SMPC_SIGN_VERIF)
    uint8_t type;
    /// Data PDU length (Bytes)
    uint16_t byte_len;
    /// Data PDU + SignCounter + MAC
    uint8_t signed_msg[__ARRAY_EMPTY];
};

/// Parameters of the @ref SMPC_SIGN_COUNTER_IND message
struct smpc_sign_counter_ind
{
    /// Local SignCounter value
    uint32_t local_sign_counter;
    /// Peer SignCounter value
    uint32_t peer_sign_counter;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler smpc_state_handler[SMPC_STATE_MAX];
extern const struct ke_state_handler smpc_default_handler;
extern ke_state_t smpc_state[SMPC_IDX_MAX];

#endif //(SMPC_TASK_H_)

/// @} SMPC_TASK
