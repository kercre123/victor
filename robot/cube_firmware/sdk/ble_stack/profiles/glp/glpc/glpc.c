/**
 ****************************************************************************************
 *
 * @file glpc.c
 *
 * @brief Glucose Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup GLPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_GL_COLLECTOR)
#include "glpc.h"
#include "glpc_task.h"
#include "gap.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct glpc_env_tag **glpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// GLPC task descriptor
static const struct ke_task_desc TASK_DESC_GLPC = {glpc_state_handler, &glpc_default_handler, glpc_state, GLPC_STATE_MAX, GLPC_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void glpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(glpc_envs, GLPC);
}


void glpc_enable_cfm_send(struct glpc_env_tag *glpc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on GLPS
    struct glpc_enable_cfm * rsp = KE_MSG_ALLOC(GLPC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                glpc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->gls = glpc_env->gls;

        // Register GLPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&glpc_env->con_info, &glpc_env->gls.svc);

        // Go to connected state
        ke_state_set(con_info->prf_id, GLPC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(glpc_envs, con_info->prf_id, GLPC);
    }

    ke_msg_send(rsp);
}

uint8_t glpc_unpack_meas_value(uint8_t *packed_meas, struct glp_meas* meas_val,
                               uint16_t *seq_num)
{
    uint8_t cursor = 0;

    // Flags
    meas_val->flags = packed_meas[cursor];
    cursor += 1;

    // Sequence Number
    *seq_num = co_read16p(packed_meas + cursor);
    cursor += 2;

    // Base Time
    cursor += prf_unpack_date_time(packed_meas + cursor, &(meas_val->base_time));

    //Time Offset
    if((meas_val->flags & GLP_MEAS_TIME_OFF_PRES) != 0)
    {
        meas_val->time_offset = co_read16p(packed_meas + cursor);
        cursor += 2;
    }

    // Glucose Concentration, type and location
    if((meas_val->flags & GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES) != 0)
    {
        meas_val->concentration = co_read16p(packed_meas + cursor);
        cursor += 2;

        /* type and location are 2 nibble values */
        meas_val->location = packed_meas[cursor] >> 4;
        meas_val->type = packed_meas[cursor] & 0xF;

        cursor += 1;
    }

    // Sensor Status Annunciation
    if((meas_val->flags & GLP_MEAS_SENS_STAT_ANNUN_PRES) != 0)
    {
        meas_val->status = co_read16p(packed_meas + cursor);
        cursor += 2;
    }

    return cursor;}


