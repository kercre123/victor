/**
 ****************************************************************************************
 *
 * @file glps.c
 *
 * @brief Glucose Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GLPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GL_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "glps.h"
#include "glps_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct glps_env_tag glps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// GLPS task descriptor
static const struct ke_task_desc TASK_DESC_GLPS = {glps_state_handler, &glps_default_handler, glps_state, GLPS_STATE_MAX, GLPS_IDX_MAX};


/*
 * DEFINES
 ****************************************************************************************
 */

#define GLPS_FILTER_USER_FACING_TIME_SIZE (7)

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void glps_init(void)
{
    // Reset environment
    memset(&glps_env, 0, sizeof(glps_env));

    // Create GLPS task
    ke_task_create(TASK_GLPS, &TASK_DESC_GLPS);

    // Go to IDLE state
    ke_state_set(TASK_GLPS, GLPS_DISABLED);
}

uint8_t glps_pack_meas_value(uint8_t *packed_meas, const struct glp_meas* meas_val,
                             uint16_t seq_num)
{
    uint8_t cursor = 0;

    // Flags
    packed_meas[cursor] = meas_val->flags;
    cursor += 1;

    // Sequence Number
    co_write16p(packed_meas + cursor, seq_num);
    cursor += 2;

    // Base Time
    cursor += prf_pack_date_time(packed_meas + cursor, &(meas_val->base_time));

    //Time Offset
    if((meas_val->flags & GLP_MEAS_TIME_OFF_PRES) != 0)
    {
        co_write16p(packed_meas + cursor, meas_val->time_offset);
        cursor += 2;
    }

    // Glucose Concentration, type and location
    if((meas_val->flags & GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES) != 0)
    {
        co_write16p(packed_meas + cursor, meas_val->concentration);
        cursor += 2;

        /* type and location are 2 nibble values */
        packed_meas[cursor] = (meas_val->location << 4) | (meas_val->type);
        cursor += 1;
    }

    // Sensor Status Annunciation
    if((meas_val->flags & GLP_MEAS_SENS_STAT_ANNUN_PRES) != 0)
    {
        co_write16p(packed_meas + cursor, meas_val->status);
        cursor += 2;
    }

    return cursor;
}


uint8_t glps_pack_meas_ctx_value(uint8_t *packed_meas_ctx,
                                 const struct glp_meas_ctx* meas_ctx_val,
                                 uint16_t seq_num)
{
    uint8_t cursor = 0;
    // Flags
    packed_meas_ctx[cursor] = meas_ctx_val->flags;
    cursor += 1;

    // Sequence Number
    co_write16p(packed_meas_ctx + cursor, seq_num);
    cursor += 2;

    // Extended Flags
    if((meas_ctx_val->flags & GLP_CTX_EXTD_F_PRES) != 0)
    {
        packed_meas_ctx[cursor] = meas_ctx_val->ext_flags;
        cursor += 1;
    }

    // Carbohydrate ID And Carbohydrate Present
    if((meas_ctx_val->flags & GLP_CTX_CRBH_ID_AND_CRBH_PRES) != 0)
    {
        // Carbohydrate ID
        packed_meas_ctx[cursor] = meas_ctx_val->carbo_id;
        cursor += 1;
        // Carbohydrate Present
        co_write16p(packed_meas_ctx + cursor, meas_ctx_val->carbo_val);
        cursor += 2;
    }

    // Meal Present
    if((meas_ctx_val->flags & GLP_CTX_MEAL_PRES) != 0)
    {
        packed_meas_ctx[cursor] = meas_ctx_val->meal;
        cursor += 1;
    }

    // Tester-Health Present
    if((meas_ctx_val->flags & GLP_CTX_TESTER_HEALTH_PRES) != 0)
    {
        // Tester and Health are 2 nibble values
        packed_meas_ctx[cursor] = (meas_ctx_val->health << 4) | (meas_ctx_val->tester);
        cursor += 1;
    }

    // Exercise Duration & Exercise Intensity Present
    if((meas_ctx_val->flags & GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES) != 0)
    {
        // Exercise Duration
        co_write16p(packed_meas_ctx + cursor, meas_ctx_val->exercise_dur);
        cursor += 2;

        // Exercise Intensity
        packed_meas_ctx[cursor] = meas_ctx_val->exercise_intens;
        cursor += 1;
    }

    // Medication ID And Medication Present
    if((meas_ctx_val->flags & GLP_CTX_MEDIC_ID_AND_MEDIC_PRES) != 0)
    {
        // Medication ID
        packed_meas_ctx[cursor] = meas_ctx_val->med_id;
        cursor += 1;

        // Medication Present
        co_write16p(packed_meas_ctx + cursor, meas_ctx_val->med_val);
        cursor += 2;
    }

    // HbA1c Present
    if((meas_ctx_val->flags & GLP_CTX_HBA1C_PRES) != 0)
    {
        // HbA1c
        co_write16p(packed_meas_ctx + cursor, meas_ctx_val->hba1c_val);
        cursor += 2;
    }

    return cursor;
}

