/**
 ****************************************************************************************
 *
 * @file smpm_task.h
 *
 * @brief Header file - SMPM TASK
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef SMPM_TASK_H_
#define SMPM_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SMPM_TASK Security Manager Protocol Manager Task
 * @ingroup SMPM
 * @brief Handles ALL messages to/from SMPM block.
 *
 * The SMPM task is responsible for all security related functions not related to a
 * specific connection with a peer.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "smpm.h"

#if (RW_BLE_USE_CRYPT)

#include "gapm_task.h"          // GAP Manager Task Definitions
#include "gap.h"                // GAP Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum SMPM instance number
#define SMPM_IDX_MAX                    (0x01)

/**
 * Operation Codes (Mapped on GAPM Operation Codes)
 */
/// Random Address Generation Operation Code
#define SMPM_OP_GEN_RAND_ADDR           (GAPM_GEN_RAND_ADDR)
/// Address Resolution Operation Code
#define SMPM_OP_RESOLV_ADDR             (GAPM_RESOLV_ADDR)
/// Generate Random Number
#define SMPM_OP_GEN_RAND_NB             (GAPM_GEN_RAND_NB)
/// Use Encryption Block
#define SMPM_OP_USE_ENC_BLOCK           (GAPM_USE_ENC_BLOCK)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/**
 * States of SMPM Task
 */
enum
{
    /// IDLE state
    SMPM_IDLE,
    /// BUSY state: Communication with LLM ongoing state
    SMPM_BUSY,

    // Number of defined states.
    SMPM_STATE_MAX
};

/**
 * SMPM block API message IDs
 */
enum
{
    /// Random Address Generation
    SMPM_GEN_RAND_ADDR_CMD          = KE_FIRST_MSG(TASK_SMPM),
    SMPM_GEN_RAND_ADDR_IND,

    /// Address Resolution
    SMPM_RESOLV_ADDR_CMD,
    SMPM_RESOLV_ADDR_IND,

    ///Encryption Toolbox Access
    SMPM_USE_ENC_BLOCK_CMD,
    SMPM_USE_ENC_BLOCK_IND,
    SMPM_GEN_RAND_NB_CMD,
    SMPM_GEN_RAND_NB_IND,

    /// Command Complete Event
    SMPM_CMP_EVT,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref SMPM_GEN_RAND_ADDR_CMD message
struct smpm_gen_rand_addr_cmd
{
    /// Command Operation Code (shall be SMPM_OP_GEN_RAND_ADDR)
    uint8_t operation;
    /// Address Type
    uint8_t addr_type;
    /// Prand value used during generation
    uint8_t prand[SMPM_RAND_ADDR_PRAND_LEN];
};

/// Parameters of the @ref SMPM_GEN_RAND_ADDR_IND message
struct smpm_gen_rand_addr_ind
{
    /// Generated Random Address
    struct bd_addr addr;
};

/// Parameters of the @ref SMPM_RESOLV_ADDR_CMD message
struct smpm_resolv_addr_cmd
{
    /// Command Operation Code (shall be SMPM_OP_RESOLV_ADDR)
    uint8_t operation;
    /// Address to resolve
    struct bd_addr addr;
    /// IRK to use
    struct gap_sec_key irk;
};

/// Parameters of the @ref SMPM_RESOLV_ADDR_IND message
struct smpm_resolv_addr_ind
{
    /// Resolved address
    struct bd_addr addr;
    /// Used IRK
    struct gap_sec_key irk;
};

/// Parameters of the @ref SMPM_CMP_EVT message
struct smpm_cmp_evt
{
    /// Completed Command Operation Code
    uint8_t operation;
    /// Command status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler smpm_state_handler[SMPM_STATE_MAX];
extern const struct ke_state_handler smpm_default_handler;
extern ke_state_t smpm_state[SMPM_IDX_MAX];

#endif // (RW_BLE_USE_CRYPT)

#endif // (SMPM_TASK_H_)

/// @} SMPM_TASK
