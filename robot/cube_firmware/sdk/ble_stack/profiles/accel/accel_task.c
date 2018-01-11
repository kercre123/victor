/**
 ****************************************************************************************
 *
 * @file accel_task.c
 *
 * @brief Accelerometer profile task
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup ACCELTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_ACCEL)

#include "gattc_task.h"
#include "accel_task.h"
#include "accel.h"
#include "attm_util.h"
#include "attm_db.h"
#include "gap.h"
#include "prf_utils.h"
#include "battery.h"                      


extern uint8_t accel_adv_count __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint16_t accel_adv_interval __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern int8_t update_conn_params __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

extern uint8_t accel_threshold __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint8_t accel_adv_interval1 __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint8_t accel_adv_interval2 __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint8_t accel_adv_interval3 __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

extern uint8_t accel_con_interval __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

extern uint8_t accel_mode __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint8_t accel_latency __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
extern uint8_t accel_window __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Pong Service
static const att_svc_desc_t accel_svc = ACCEL_SERVICE_UUID;
/// Enable Characteristic
static const struct att_char_desc accel_enable_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP, 0, ACCEL_ENABLE_UUID);

/// Range Characteristic
static const struct att_char_desc accel_range_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR, 0, ACCEL_RANGE_UUID);

/// X axis Characteristic
static const struct att_char_desc accel_x_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF, 0, ACCEL_X_UUID);

/// Y axis Characteristic
static const struct att_char_desc accel_y_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF, 0, ACCEL_Y_UUID);

/// Z axis Characteristic
static const struct att_char_desc accel_z_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF, 0, ACCEL_Z_UUID);

/// Display line 1 Characteristic
static const struct att_char_desc accel_display1_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR, 0, ACCEL_DISPLAY1_UUID);

/// Display line 2 Characteristic
static const struct att_char_desc accel_display2_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR, 0, ACCEL_DISPLAY2_UUID);

/// Enable description
static const uint8_t accel_enable_desc[] = ACCEL_ENABLE_DESC;

/// Range description
static const uint8_t accel_range_desc[] = ACCEL_RANGE_DESC;

/// X axis description
static const uint8_t accel_x_desc[] = ACCEL_X_DESC;

/// Y axis description
static const uint8_t accel_y_desc[] = ACCEL_Y_DESC;

/// Z axis description
static const uint8_t accel_z_desc[] = ACCEL_Z_DESC;

/// Display line 1 description
static const uint8_t accel_display1_desc[] = ACCEL_DISPLAY1_DESC;

/// Display line 2 description
static const uint8_t accel_display2_desc[] = ACCEL_DISPLAY2_DESC;




/// Full ACCEL Database Description - Used to add attributes into the database
static const struct attm_desc accel_att_db[ACCEL_IDX_NB] =
{
    [ACCEL_IDX_PRIM_SVC]   =   /* Accelerometer service */
                               {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE),
                                       sizeof(accel_svc), sizeof(accel_svc),
                                       (uint8_t*) &accel_svc},
    [ACCEL_IDX_ENABLE_CHAR]  = /* Accelerometer enable characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_enable_char), sizeof(accel_enable_char),
                                       (uint8_t*) &accel_enable_char},
    [ACCEL_IDX_ENABLE_VAL] =   /* Accelerometer Enable Value */
                               {ACCEL_ENABLE_UUID,  (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                       sizeof(uint8_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ENABLE_DESC] =  /* Accelerometer Enable User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION,  PERM(RD, ENABLE),
                                       ACCEL_ENABLE_DESC_LEN, ACCEL_ENABLE_DESC_LEN,
                                       (uint8_t*) accel_enable_desc},
    [ACCEL_IDX_RANGE_CHAR] = /* Accelerometer range characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_range_char), sizeof(accel_range_char),
                                       (uint8_t*) &accel_range_char},
    [ACCEL_IDX_RANGE_VAL] =   /* Accelerometer range Value */
                               {ACCEL_ENABLE_UUID, (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                       sizeof(uint8_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_RANGE_DESC] =  /* Accelerometer range User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_RANGE_DESC_LEN, ACCEL_RANGE_DESC_LEN,
                                       (uint8_t*) accel_range_desc},
    [ACCEL_IDX_ACCEL_X_CHAR] = /* Accelerometer X-Axis characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_x_char), sizeof(accel_x_char),
                                       (uint8_t*) &accel_x_char},
    [ACCEL_IDX_ACCEL_X_VAL] =   /* Accelerometer X-Axis Value */
                               {ACCEL_X_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE),
                                       sizeof(uint8_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_X_EN] =   /* Accelerometer X-Axis enable Value */
                               {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                       sizeof(uint16_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_X_DESC] =  /* Accelerometer X-Axis User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_X_DESC_LEN, ACCEL_X_DESC_LEN,
                                       (uint8_t*) accel_x_desc},
    [ACCEL_IDX_ACCEL_Y_CHAR] = /* Accelerometer Y-Axis characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_y_char), sizeof(accel_y_char),
                                       (uint8_t*) &accel_y_char},
    [ACCEL_IDX_ACCEL_Y_VAL] =   /* Accelerometer Y-Axis Value */
                               {ACCEL_Y_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE),
                                       sizeof(uint8_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_Y_EN] =   /* Accelerometer Y-Axis enable Value */
                               {ATT_DESC_CLIENT_CHAR_CFG, (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                       sizeof(uint16_t),0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_Y_DESC] =  /* Accelerometer Y-Axis User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_Y_DESC_LEN, ACCEL_Y_DESC_LEN,
                                       (uint8_t*) accel_y_desc},
    [ACCEL_IDX_ACCEL_Z_CHAR] = /* Accelerometer Z-Axis characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_z_char), sizeof(accel_z_char),
                                       (uint8_t*) &accel_z_char},
    [ACCEL_IDX_ACCEL_Z_VAL] =   /* Accelerometer Z-Axis Value */
                               {ACCEL_Z_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE),
                                       sizeof(uint8_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_Z_EN] =   /* Accelerometer Z-Axis enable Value */
                               {ATT_DESC_CLIENT_CHAR_CFG, (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                       sizeof(uint16_t), 0, (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_Z_DESC] =  /* Accelerometer Z-Axis User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_Z_DESC_LEN, ACCEL_Z_DESC_LEN,
                                       (uint8_t*) accel_z_desc},
    [ACCEL_IDX_ACCEL_DISPLAY1_CHAR] = /* Accelerometer Display Line1 characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_display1_char), sizeof(accel_display1_char),
                                       (uint8_t*) &accel_display1_char},
    [ACCEL_IDX_ACCEL_DISPLAY1_VAL] =   /* Accelerometer Display Line1 Value */
                               {ACCEL_ENABLE_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                       ACCEL_LINE_LEN_MAX, 0,
                                       (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_DISPLAY1_DESC] =  /* Accelerometer Z-Axis User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_DISPLAY1_DESC_LEN,  ACCEL_DISPLAY1_DESC_LEN,
                                       (void*) accel_display1_desc},
    [ACCEL_IDX_ACCEL_DISPLAY2_CHAR] = /* Accelerometer Display Line2 characteristic */
                               {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE),
                                       sizeof(accel_display2_char), sizeof(accel_display2_char),
                                       (uint8_t*) &accel_display2_char},
    [ACCEL_IDX_ACCEL_DISPLAY2_VAL] =   /* Accelerometer Display Line2 Value */
                               {ACCEL_ENABLE_UUID,PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                       ACCEL_LINE_LEN_MAX, 0,
                                       (uint8_t*) NULL},
    [ACCEL_IDX_ACCEL_DISPLAY2_DESC] =  /* Accelerometer Display Line2 User description */
                               {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE),
                                       ACCEL_DISPLAY2_DESC_LEN, ACCEL_DISPLAY2_DESC_LEN,
                                       (uint8_t*) accel_display2_desc},
};


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ACCEL_CREATE_DB_REQ message.
 * The handler adds ACCEL Service into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int accel_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct accel_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag

    uint32_t cfg_flag = 0xFFFFFFFF;

    //Database Creation Status
    uint8_t status;


    // set start handle or automatically set it when creating database (start_hdl = 0)
    accel_env.shdl=param->start_hdl;

    //Add Service Into Database
    status = attm_svc_create_db(&accel_env.shdl, (uint8_t *)&cfg_flag,  ACCEL_IDX_NB, NULL,
                               dest_id, &accel_att_db[0]);

    //Disable GLS
    attmdb_svc_set_permission(accel_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_ACCEL, ACCEL_IDLE);
    }

    //Send response to application
    struct accel_create_db_cfm * cfm = KE_MSG_ALLOC(ACCEL_CREATE_DB_CFM, src_id,
                                                    TASK_ACCEL, accel_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ACCEL_ENABLE_REQ message.
 * The handler enables the accelerometer profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int accel_enable_req_handler(ke_msg_id_t const msgid,
                                    struct accel_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint16_t disable_val = 0x00;

    //Save Application ID
    accel_env.con_info.appid = src_id;
    accel_env.con_info.prf_id = dest_id;

    // Save the connection index associated to the profile
    accel_env.con_info.conidx = gapc_get_conidx(param->conhdl);


    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_ENABLE_VAL), sizeof(uint8_t), (uint8_t*) &(disable_val));
    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_RANGE_VAL), sizeof(uint8_t), (uint8_t*) &(disable_val));

    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_ACCEL_X_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_ACCEL_Y_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_ACCEL_Z_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));

{
	uint8_t tb;
	
	tb = battery_get_lvl(BATT_CR1225);

	 // Update the value in the attribute database
    attmdb_att_set_value(ACCEL_HANDLE(ACCEL_IDX_ENABLE_VAL), sizeof(uint8_t), (uint8_t*) &(tb));
}
	    
    //Enable Service
    attmdb_svc_set_permission(accel_env.shdl, PERM(SVC, ENABLE));

    // Go to active state
    ke_state_set(TASK_ACCEL, ACCEL_ACTIVE);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ACCEL_DISABLE_REQ message.
 * The handler disables the accelerometer profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int accel_disable_req_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Go to idle state
    ke_state_set(TASK_ACCEL, ACCEL_IDLE);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ACCEL_VALUE_REQ message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int accel_value_req_handler(ke_msg_id_t const msgid,
                                   struct accel_value_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint16_t len;
    uint16_t* axis_en;
    uint8_t* axis_val;

    // Check if acceleration changed on each axis and notify it if required
    for (int i = 0; i < ACCEL_MAX; i++)
    {
        attmdb_att_get_value(ACCEL_DIR_VAL_HANDLE(i), &(len), (uint8_t**)&(axis_val));
        attmdb_att_get_value(ACCEL_DIR_EN_HANDLE(i), &(len), (uint8_t**)&(axis_en));

        if ((*axis_en) && (param->accel[i] != *axis_val))
        {
            // Update the value in the attribute database
            attmdb_att_set_value(ACCEL_DIR_VAL_HANDLE(i),   sizeof(uint8_t), (uint8_t*) &(param->accel[i]));

            // Send notification
            prf_server_send_event((prf_env_struct *)&accel_env, false, ACCEL_DIR_VAL_HANDLE(i));

        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Update the attribute value
    attmdb_att_update_value(param->handle, param->length, param->offset,
            (uint8_t*)&(param->value[0]));

    switch (ACCEL_IDX(param->handle))
    {
        case ACCEL_IDX_ENABLE_VAL:
        {
            uint16_t len;
            uint8_t* accel_en;
            uint8_t* axis_en;
            uint8_t* range;

            if(*(uint8_t*)&(param->value[0]) > 1)
                accel_threshold = *(uint8_t*)&(param->value[0]) - 2;
            set_accel_thr();
                        
            attmdb_att_get_value(ACCEL_HANDLE(ACCEL_IDX_ENABLE_VAL), &(len), &(accel_en));

            // Indicate to the application the state of the profile
            if (accel_en)
            {
                // Allocate the start indication message
                struct accel_start_ind *ind = KE_MSG_ALLOC(ACCEL_START_IND,
                                                           accel_env.con_info.appid, dest_id,
                                                           accel_start_ind);

                // Fill in the parameter structure
                for (int i = 0; i < ACCEL_MAX; i++)
                {
                    attmdb_att_get_value(ACCEL_DIR_VAL_HANDLE(i), &(len), &(axis_en));

                    ind->accel_en[i] = *axis_en;
                }


                attmdb_att_get_value(ACCEL_HANDLE(ACCEL_IDX_RANGE_VAL), &(len), &(range));

                ind->range = *range;

                // Send the message
                ke_msg_send(ind);
            }
            else
            {
                // Send the stop indication
                ke_msg_send_basic(ACCEL_STOP_IND, accel_env.con_info.appid, TASK_ACCEL);
            }
        }
        break;

        case ACCEL_IDX_ACCEL_DISPLAY1_VAL:
        case ACCEL_IDX_ACCEL_DISPLAY2_VAL:            
            if(*(uint8_t*)&(param->value[0]) == 0xFF)
            {
                if(param->length > 1 && *(uint8_t*)&(param->value[1]) > 1)
					accel_con_interval = *(uint8_t*)&(param->value[1]);
                if(param->length > 2 && *(uint8_t*)&(param->value[2]) > 1)
					accel_mode = *(uint8_t*)&(param->value[2])-2;
				if(param->length > 3 && *(uint8_t*)&(param->value[3]) > 1)
					accel_latency = *(uint8_t*)&(param->value[3])-2;
				if(param->length > 4 && *(uint8_t*)&(param->value[4]) > 1)
					accel_window = *(uint8_t*)&(param->value[4]);
            }
        {
            uint8_t line;
            uint16_t len;
            uint8_t* display;

            struct accel_write_line_ind *ind = KE_MSG_ALLOC(ACCEL_WRITE_LINE_IND,
                                                            accel_env.con_info.appid, TASK_ACCEL,
                                                            accel_write_line_ind);
            line = (param->handle == ACCEL_HANDLE(ACCEL_IDX_ACCEL_DISPLAY1_VAL))?0:1;


            attmdb_att_get_value(param->handle, &(len), &(display));

            // Fill in the parameter structure
            memcpy(ind->text, display, len);
            ind->line = line;

            // Send the message
            ke_msg_send(ind);
        }
            atts_write_rsp_send(accel_env.con_info.conidx, param->handle, PRF_ERR_OK);
            break;        
        case ACCEL_IDX_ACCEL_X_VAL:
        case ACCEL_IDX_ACCEL_X_EN:
					if(*(uint8_t*)&(param->value[0]) > 1)
					accel_adv_interval1 = *(uint8_t*)&(param->value[0]);
						atts_write_rsp_send(accel_env.con_info.conidx, param->handle, PRF_ERR_OK);
            break;
        case ACCEL_IDX_ACCEL_Y_VAL:
        case ACCEL_IDX_ACCEL_Y_EN:
					if(*(uint8_t*)&(param->value[0]) > 1)
					accel_adv_interval2 = *(uint8_t*)&(param->value[0]);
					if(*(uint8_t*)&(param->value[0]) == 255)
					accel_adv_interval2 = 0;
						atts_write_rsp_send(accel_env.con_info.conidx, param->handle, PRF_ERR_OK);
            break;
        case ACCEL_IDX_ACCEL_Z_VAL:
        case ACCEL_IDX_ACCEL_Z_EN:
					if(*(uint8_t*)&(param->value[0]) > 1)
					accel_adv_interval3 = *(uint8_t*)&(param->value[0]);
					if(*(uint8_t*)&(param->value[0]) == 255)
					accel_adv_interval3 = 0;
						atts_write_rsp_send(accel_env.con_info.conidx, param->handle, PRF_ERR_OK);
            break;
        
        default:
            atts_write_rsp_send(accel_env.con_info.conidx, param->handle, PRF_ERR_OK);
            break;
            
    }

    return (KE_MSG_CONSUMED);
}


/*
 * TASK DESCRIPTOR DEFINITIONS
 ****************************************************************************************
 */
/// Disabled State handler definition.
const struct ke_msg_handler accel_disabled[] =
{
    {ACCEL_CREATE_DB_REQ,  (ke_msg_func_t) accel_create_db_req_handler}
};


/// IDLE State handlers definition
const struct ke_msg_handler accel_idle[] =
{
    {ACCEL_ENABLE_REQ,    (ke_msg_func_t)accel_enable_req_handler}
};

/// ACTIVE State handlers definition
const struct ke_msg_handler accel_active[] =
{
    {ACCEL_DISABLE_REQ,   (ke_msg_func_t)accel_disable_req_handler},
    {ACCEL_VALUE_REQ,     (ke_msg_func_t)accel_value_req_handler},
    {GATTC_WRITE_CMD_IND, (ke_msg_func_t)gattc_write_cmd_ind_handler}
};

/// Specifies the message handler structure for every input state
const struct ke_state_handler accel_state_handler[ACCEL_STATE_MAX] =
{
    /// DISABLE State message handlers.
    [ACCEL_DISABLED]  = KE_STATE_HANDLER(accel_disabled),
    /// IDLE State message handlers.
    [ACCEL_IDLE]      = KE_STATE_HANDLER(accel_idle),
    /// ACTIVE State message handlers.
    [ACCEL_ACTIVE]    = KE_STATE_HANDLER(accel_active),

};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler accel_default_handler = KE_STATE_HANDLER_NONE;

/// Defines the placeholder for the states of all the task instances.
ke_state_t accel_state[ACCEL_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* BLE_ACCEL */
/// @} ACCELTASK