uint8_t glpc_unpack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                   struct glp_meas_ctx* meas_ctx_val,
                                   uint16_t* seq_num)
{
    uint8_t cursor = 0;
    // Flags
    meas_ctx_val->flags = packed_meas_ctx[cursor];
    cursor += 1;

    // Sequence Number
    *seq_num = co_read16p(packed_meas_ctx + cursor);
    cursor += 2;

    // Extended Flags
    if((meas_ctx_val->flags & GLP_CTX_EXTD_F_PRES) != 0)
    {
        meas_ctx_val->ext_flags = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Carbohydrate ID And Carbohydrate Present
    if((meas_ctx_val->flags & GLP_CTX_CRBH_ID_AND_CRBH_PRES) != 0)
    {
        // Carbohydrate ID
        meas_ctx_val->carbo_id = packed_meas_ctx[cursor];
        cursor += 1;
        // Carbohydrate Present
        meas_ctx_val->carbo_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    // Meal Present
    if((meas_ctx_val->flags & GLP_CTX_MEAL_PRES) != 0)
    {
        meas_ctx_val->meal = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Tester-Health Present
    if((meas_ctx_val->flags & GLP_CTX_TESTER_HEALTH_PRES) != 0)
    {
        // Tester and Health are 2 nibble values
        meas_ctx_val->health = packed_meas_ctx[cursor] >> 4;
        meas_ctx_val->tester = packed_meas_ctx[cursor] & 0xF;
        cursor += 1;
    }

    // Exercise Duration & Exercise Intensity Present
    if((meas_ctx_val->flags & GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES) != 0)
    {
        // Exercise Duration
        meas_ctx_val->exercise_dur = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;

        // Exercise Intensity
        meas_ctx_val->exercise_intens = packed_meas_ctx[cursor];
        cursor += 1;
    }

    // Medication ID And Medication Present
    if((meas_ctx_val->flags & GLP_CTX_MEDIC_ID_AND_MEDIC_PRES) != 0)
    {
        // Medication ID
        meas_ctx_val->med_id = packed_meas_ctx[cursor];
        cursor += 1;

        // Medication Present
        meas_ctx_val->med_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    // HbA1c Present
    if((meas_ctx_val->flags & GLP_CTX_HBA1C_PRES) != 0)
    {
        // HbA1c
        meas_ctx_val->hba1c_val = co_read16p(packed_meas_ctx + cursor);
        cursor += 2;
    }

    return cursor;
}

uint8_t glpc_pack_racp_req(uint8_t *packed_val,
                           const struct glp_racp_req* racp_req)
{
    uint8_t cursor = 0;

    // command op code
    packed_val[cursor] = racp_req->op_code;
    cursor++;

    // operator of the function
    packed_val[cursor] = racp_req->filter.operator;
    cursor++;

    // Abort operation don't require any other parameter
    if(racp_req->op_code == GLP_REQ_ABORT_OP)
    {
        return cursor;
    }

    // check if request requires operand (filter)
    if((racp_req->filter.operator >= GLP_OP_LT_OR_EQ)
            && (racp_req->filter.operator <= GLP_OP_WITHIN_RANGE_OF))
    {

        // command filter type
        packed_val[cursor] = racp_req->filter.filter_type;
        cursor++;

        // filter uses sequence number
        if(racp_req->filter.filter_type == GLP_FILTER_SEQ_NUMBER)
        {
            // minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // minimum value
                co_write16p(packed_val + cursor,racp_req->filter.val.seq_num.min);
                cursor +=2;
            }

            // maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // maximum value
                co_write16p(packed_val + cursor,racp_req->filter.val.seq_num.max);
                cursor +=2;
            }
        }
        // filter uses user facing time
        else
        {
            // retrieve minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                 // retrieve minimum facing time
                cursor += prf_pack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_min));
            }

            // retrieve maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // retrieve maximum facing time
                cursor += prf_pack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_max));
            }
        }
    }

    return cursor;
}

uint8_t glpc_unpack_racp_rsp(uint8_t *packed_val,
                             struct glp_racp_rsp* racp_rsp)
{
    uint8_t cursor = 0;

    // response op code
    racp_rsp->op_code = packed_val[cursor];
    cursor++;

    // operator (null)
    cursor++;

    // number of records
    if(racp_rsp->op_code == GLP_REQ_NUM_OF_STRD_RECS_RSP)
    {
        racp_rsp->operand.num_of_record = co_read16p(packed_val + cursor);
        cursor += 2;
    }
    else
    {
        // requested opcode
        racp_rsp->operand.rsp.op_code_req = packed_val[cursor];
        cursor++;
        // command status
        racp_rsp->operand.rsp.status = packed_val[cursor];
        cursor++;
    }

    return cursor;
}

uint8_t glpc_validate_request(struct glpc_env_tag *glpc_env, uint16_t conhdl, uint8_t char_code)
{
    uint8_t status = PRF_ERR_OK;

    // Check if collector enabled
    if (conhdl == gapc_get_conhdl(glpc_env->con_info.conidx))
    {
        // check if feature val characteristic exists
        if (glpc_env->gls.chars[char_code].val_hdl == ATT_INVALID_HANDLE)
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    return status;
}

#endif /* (BLE_GL_COLLECTOR) */

/// @} GLPC
