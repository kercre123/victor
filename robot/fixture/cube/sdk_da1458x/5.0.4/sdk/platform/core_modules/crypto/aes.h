/**
 ****************************************************************************************
 *
 * @file aes.h
 *
 * @brief Header file - aes.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef AES_H_
#define AES_H_

/**
 ****************************************************************************************
 * @addtogroup aes Security Manager Protocol Manager
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

#if (USE_AES)
#include "co_bt.h"
#include "ke_msg.h"
#include "sw_aes.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Length of resolvable random address prand part
#define AES_RAND_ADDR_PRAND_LEN            (3)
// Length of resolvable random address hash part
#define AES_RAND_ADDR_HASH_LEN             (3)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/**
 * Random Address Types
 */
enum aes_rand_addr_type
{
    /// Private Non Resolvable  - 00 (MSB->LSB)
    AES_ADDR_TYPE_PRIV_NON_RESOLV          = 0x00,
    /// Private Resolvable      - 01
    AES_ADDR_TYPE_PRIV_RESOLV              = 0x40,
    /// Static                  - 11
    AES_ADDR_TYPE_STATIC                   = 0xC0,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */
typedef AES_CTX AES_KEY;
/**
 * aes environment structure
 */
struct aes_env_tag
{
    /// Request operation Kernel message
    void *operation;
    /// Operation requester task id
    ke_task_id_t requester;
    
    uint8_t enc_dec;
    int key_len;
    unsigned char *key;
    AES_KEY aes_key;
    unsigned char *in;
    int in_len;
    int in_cur;
    unsigned char *out;
    int out_len;
    int out_cur;
    
    void (*aes_done_cb)(uint8_t status);
    
    unsigned char ble_flags;
 };

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

extern struct aes_env_tag aes_env;
extern unsigned char aes_out[KEY_LEN];

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
void aes_init(bool reset, void (*aes_done_cb)(uint8_t status));

int aes_operation(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *out, int out_len, int operation, void (*aes_done_cb)(uint8_t status), unsigned char ble_flags);


/*
 * PRIVATE FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send a aes_CMP_EVT message to the task which sent the request.
 *
 * @param[in] cmd_src_id            Command source ID
 * @param[in] operation             Command operation code
 * @param[in] status                Status of the request
 ****************************************************************************************
 */
void aes_send_cmp_evt(ke_task_id_t cmd_src_id, uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send an encryption request to the LLM.
 ****************************************************************************************
 */
void aes_send_encrypt_req(uint8_t *operand_1, uint8_t *operand_2);

/**
 ****************************************************************************************
 * @brief Send a generate Random Number request to the LLM.
 ****************************************************************************************
 */
void aes_send_gen_rand_nb_req(void);

/**
 ****************************************************************************************
 * @brief Check the address type provided by the application.
 *
 * @param[in]  addr_type            Provided address type to check
 * @param[out] true if the address type is valid, false else
 ****************************************************************************************
 */
bool aes_check_addr_type(uint8_t addr_type);

#endif // (USE_AES)

#endif // (aes_H_)

/// @} aes
