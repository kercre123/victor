/**
 ****************************************************************************************
 *
 * @file aes.c
 *
 * @brief aes implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup aes
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "aes.h"

#if (USE_AES)

#include "string.h"
#include "ke_task.h"
#include "aes_task.h"
#include "llm_task.h"
#include "ke_mem.h"
#include "sw_aes.h"
#include "aes_api.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct aes_env_tag aes_env __attribute__((section("exchange_mem_case1"))); //@RETENTION MEMORY 

/// aes task descriptor
static const struct ke_task_desc TASK_DESC_AES = {aes_state_handler,
                                                   &aes_default_handler,
                                                   aes_state, AES_STATE_MAX, AES_IDX_MAX};

unsigned char aes_out[KEY_LEN] __attribute__((section("exchange_mem_case1")));
void (*aes_cb)(uint8_t status) __attribute__((section("exchange_mem_case1")));

/* using a dummy IV vector filled with zeroes for decryption sicne the chip does not use IV at encryption */
unsigned char IV[16]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


/*
 * PUBLIC FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief AES init. Can also set the callback to be called at the end of each operation
 * @param[in] reset         FALSE will create the task, TRUE will just reset the environment.
 * @param[in] aes_done_cb   The callback to be called at the end of each operation
 ****************************************************************************************
 */
void aes_init(bool reset, void (*aes_done_cb)(uint8_t status))
{
    aes_cb = aes_done_cb;
    
    if(!reset)
    {
        // Reset the aes environment
        memset(&aes_env, 0, sizeof(aes_env));

        // Create aes task
        ke_task_create(TASK_AES, &TASK_DESC_AES);
    }
    else
    {
        if(aes_env.operation != NULL)
        {
            ke_free(ke_param2msg(aes_env.operation));
        }

        // Reset the aes environment
        memset(&aes_env, 0, sizeof(aes_env));
    }

    //default initial state is IDLE
    ke_state_set(TASK_AES, AES_IDLE);
}

/**
 ****************************************************************************************
 * @brief AES partial operation. Encrypt/decrypt a 16 byte block.
 * uses the AES environment for different block parameters
 * @param[in] enc_dec   decrypt, 1 encrypt.
 * @return 0 if successfull, -1 if userKey or key are NULL, -2 if bits is not 128, -3 if enc_dec not 0/1.
 ****************************************************************************************
 */
int aes_part(int enc_dec)
{
    int status=0;
    volatile uint32_t j;
    struct llm_le_enc_cmp_evt *evt;

sync_loop:
    
    if(enc_dec == 1)  //encrypt
    {
        status = aes_enc_dec(&aes_env.in[aes_env.in_cur],&aes_env.out[aes_env.out_cur], &aes_env.aes_key, enc_dec, aes_env.ble_flags);
    }
    else if(enc_dec == 0)  //decrypt
    { 
        status = aes_enc_dec(&aes_env.in[aes_env.in_cur], &aes_env.out[aes_env.out_cur], &aes_env.aes_key, enc_dec, aes_env.ble_flags);
    }
    else
        return -3;
    
    if(status == 0)
        aes_env.in_cur += KEY_LEN;
    
    if((aes_env.ble_flags & BLE_SYNC_MASK))     //use synchronous mode
    {
        if(status == 0 && aes_env.in_cur < aes_env.in_len)
        {
            aes_env.out_cur += KEY_LEN;
            goto sync_loop;
        }
    }
    else   //use asynchronous, message based mode of operation
    {
        // Allocate the message for the response
        evt = KE_MSG_ALLOC(LLM_LE_ENC_CMP_EVT, TASK_AES, TASK_LLM, llm_le_enc_cmp_evt);
        // Set the status
        evt->status = status;

        // Send the event
        ke_msg_send(evt);
    }
    
    return status;    
}

/**
 ****************************************************************************************
 * @brief AES encrypt/decrypt operation.
 * @param[in] key       The key data.
 * @param[in] key_len   The key data length in bytes. Should be 16.
 * @param[in] in        The input data block
 * @param[in] in_len    The input data block length
 * @param[in] out       The output data block
 * @param[in] out_len   The output data block length
 * @param[in] enc_dec   0 decrypt, 1 encrypt.
 * @param[in] aes_done_cb   The callback to be called at the end of each operation
 * @param[in] ble_flags used to specify whether the encryption/decryption 
 * will be performed syncronously or asynchronously (message based)
 * also if ble_safe is specified in ble_flags rwip_schedule() will be called
 * to avoid loosing any ble events
 * @return 0 if successfull, -1 if userKey or key are NULL, -2 if AES task is busy, -3 if enc_dec not 0/1, -4 if key_len not 16.
 ****************************************************************************************
 */
int aes_operation(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *out, int out_len, int enc_dec, void (*aes_done_cb)(uint8_t status), unsigned char ble_flags)
{
    int status;
    
    if(aes_env.operation != 0)  //busy
        return -2;
        
    if(key_len != KEY_LEN)
        return -4;

    aes_env.key = key;
    aes_env.key_len = key_len;
    aes_env.in = in;
    aes_env.in_len = in_len;
    aes_env.out = out;
    aes_env.out_len = out_len;
    
    aes_env.in_cur = 0;
    aes_env.out_cur = 0;
    aes_env.enc_dec = enc_dec;
    aes_env.aes_done_cb = aes_done_cb;
    aes_env.ble_flags = ble_flags;

    if((ble_flags & BLE_SYNC_MASK) == 0)   //use asynchronous, message based mode of operation
    {
        struct gapm_use_enc_block_cmd *cmd = KE_MSG_ALLOC(AES_USE_ENC_BLOCK_CMD,
                                                          TASK_AES, TASK_AES,
                                                          gapm_use_enc_block_cmd);

        cmd->operation = enc_dec?AES_OP_USE_ENC_BLOCK:AES_OP_USE_ENC_BLOCK+1;
        
        // Go in a busy state
        ke_state_set(TASK_AES, AES_BUSY);
        // Keep the command message until the end of the request
        aes_env.operation = (void *)cmd;
        aes_env.requester = TASK_AES;
    }
    
    status = aes_set_key(aes_env.key, 128, &aes_env.aes_key, aes_env.enc_dec);
    if(status != 0)
        return status;
    
    return aes_part(enc_dec);
}

/*
 * PRIVATE FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void aes_send_cmp_evt(ke_task_id_t cmd_src_id, uint8_t operation, uint8_t status)
{
    //status is OK so continue
    if(status == SMP_ERROR_NO_ERROR && aes_env.in_cur < aes_env.in_len) //more bytes
    {
        aes_env.out_cur += KEY_LEN;

        if(aes_part(aes_env.enc_dec) == 0)
            return;
        else
            status = -status;
    }

    // Come back to IDLE state
    ke_state_set(TASK_AES, AES_IDLE);
    
    // Release the command message
    if (aes_env.operation != NULL)
    {
        ke_msg_free(ke_param2msg(aes_env.operation));
        aes_env.operation = NULL;
    }
    
    if(aes_env.aes_done_cb == NULL) //no callback so do nothing
        return;
    
    aes_env.aes_done_cb(status);    
}


void aes_send_encrypt_req(uint8_t *operand_1, uint8_t *operand_2)
{
    int enc_dec = ((struct smp_cmd *)(aes_env.operation))->operation == AES_OP_USE_ENC_BLOCK?1:0;
    
    aes_operation(operand_1, KEY_LEN, operand_2, KEY_LEN, aes_out, KEY_LEN, enc_dec, aes_cb, 0);
}

#endif // (USE_AES)
/// @} aes
