 /**
 ****************************************************************************************
 *
 * @file streamdatad.c
 *
 * @brief StreamData Device profile
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @addtogroup STREAMDATAD
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"
#if (BLE_STREAMDATA_DEVICE)

#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "atts_util.h"
#include "attm_cfg.h"
#include "prf_utils.h"
#include "l2cm.h"

#include "streamdatad.h"
#include "streamdatad_task.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct streamdatad_env_tag streamdatad_env; // __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// PROXR task descriptor
static const struct ke_task_desc TASK_DESC_STREAMD = {streamdatad_state_handler, &streamdatad_default_handler, streamdatad_state, STREAMDATAD_STATE_MAX, STREAMDATAD_IDX_MAX};
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void streamdatad_init(void)
{
    // Reset the streamdataderometer environment
    memset(&streamdatad_env, 0, sizeof(streamdatad_env));
    
    // Create PROXR task
    ke_task_create(TASK_STREAMDATAD, &TASK_DESC_STREAMD);

    // Go to IDLE state
    ke_state_set(TASK_STREAMDATAD, STREAMDATAD_DISABLED);
}

void streamdatad_streamonoff(void)
{
	uint16_t len = 0;
	uint8_t* streamdatad_en = NULL;

	attmdb_att_get_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_ENABLE_VAL), &(len), &(streamdatad_en));

	
	// Indicate to the application the state of the profile
	int on = ((len == 2) && (streamdatad_en && (*streamdatad_en))) ? 1 : 0;
	
	if (on)
	{
		// Allocate the start indication message
		struct streamdatad_start_ind *ind = KE_MSG_ALLOC(STREAMDATAD_START_IND,
												   streamdatad_env.appid, TASK_STREAMDATAD,
												   streamdatad_start_ind);
		streamdatad_env.stream_enabled = (*streamdatad_en);
		ind->status = PRF_ERR_OK;
		// Send the message
		ke_msg_send(ind);
	}
	else
	{
		streamdatad_env.next_attribute_idx = 0;
		streamdatad_env.stream_enabled = 0;

		// Send the stop indication
		ke_msg_send_basic(STREAMDATAD_STOP_IND, streamdatad_env.appid, TASK_STREAMDATAD);
	}
	
	uint16_t enable_val = streamdatad_en?(*streamdatad_en):0;
	// Enable or disable all data notifications
	streamdatad_env.nr_enabled_attributes = 0;
	int lastattr = enable_val?((STREAMDATAD_MAX>MAX_TRANSMIT_BUFFER_PACKETS)?MAX_TRANSMIT_BUFFER_PACKETS:STREAMDATAD_MAX):STREAMDATAD_MAX;
	
	attmdb_att_set_value(STREAMDATAD_IDX_ENABLE_EN, sizeof(uint16_t),(uint8_t*) &(enable_val));
	for (int i = 0; i < lastattr; i++)
	{
		attmdb_att_set_value(STREAMDATAD_DIR_EN_HANDLE(i), sizeof(uint16_t),(uint8_t*) &(enable_val));
		streamdatad_env.nr_enabled_attributes++;
	}
	if (!enable_val) streamdatad_env.nr_enabled_attributes = 0;
}

int streamdatad_send_data_packet(uint8_t *data)
{
    uint16_t* packet_buffer_enabled;
	int retval = 0;
    uint16_t len = 0;
	packet_buffer_enabled = NULL;
	attmdb_att_get_value(STREAMDATAD_DIR_EN_HANDLE(streamdatad_env.next_attribute_idx), &(len), (uint8_t**)&(packet_buffer_enabled));

	if ((packet_buffer_enabled && (*packet_buffer_enabled)))
	{        
        // Update the value in the attribute database
        attmdb_att_set_value(STREAMDATAD_DIR_VAL_HANDLE(streamdatad_env.next_attribute_idx), sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE, data);

        // Send notification
        prf_server_send_event((prf_env_struct *)&(streamdatad_env.con_info), false, STREAMDATAD_DIR_VAL_HANDLE(streamdatad_env.next_attribute_idx));
        
		retval = 1;
	}

	streamdatad_env.next_attribute_idx++; if (streamdatad_env.next_attribute_idx >= STREAMDATAD_MAX) streamdatad_env.next_attribute_idx = 0;
	return retval;
}



#endif /* BLE_STREAMDATA_DEVICE */

/// @} STREAMDATAD