uint8_t glps_unpack_racp_req(uint8_t *packed_val, uint16_t length,
                                        struct glp_racp_req* racp_req)
{
    uint8_t cursor = 0;

    // verify that enough data present to load operation filter
    if(length < 2)
    {
        return PRF_APP_ERROR;
    }

    // retrieve command op code
    racp_req->op_code = packed_val[cursor];
    cursor++;

    // clear filter structure
    memset(&(racp_req->filter), 0, sizeof(struct glp_filter));


    // retrieve operator of the function
    racp_req->filter.operator = packed_val[cursor];
    cursor++;

    // check if opcode is supported
    if((racp_req->op_code < GLP_REQ_REP_STRD_RECS)
       || (racp_req->op_code > GLP_REQ_REP_NUM_OF_STRD_RECS))
    {
        return GLP_RSP_OP_CODE_NOT_SUP;
    }

    // Abort operation don't require any other parameter
    if(racp_req->op_code == GLP_REQ_ABORT_OP)
    {
        return PRF_ERR_OK;
    }

    // check if operator is valid
    if(racp_req->filter.operator < GLP_OP_ALL_RECS)
    {
        return GLP_RSP_INVALID_OPERATOR;
    }
    // check if operator is supported
    else if(racp_req->filter.operator > GLP_OP_LAST_REC)
    {
        return GLP_RSP_OPERATOR_NOT_SUP;
    }

    // check if request requires operand (filter)
    if((racp_req->filter.operator >= GLP_OP_LT_OR_EQ)
            && (racp_req->filter.operator <= GLP_OP_WITHIN_RANGE_OF))
    {
        // verify that enough data present to load operand filter
        if(length < cursor)
        {
            return  GLP_RSP_INVALID_OPERAND;
        }

        // retrieve command filter type
        racp_req->filter.filter_type = packed_val[cursor];
        cursor++;

        // filter uses sequence number
        if(racp_req->filter.filter_type == GLP_FILTER_SEQ_NUMBER)
        {
            // retrieve minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // check sufficient data available
                if((length - cursor) < 2)
                {
                    return GLP_RSP_INVALID_OPERAND;
                }

                // retrieve minimum value
                racp_req->filter.val.seq_num.min = co_read16p(packed_val + cursor);
                cursor +=2;
            }

            // retrieve maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                if((length - cursor) < 2)
                {
                    return GLP_RSP_INVALID_OPERAND;
                }

                // retrieve maximum value
                racp_req->filter.val.seq_num.max = co_read16p(packed_val + cursor);
                cursor +=2;
            }


