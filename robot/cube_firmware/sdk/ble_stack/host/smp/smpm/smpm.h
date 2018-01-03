/**
 ****************************************************************************************
 *
 * @file smpm.h
 *
 * @brief Header file - SMPM.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef SMPM_H_
#define SMPM_H_

/**
 ****************************************************************************************
 * @addtogroup SMPM Security Manager Protocol Manager
 * @ingroup SMP
 * @brief Security Manager Protocol Manager.
 *
 * This Module allows the 1-instanced modules to communicate with multi-instanced SMPC module.
 * It is only an intermediary between the actual SMPC handling SM behavior, and
 * LLM, GAP, or GATT which only indicate the index of the connection for which
 * SMPC actions are necessary.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "smp_common.h"       // Firmware Configuration Flags


#include "ke_msg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Length of resolvable random address prand part
#define SMPM_RAND_ADDR_PRAND_LEN            (3)
// Length of resolvable random address hash part
#define SMPM_RAND_ADDR_HASH_LEN             (3)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/**
 * Random Address Types
 */
enum smpm_rand_addr_type
{
    /// Private Non Resolvable  - 00 (MSB->LSB)
    SMPM_ADDR_TYPE_PRIV_NON_RESOLV          = 0x00,
    /// Private Resolvable      - 01
    SMPM_ADDR_TYPE_PRIV_RESOLV              = 0x40,
    /// Static                  - 11
    SMPM_ADDR_TYPE_STATIC                   = 0xC0,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * SMPM environment structure
 */
struct smpm_env_tag
{
    /// Request operation Kernel message
    void *operation;
    /// Operation requester task id
    ke_task_id_t requester;
 };

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

extern struct smpm_env_tag smpm_env;

extern struct smpm_old_env_tag smpm_old_env;

/*
 * PUBLIC FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize SMP task.
 *
 * @param[in] reset   true if it's requested by a reset; false if it's boot initialization
 ****************************************************************************************
 */
void smpm_init(bool reset);

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @brief Allocate SMP resources for connection creates.
 *
 * @param[in] conidx           connection index
 ****************************************************************************************
 */
void smpm_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Cleanup SMP resources for specified disconnected link.
 *
 * @param[in] conidx           connection index
 ****************************************************************************************
 */
void smpm_cleanup(uint8_t conidx);
#endif //(BLE_CENTRAL || BLE_PERIPHERAL)

/*
 * PRIVATE FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send a SMPM_CMP_EVT message to the task which sent the request.
 *
 * @param[in] cmd_src_id            Command source ID
 * @param[in] operation             Command operation code
 * @param[in] status                Status of the request
 ****************************************************************************************
 */
void smpm_send_cmp_evt(ke_task_id_t cmd_src_id, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send an encryption request to the LLM.
 ****************************************************************************************
 */
void smpm_send_encrypt_req(uint8_t *operand_1, uint8_t *operand_2);

/**
 ****************************************************************************************
 * @brief Send a generate Random Number request to the LLM.
 ****************************************************************************************
 */
void smpm_send_gen_rand_nb_req(void);

/**
 ****************************************************************************************
 * @brief Check the address type provided by the application.
 *
 * @param[in]  addr_type            Provided address type to check
 * @param[out] true if the address type is valid, false else
 ****************************************************************************************
 */
bool smpm_check_addr_type(uint8_t addr_type);

#endif // (SMPM_H_)

/// @} SMPM
