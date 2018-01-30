/**
 ****************************************************************************************
 *
 * @file aes_task.h
 *
 * @brief Header file - aes TASK
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef AES_TASK_H_
#define AES_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup aes_TASK Security Manager Protocol Manager Task
 * @ingroup aes
 * @brief Handles ALL messages to/from aes block.
 *
 * The aes task is responsible for all security related functions not related to a
 * specific connection with a peer.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "aes.h"

#if (USE_AES)

#include "gapm_task.h"          // GAP Manager Task Definitions
#include "gap.h"                // GAP Definitions

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum aes instance number
#define AES_IDX_MAX                    (0x01)

/**
 * Operation Codes (Mapped on GAPM Operation Codes)
 */
/// Random Address Generation Operation Code
#define AES_OP_GEN_RAND_ADDR           (GAPM_GEN_RAND_ADDR)
/// Address Resolution Operation Code
#define AES_OP_RESOLV_ADDR             (GAPM_RESOLV_ADDR)
/// Generate Random Number
#define AES_OP_GEN_RAND_NB             (GAPM_GEN_RAND_NB)
/// Use Encryption Block
#define AES_OP_USE_ENC_BLOCK           (GAPM_USE_ENC_BLOCK)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/**
 * States of aes Task
 */
enum aes_state_id
{
    /// IDLE state
    AES_IDLE,
    /// BUSY state: Communication with LLM ongoing state
    AES_BUSY,

    // Number of defined states.
    AES_STATE_MAX
};

/**
 * aes block API message IDs
 */
enum aes_msg_id
{
    /// Random Address Generation
    AES_GEN_RAND_ADDR_CMD          = KE_FIRST_MSG(TASK_AES),
    AES_GEN_RAND_ADDR_IND,

    /// Address Resolution
    AES_RESOLV_ADDR_CMD,
    AES_RESOLV_ADDR_IND,

    ///Encryption Toolbox Access
    AES_USE_ENC_BLOCK_CMD,
    AES_USE_ENC_BLOCK_IND,
    AES_GEN_RAND_NB_CMD,
    AES_GEN_RAND_NB_IND,

    /// Command Complete Event
    AES_CMP_EVT
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref aes_GEN_RAND_ADDR_CMD message
struct aes_gen_rand_addr_cmd
{
    /// Command Operation Code (shall be aes_OP_GEN_RAND_ADDR)
    uint8_t operation;
    /// Address Type
    uint8_t addr_type;
    /// Prand value used during generation
    uint8_t prand[AES_RAND_ADDR_PRAND_LEN];
};

/// Parameters of the @ref aes_GEN_RAND_ADDR_IND message
struct aes_gen_rand_addr_ind
{
    /// Generated Random Address
    struct bd_addr addr;
};

/// Parameters of the @ref aes_RESOLV_ADDR_CMD message
struct aes_resolv_addr_cmd
{
    /// Command Operation Code (shall be aes_OP_RESOLV_ADDR)
    uint8_t operation;
    /// Address to resolve
    struct bd_addr addr;
    /// IRK to use
    struct gap_sec_key irk;
};

/// Parameters of the @ref aes_RESOLV_ADDR_IND message
struct aes_resolv_addr_ind
{
    /// Resolved address
    struct bd_addr addr;
    /// Used IRK
    struct gap_sec_key irk;
};

/// Parameters of the @ref aes_CMP_EVT message
struct aes_cmp_evt
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

extern const struct ke_state_handler aes_state_handler[AES_STATE_MAX];
extern const struct ke_state_handler aes_default_handler;
extern ke_state_t aes_state[AES_IDX_MAX];

#endif // (USE_AES)

#endif // (aes_TASK_H_)

/// @} aes_TASK