            // check that range value is valid
            if((racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF)
                    && (racp_req->filter.val.seq_num.min > racp_req->filter.val.seq_num.max))
            {
                return GLP_RSP_INVALID_OPERAND;
            }
        }
        // filter uses user facing time
        else if (racp_req->filter.filter_type == GLP_FILTER_USER_FACING_TIME)
        {
            // retrieve minimum value
            if((racp_req->filter.operator == GLP_OP_GT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                // check sufficient data available
                if((length - cursor) < GLPS_FILTER_USER_FACING_TIME_SIZE)
                {
                    return GLP_RSP_INVALID_OPERAND;
                }

                // retrieve minimum facing time
                cursor += prf_unpack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_min));
                cursor +=2;
            }

            // retrieve maximum value
            if((racp_req->filter.operator == GLP_OP_LT_OR_EQ)
                    || (racp_req->filter.operator == GLP_OP_WITHIN_RANGE_OF))
            {
                if((length - cursor) < GLPS_FILTER_USER_FACING_TIME_SIZE)
                {
                    return GLP_RSP_INVALID_OPERAND;
                }

                // retrieve maximum facing time
                cursor += prf_unpack_date_time((packed_val + cursor),
                        &(racp_req->filter.val.time.facetime_max));
            }
        }
        else
        {
            return GLP_RSP_OPERAND_NOT_SUP;
        }
    }
    else if ((length - cursor) != 0)
    {
        return GLP_RSP_INVALID_OPERAND;
    }

    // Check that operand is valid
    return PRF_ERR_OK;
}

uint8_t glps_pack_racp_rsp(uint8_t *packed_val,
                                      struct glp_racp_rsp* racp_rsp)
{
    uint8_t cursor = 0;

    // set response op code
    packed_val[cursor] = racp_rsp->op_code;
    cursor++;

    // set operator (null)
    packed_val[cursor] = 0;
    cursor++;

    // number of record
    if(racp_rsp->op_code == GLP_REQ_NUM_OF_STRD_RECS_RSP)
    {
        co_write16(packed_val+cursor, racp_rsp->operand.num_of_record);
        cursor += 2;
    }
    else
    {
        // requested opcode
        packed_val[cursor] = racp_rsp->operand.rsp.op_code_req;;
        cursor++;
        // command status
        packed_val[cursor] = racp_rsp->operand.rsp.status;
        cursor++;
    }

    return cursor;
}

uint8_t glps_send_racp_rsp(struct glp_racp_rsp * racp_rsp,
                                      ke_task_id_t racp_ind_src)
{
    uint8_t status = PRF_ERR_IND_DISABLED;
    if((glps_env.evt_cfg & GLPS_RACP_IND_CFG) != 0)
    {
        struct attm_elmt * attm_elmt;

        // save response requester task
        glps_env.racp_ind_src = racp_ind_src;

        // retrieve attdb data value pointers
        attm_elmt = attmdb_get_attribute(GLPS_HANDLE(GLS_IDX_REC_ACCESS_CTRL_VAL));

        // pack data (updates database)
        attm_elmt->length = glps_pack_racp_rsp(attm_elmt->value, racp_rsp);

        //Send indication through GATT
        prf_server_send_event((prf_env_struct *)&glps_env, true,
                GLPS_HANDLE(GLS_IDX_REC_ACCESS_CTRL_VAL));

        status = PRF_ERR_OK;
    }

    return status;
}

void glps_disable(uint8_t status, uint16_t conhdl) 
{
    //Disable GLS in database
    attmdb_svc_set_permission(glps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct glps_disable_ind *ind = KE_MSG_ALLOC(GLPS_DISABLE_IND,
                                                glps_env.con_info.appid, TASK_GLPS,
                                                glps_disable_ind);

    ind->conhdl = conhdl;
    ind->evt_cfg = glps_env.evt_cfg;

    ke_msg_send(ind);

    //Reset indications/notifications bit field
    glps_env.evt_cfg = 0;

    GLPS_CLEAR(ENABLE);

    //Go to idle state
    ke_state_set(TASK_GLPS, GLPS_IDLE);
}

#endif /* BLE_GL_SENSOR */

/// @} GLPS
