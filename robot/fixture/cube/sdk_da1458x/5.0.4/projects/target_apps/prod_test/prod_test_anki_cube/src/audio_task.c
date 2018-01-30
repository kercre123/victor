/**
 ****************************************************************************************
 *
 * @file audio_task.c
 *
 * @brief Audio Task implementation.
 *
 * @brief 128 UUID service. Audio code
 *
 * Copyright (C) 2015 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#if HAS_AUDIO


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "audio_task.h"
#include "co_bt.h"
#include "customer_prod.h"
#include "uart.h"


/*
 * DEFINES
 ****************************************************************************************
 */
#define _RETAINED __attribute__((section("retention_mem_area0"), zero_init))


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

uint16_t packets_required _RETAINED;

bool continuous_test _RETAINED;

uint32_t packet_nr _RETAINED;

size_t  read_ptr _RETAINED,
        write_ptr _RETAINED;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Start request to audio.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int audio_start_req_handler(ke_msg_id_t const msgid,
                                   struct audio_start_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    ke_state_set(TASK_APP, AUDIO_ON);
    packets_required = param->packets_required;
    continuous_test = !packets_required;
    packet_nr = 0;
    read_ptr = 0;
    write_ptr = 0;

    app_audio439_start();
    
    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Stop request to audio.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message. (unused)
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int audio_stop_req_handler(ke_msg_id_t const msgid,
                                  void const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    app_audio439_stop();
    ke_state_set(TASK_APP, AUDIO_OFF);

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Samples indication to audio.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int audio_samples_ind_handler(ke_msg_id_t const msgid,
                                     struct audio_samples_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    static struct msg_audio_test_cfm msg;
    static struct audio_packet *packet = &msg.results.audio_packet;
    const size_t AUDIO_TOTAL_SIZE = sizeof(packet->encoded_bytes);
    static uint8_t buffer[2 * AUDIO_TOTAL_SIZE];
    
    memcpy(buffer + write_ptr, param->bytes, AUDIO_ENC_SIZE);
    write_ptr += AUDIO_ENC_SIZE;
    
    if (write_ptr - read_ptr == AUDIO_TOTAL_SIZE) {
        msg.packet_type	        = HCI_EVT_MSG_TYPE;
        msg.event_code 	        = HCI_CMD_STATUS_EVT_CODE; 
        msg.length     	        = sizeof(msg) - HCI_CCEVT_HDR_PARLEN;
        msg.num_hci_cmd_packets	= HCI_CCEVT_NUM_CMD_PACKETS;
        msg.param_opcode        = HCI_AUDIO_TEST_CMD_OPCODE;

        packet->number = packet_nr++;
        switch (param->encoding) {
        case AUDIO_IMA:
            packet->type = AUDIO_IMA_PACKET;
            break;
        case AUDIO_ALAW:
            packet->type = AUDIO_ALAW_PACKET;
            break;
        case AUDIO_RAW:
            packet->type = AUDIO_RAW_PACKET;
            break;
        default:
            packet->type = AUDIO_WAS_INACTIVE;
            break;
        }
        memcpy(packet->encoded_bytes, buffer + read_ptr, AUDIO_TOTAL_SIZE);

        uart_write((uint8_t*) &msg, sizeof(msg), NULL);
        uart_finish_transfers();
        
        if (!continuous_test && packet_nr == packets_required) {
            return audio_stop_req_handler(AUDIO_STOP_REQ, NULL, dest_id, src_id);
        }
        if (write_ptr == sizeof(buffer)) {
            write_ptr = 0;
        }
        read_ptr += AUDIO_TOTAL_SIZE;
        if (read_ptr == sizeof(buffer)) {
            read_ptr = 0;
        }
    }
    return KE_MSG_CONSUMED;
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Off state handler definition.
const struct ke_msg_handler audio_off[] =
{
    {AUDIO_START_REQ, (ke_msg_func_t) audio_start_req_handler},
};

/// On state handler definition.
const struct ke_msg_handler audio_on[] =
{
    {AUDIO_SAMPLES_IND, (ke_msg_func_t) audio_samples_ind_handler},
    {AUDIO_STOP_REQ, (ke_msg_func_t) audio_stop_req_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler audio_state_handler[AUDIO_STATE_MAX] =
{
    [AUDIO_ON] = KE_STATE_HANDLER(audio_on),
    [AUDIO_OFF] = KE_STATE_HANDLER(audio_off),
};

/// Defines the place holder for the states of all the task instances.
ke_state_t audio_state[AUDIO_IDX_MAX] __attribute__((section("retention_mem_area0"), zero_init));

#endif //HAS_AUDIO
