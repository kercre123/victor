/**
 ****************************************************************************************
 *
 * @file aes_task.c
 *
 * @brief aes Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup aes_TASK
 * @ingroup aes
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "aes_task.h"

#if (USE_AES)

#include "ke_msg.h"
#include "smpc_task.h"
#include "llm_task.h"
#include "gapm_util.h"
#include "gapc.h"
#include <string.h>

/*
 * FUNCTION DEFINITONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Default message handler used to handle all messages from:
 *  - LLC task
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAPM).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int aes_default_msg_handler(ke_msg_id_t const msgid,
                                    void *event,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    #if (BLE_CENTRAL || BLE_PERIPHERAL)
    // If a message comes from LLC task, convey it to SMPC task
    if(MSG_T(msgid) == TASK_LLC)
    {
        // Retrieve connection handler - Message from LLC have common command data structure
        uint8_t conidx = gapc_get_conidx(((struct llc_event_common_cmd_status *)(event))->conhdl);

        // Check if the connection exists
        if (conidx != GAP_INVALID_CONIDX)
        {
            // convey message to GAPC
            ke_msg_forward(event, KE_BUILD_ID(TASK_SMPC, conidx), src_id);

            msg_status = KE_MSG_NO_FREE;
        }
    }
    #endif //(BLE_CENTRAL || BLE_PERIPHERAL)

    return (msg_status);
}



/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] req       Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int aes_use_enc_block_cmd_handler(ke_msg_id_t const msgid,
                                          struct gapm_use_enc_block_cmd const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message Status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Check if the aes task is not busy
    if (ke_state_get(dest_id) == AES_IDLE)
    {
        // Check the operation code and the address type
        if (param->operation == AES_OP_USE_ENC_BLOCK || param->operation == AES_OP_USE_ENC_BLOCK+1)
        {
            // Keep the command message until the end of the request
            aes_env.operation = (void *)param;
            aes_env.requester = src_id;
            // Inform the kernel that this message cannot be unallocated
            msg_status = KE_MSG_NO_FREE;

            // Ask for encryption/decryption
            aes_send_encrypt_req((uint8_t *)&param->operand_1[0], (uint8_t *)&param->operand_2[0]);

            // Go in a busy state
            ke_state_set(TASK_AES, AES_BUSY);
        }
        else
        {
            aes_send_cmp_evt(src_id,
                              AES_OP_USE_ENC_BLOCK,
                              SMP_GEN_PAIR_FAIL_REASON(SMP_PAIR_FAIL_REASON_MASK, SMP_ERROR_INVALID_PARAM));
        }
    }
    else
    {
        // Push the message in the message queue so that it can be handled later
        msg_status = KE_MSG_SAVED;
    }

    return (int)msg_status;
}


/**
 ****************************************************************************************
 * @brief LLM LE Encrypt command complete event handler.
 * The received encrypted data is to be sent to the saved SMPC source task ID (which is the
 * origin of the request).
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 * @return Received kernel message status (consumed or not).
 ****************************************************************************************
 */
static int llm_le_enc_cmp_evt_handler(ke_msg_id_t const msgid,
                                      struct llm_le_enc_cmp_evt const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Check if a command is currently handled, drop the message if not the case.
    if (ke_state_get(dest_id) == AES_BUSY)
    {
        ASSERT_ERR(aes_env.operation != NULL);
        switch (((struct smp_cmd *)(aes_env.operation))->operation)
        {
            case (AES_OP_USE_ENC_BLOCK):
            {
                uint8_t status = SMP_ERROR_NO_ERROR;

                // Check the returned status
                if (param->status == SMP_ERROR_NO_ERROR)
                {
                }
                else
                {
                    status = SMP_ERROR_LL_ERROR;
                }

                // Send the command complete event message
                aes_send_cmp_evt(aes_env.requester, AES_OP_USE_ENC_BLOCK, status);
            } break;
            case (AES_OP_USE_ENC_BLOCK+1):
            {
                uint8_t status = SMP_ERROR_NO_ERROR;
                // Send the command complete event message
                aes_send_cmp_evt(aes_env.requester, AES_OP_USE_ENC_BLOCK+1, status);
            }    break;
            default:
            {
                // Drop the message
                ASSERT_ERR(0);
            } break;
        }
    }

    return KE_MSG_CONSUMED;
}

/*
 * TASK DESCRIPTOR DEFINITIONS
 ****************************************************************************************
 */

/// The default message handlers
const struct ke_msg_handler aes_default_state[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)aes_default_msg_handler},

    {AES_USE_ENC_BLOCK_CMD,        (ke_msg_func_t)aes_use_enc_block_cmd_handler},

    {LLM_LE_ENC_CMP_EVT,            (ke_msg_func_t)llm_le_enc_cmp_evt_handler},
};

/// State handlers table
const struct ke_state_handler aes_state_handler[AES_STATE_MAX] =
{
    [AES_IDLE]     = KE_STATE_HANDLER_NONE,
    [AES_BUSY]     = KE_STATE_HANDLER_NONE,
};

/// Default state handlers table
const struct ke_state_handler aes_default_handler = KE_STATE_HANDLER(aes_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t aes_state[AES_IDX_MAX]  __attribute__((section("exchange_mem_case1"))); //@RETENTION MEMORY;

#endif // #if (USE_AES)

/// @} aes_TASK
