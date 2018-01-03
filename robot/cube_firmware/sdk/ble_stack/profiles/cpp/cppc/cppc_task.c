/**
 ****************************************************************************************
 *
 * @file cppc_task.c
 *
 * @brief Cycling Power Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup CPPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_CP_COLLECTOR)
#include "cpp_common.h"
#include "gap.h"
#include "attm.h"
#include "cppc.h"
#include "cppc_task.h"
#include "gattc_task.h"
#include "attm_cfg.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Cycling Power service characteristics information
const struct prf_char_def cppc_cps_char[CPP_CPS_CHAR_MAX] =
{
    /// CP Measurement
    [CPP_CPS_MEAS_CHAR]         = {ATT_CHAR_CP_MEAS,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_NTF},
    /// CP Feature
    [CPP_CPS_FEAT_CHAR]         = {ATT_CHAR_CP_FEAT,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_RD},
    /// Sensor Location
    [CPP_CPS_SENSOR_LOC_CHAR]   = {ATT_CHAR_SENSOR_LOC,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_RD},
    ///Vector
    [CPP_CPS_VECTOR_CHAR]       = {ATT_CHAR_CP_VECTOR,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_NTF},
    /// SC Control Point
    [CPP_CPS_CTNL_PT_CHAR]      = {ATT_CHAR_CP_CNTL_PT,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Cycling Power service characteristic descriptor information
const struct prf_char_desc_def cppc_cps_char_desc[CPPC_DESC_MAX] =
{
    /// CP Measurement Char. - Client Characteristic Configuration
    [CPPC_DESC_CP_MEAS_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_MANDATORY,
                                        CPP_CPS_MEAS_CHAR},

    /// CP Measurement Char. - Server Characteristic Configuration
    [CPPC_DESC_CP_MEAS_SV_CFG] = {ATT_DESC_SERVER_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        CPP_CPS_MEAS_CHAR},

    /// CP Vector Char. - Client Characteristic Configuration
    [CPPC_DESC_VECTOR_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        CPP_CPS_VECTOR_CHAR},

    /// Control Point Char. - Client Characteristic Configuration
    [CPPC_DESC_CTNL_PT_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        CPP_CPS_CTNL_PT_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Performs discovery procedure for CP service
 * @param[in] cppc_env environment variable
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cppc_discovery_procedure (struct cppc_env_tag *cppc_env)
{
    // Status
    uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

    /* -------------------------------------------------
     * SERVICE DISCOVERY -------------------------------
     * ------------------------------------------------- */
    if (cppc_env->last_req == ATT_SVC_CYCLING_POWER)
    {
        if (cppc_env->nb_svc > 0)
        {
            // Check if service handles are OK
            if ((cppc_env->cps.svc.shdl != ATT_INVALID_HANDLE) &&
                (cppc_env->cps.svc.ehdl != ATT_INVALID_HANDLE) &&
                (cppc_env->cps.svc.shdl < cppc_env->cps.svc.ehdl))
            {
                // Status is OK
                status = PRF_ERR_OK;

                // Discover all CPS characteristics
                cppc_env->last_req = ATT_DECL_CHARACTERISTIC;
                prf_disc_char_all_send(&(cppc_env->con_info), &(cppc_env->cps.svc));
            } // Handles are not corrects, the Cycling Power Service has not been found, stop
        }// The Cycling Power Service has not been found, stop discovery
    }

    /* -------------------------------------------------
     * CHARACTERISTICS DISCOVERY -----------------------
     * ------------------------------------------------- */
    else if (cppc_env->last_req == ATT_DECL_CHARACTERISTIC)
    {
        // Check if mandatory properties have been found and if properties are correct
        status = prf_check_svc_char_validity(CPP_CPS_CHAR_MAX, cppc_env->cps.chars, cppc_cps_char);

        // Check for characteristic properties.
        if (status == PRF_ERR_OK)
        {
            cppc_env->last_req = CPP_CPS_MEAS_CHAR;

            // Find the Client Characteristic Configuration Descriptor for the CP Measurement characteristic
            prf_disc_char_desc_send(&(cppc_env->con_info),
                                    &(cppc_env->cps.chars[CPP_CPS_MEAS_CHAR]));
        }
    }

    /* -------------------------------------------------
     * DESCRIPTORS DISCOVERY ---------------------------
     * ------------------------------------------------- */
    else
    {
        // Discovery over ?
        bool disc_over = true;

        if (cppc_env->last_req == CPP_CPS_MEAS_CHAR)
        {
            // Check if the Vector Characteristic has been found
            if (cppc_env->cps.chars[CPP_CPS_VECTOR_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Status is OK
                status = PRF_ERR_OK;

                // Find the Client Characteristic Configuration Descriptor for the Vector characteristic
                cppc_env->last_req = CPP_CPS_VECTOR_CHAR;
                prf_disc_char_desc_send(&(cppc_env->con_info),
                                        &(cppc_env->cps.chars[CPP_CPS_VECTOR_CHAR]));

                // Discovery continues
                disc_over = false;
            }

            else //case there is no Vector but Ctl Point
            {
                // Check if the Control Point Characteristic has been found
                if (cppc_env->cps.chars[CPP_CPS_CTNL_PT_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Status is OK
                    status = PRF_ERR_OK;

                    // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                    cppc_env->last_req = CPP_CPS_CTNL_PT_CHAR;
                    prf_disc_char_desc_send(&(cppc_env->con_info),
                                            &(cppc_env->cps.chars[CPP_CPS_CTNL_PT_CHAR]));

                    // Discovery continues
                    disc_over = false;
                }
            }
        }

        else if (cppc_env->last_req == CPP_CPS_VECTOR_CHAR)
        {
            // Check if the Control Point Characteristic has been found
            if (cppc_env->cps.chars[CPP_CPS_CTNL_PT_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Status is OK
                status = PRF_ERR_OK;
                // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                cppc_env->last_req = CPP_CPS_CTNL_PT_CHAR;
                prf_disc_char_desc_send(&(cppc_env->con_info),
                                        &(cppc_env->cps.chars[CPP_CPS_CTNL_PT_CHAR]));
                // Discovery continues
                disc_over = false;
            }
        }

        else
        {
            // Discovery is over
            ASSERT_ERR(cppc_env->last_req == CPP_CPS_CTNL_PT_CHAR);
        }

        if (disc_over)
        {
            status = prf_check_svc_char_desc_validity(CPPC_DESC_MAX,
                                                      cppc_env->cps.descs,
                                                      cppc_cps_char_desc,
                                                      cppc_env->cps.chars);

            if (status == PRF_ERR_OK)
            {
                // Reset number of services
                cppc_env->nb_svc = 0;

                // Register in GATT for notifications/indications
                prf_register_atthdl2gatt(&cppc_env->con_info, &cppc_env->cps.svc);

                // Send the content of the service to the HL
                struct cppc_cps_content_ind *ind = KE_MSG_ALLOC(CPPC_CPS_CONTENT_IND,
                                                                  cppc_env->con_info.appid,
                                                                  cppc_env->con_info.prf_id,
                                                                  cppc_cps_content_ind);

                ind->conhdl = gapc_get_conhdl(cppc_env->con_info.conidx);
                memcpy(&ind->cps, &cppc_env->cps, sizeof(struct cppc_cps_content));

                // Send the message
                ke_msg_send(ind);

                // Stop discovery procedure.
                cppc_send_cmp_evt(cppc_env, CPPC_ENABLE_OP_CODE, status);
            }
        }
    }
    return status;
}

/**
 ****************************************************************************************
 * @brief Gets correct read handle according to the request
 * @param[in] cppc_env environment variable
 * @param[in] param Pointer to the parameters of the message.
 * @return handle
 ****************************************************************************************
 */
static uint16_t cppc_get_read_handle_req (struct cppc_env_tag *cppc_env, struct cppc_read_cmd *param)
{
    // Attribute Handle
    uint16_t handle = ATT_INVALID_SEARCH_HANDLE;

    switch (param->read_code)
    {
        // Read CP Feature
        case (CPPC_RD_CP_FEAT):
        {
            handle = cppc_env->cps.chars[CPP_CPS_FEAT_CHAR].val_hdl;
        } break;

        // Read Sensor Location
        case (CPPC_RD_SENSOR_LOC):
        {
            handle = cppc_env->cps.chars[CPP_CPS_SENSOR_LOC_CHAR].val_hdl;
        } break;

        // Read CP Measurement Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_CL_CFG):
        {
            handle = cppc_env->cps.descs[CPPC_DESC_CP_MEAS_CL_CFG].desc_hdl;
        } break;

        // Read CP Measurement Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_SV_CFG):
        {
            handle = cppc_env->cps.descs[CPPC_DESC_CP_MEAS_SV_CFG].desc_hdl;
        } break;

        // Read CP Vector Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_VECTOR_CFG):
        {
            handle = cppc_env->cps.descs[CPPC_DESC_VECTOR_CL_CFG].desc_hdl;
        } break;

        // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CTNL_PT_CFG):
        {
            handle = cppc_env->cps.descs[CPPC_DESC_CTNL_PT_CL_CFG].desc_hdl;
        } break;

        default:
        {
            handle = ATT_INVALID_SEARCH_HANDLE;
        } break;
    }

    return handle;
}

/**
 ****************************************************************************************
 * @brief Gets correct write handle according to the request
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @param[out] handle handle
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cppc_get_write_desc_handle_req (struct cppc_cfg_ntfind_cmd *param, struct cppc_env_tag *cppc_env, uint16_t *handle)
{
    // Status
    uint8_t status    = PRF_ERR_OK;

    switch(param->desc_code)
    {
        // Write CP Measurement Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_CL_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = cppc_env->cps.descs[CPPC_DESC_CP_MEAS_CL_CFG].desc_hdl;

                // The descriptor is mandatory
                ASSERT_ERR(*handle != ATT_INVALID_SEARCH_HANDLE);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        } break;

        // Write CP Measurement Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_SV_CFG):
        {
            if (param->ntfind_cfg <= PRF_SRV_START_BCST)
            {
                *handle = cppc_env->cps.descs[CPPC_DESC_CP_MEAS_SV_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        break;

        // Write CP Vector Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_VECTOR_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = cppc_env->cps.descs[CPPC_DESC_VECTOR_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        break;

        // Write SC Control Point Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CTNL_PT_CFG):
        {
            if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                (param->ntfind_cfg == PRF_CLI_START_IND))
            {
                *handle = cppc_env->cps.descs[CPPC_DESC_CTNL_PT_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        } break;

        default:
        {
            status = PRF_ERR_INVALID_PARAM;
        } break;
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Unpacks measurement data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */
static uint8_t cppc_unpack_meas_ind (struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // CP Measurement value has been received
    struct cppc_value_ind *ind = KE_MSG_ALLOC(CPPC_VALUE_IND,
            cppc_env->con_info.appid,
            cppc_env->con_info.prf_id,
            cppc_value_ind);

    // Offset
    uint8_t offset = CPP_CP_MEAS_MIN_LEN;

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(cppc_env->con_info.conidx);
    // Attribute code
    ind->att_code = CPPC_NTF_CP_MEAS;

    // Flags
    ind->value.cp_meas.flags = co_read16p(&param->value[0]);
    // Instant power
    ind->value.cp_meas.inst_power = co_read16p(&param->value[2]);

    if (ind->value.cp_meas.flags & CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT)
    {
        //Unpack Pedal Power Balance info
        ind->value.cp_meas.pedal_power_balance = param->value[offset];
        offset++;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_ACCUM_TORQUE_PRESENT)
    {
        //Unpack Accumulated Torque info
        ind->value.cp_meas.accum_torque = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_WHEEL_REV_DATA_PRESENT)
    {
        //Unpack Wheel Revolution Data (Cumulative Wheel & Last Wheel Event Time)
        ind->value.cp_meas.cumul_wheel_rev = co_read32p(&param->value[offset]);
        offset += 4;
        ind->value.cp_meas.last_wheel_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_CRANK_REV_DATA_PRESENT)
    {
        //Unpack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
        ind->value.cp_meas.cumul_crank_rev = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.last_crank_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT)
    {
        //Unpack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
        ind->value.cp_meas.max_force_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.min_force_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
    }

    else if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT)
    {
        //Unpack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
        ind->value.cp_meas.max_torque_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.min_torque_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_ANGLES_PRESENT)
    {
        //Unpack Extreme Angles (Maximum Angle & Minimum Angle)
        uint32_t angle = co_read24p(&param->value[offset]);
        offset += 3;

        //Force to 12 bits
        ind->value.cp_meas.max_angle = (angle & (0x0FFF));
        ind->value.cp_meas.min_angle = ((angle>>12) & 0x0FFF);
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT)
    {
        //Unpack Top Dead Spot Angle
        ind->value.cp_meas.top_dead_spot_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT)
    {
        //Unpack Bottom Dead Spot Angle
        ind->value.cp_meas.bot_dead_spot_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_ACCUM_ENERGY_PRESENT)
    {
        //Unpack Accumulated Energy
        ind->value.cp_meas.accum_energy = co_read16p(&param->value[offset]);
        offset += 2;
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}

/**
 ****************************************************************************************
 * @brief Unpacks Vector data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */
static uint8_t cppc_unpack_vector_ind (struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // CP Measurement value has been received
    struct cppc_value_ind *ind = KE_MSG_ALLOC_DYN(CPPC_VALUE_IND,
                                                    cppc_env->con_info.appid,
                                                    cppc_env->con_info.prf_id,
                                                    cppc_value_ind,
                                                    param->length);

    // Offset
    uint8_t offset = CPP_CP_VECTOR_MIN_LEN;

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(cppc_env->con_info.conidx);
    // Attribute code
    ind->att_code = CPPC_NTF_CP_VECTOR;

    // Flags
    ind->value.cp_vector.flags = param->value[0];

    if (ind->value.cp_vector.flags & CPP_VECTOR_CRANK_REV_DATA_PRESENT)
    {
        // Unpack Crank Revolution Data
        ind->value.cp_vector.cumul_crank_rev = co_read16p(&param->value[offset]);
        offset += 2;
        // Unpack Last Crank Evt time
        ind->value.cp_vector.last_crank_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_vector.flags & CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT)
    {
        // Unpack First Crank Measurement Angle
        ind->value.cp_vector.first_crank_meas_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (!(ind->value.cp_vector.flags & CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT) !=
            !(ind->value.cp_vector.flags & CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT))
    {
        // Unpack Force or Torque magnitude (mutually excluded)
        ind->value.cp_vector.nb = (param->length - offset)/2;
        if (ind->value.cp_vector.nb)
        {
            for (int i = 0; i < ind->value.cp_vector.nb; i++)
            {
                // Handle the array buffer to extract parameters
                ind->value.cp_vector.force_torque_magnitude[i] = co_read16p(&param->value[offset]);
                offset += 2;
            }
        }
    }

//    if ((ind->value.cp_vector.flags & CPP_VECTOR_INST_MEAS_DIRECTION_LSB) ||
//            (ind->value.cp_vector.flags & CPP_VECTOR_INST_MEAS_DIRECTION_MSB))
//    {
//
//    }

    // Send the message
    ke_msg_send(ind);

    return offset;

}

/**
 ****************************************************************************************
 * @brief Packs Control Point data
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] req packed message
 * @param[out] status status of the operation
 * @return length
 ****************************************************************************************
 */
static uint8_t cppc_pack_ctnl_pt_req (struct cppc_ctnl_pt_cfg_cmd *param, uint8_t *req, uint8_t *status)
{
    // Request Length
    uint8_t req_len = CPP_CP_CNTL_PT_REQ_MIN_LEN;

    // Set the operation code
    req[0] = param->ctnl_pt.op_code;

    // Fulfill the message according to the operation code
    switch (param->ctnl_pt.op_code)
    {
        case (CPP_CTNL_PT_SET_CUMUL_VAL):
        {
            // Set the cumulative value
            co_write32p(&req[req_len], param->ctnl_pt.value.cumul_val);
            // Update length
            req_len += 4;
        } break;

        case (CPP_CTNL_PT_UPD_SENSOR_LOC):
        {
            // Set the sensor location
            req[req_len] = param->ctnl_pt.value.sensor_loc;
            // Update length
            req_len++;
        } break;

        case (CPP_CTNL_PT_SET_CRANK_LENGTH):
        {
            // Set the crank length
            co_write16p(&req[req_len], param->ctnl_pt.value.crank_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
        {
            // Set the chain length
            co_write16p(&req[req_len], param->ctnl_pt.value.chain_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
        {
            // Set the chain weight
            co_write16p(&req[req_len], param->ctnl_pt.value.chain_weight);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_SPAN_LENGTH):
        {
            // Set the span length
            co_write16p(&req[req_len], param->ctnl_pt.value.span_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
        {
            // Set the Content Mask
            co_write16p(&req[req_len], param->ctnl_pt.value.mask_content);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
        case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
        case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
        case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
        case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
        case (CPP_CTNL_PT_START_OFFSET_COMP):
        case (CPP_CTNL_REQ_SAMPLING_RATE):
        case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
        {
            // Nothing more to do
        } break;

        default:
        {
            *status = PRF_ERR_INVALID_PARAM;
        } break;
    }

    return req_len;
}

/**
 ****************************************************************************************
 * @brief Unpacks Control Point data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */
static uint8_t cppc_unpack_ctln_pt_ind (struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // Offset
    uint8_t offset = CPP_CP_CNTL_PT_RSP_MIN_LEN;

    // Control Point value has been received
    struct cppc_ctnl_pt_rsp_ind *ind = KE_MSG_ALLOC(CPPC_CTNL_PT_RSP_IND,
            cppc_env->con_info.appid,
            cppc_env->con_info.prf_id,
            cppc_ctnl_pt_rsp_ind);

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(cppc_env->con_info.conidx);

    // Requested operation code
    ind->rsp.req_op_code = param->value[1];
    // Response value
    ind->rsp.resp_value  = param->value[2];

    if ((ind->rsp.resp_value == CPP_CTNL_PT_RESP_SUCCESS) && (param->length >= 3))
    {
        switch (ind->rsp.req_op_code)
        {
            case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
            {
                // Get the number of supported locations that have been received
               uint8_t nb_supp_loc = (param->length - 3);
               // Location
               uint8_t loc;

               for (uint8_t counter = 0; counter < nb_supp_loc; counter++)
               {
                   loc = param->value[counter + CPP_CP_CNTL_PT_RSP_MIN_LEN];
                   // Check if valid
                   if (loc < CPP_LOC_MAX)
                   {
                       ind->rsp.value.supp_loc |= (1 << loc);
                   }
                   offset++;
               }
            } break;

            case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
            {
                ind->rsp.value.crank_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
            {
                ind->rsp.value.chain_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
            {
                ind->rsp.value.chain_weight = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
            {
                ind->rsp.value.span_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_START_OFFSET_COMP):
            {
                ind->rsp.value.offset_comp = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_REQ_SAMPLING_RATE):
            {
                ind->rsp.value.sampling_rate = param->value[offset];
                offset++;
            } break;

            case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
            {
                offset += prf_unpack_date_time(
                        (uint8_t *) &(param->value[offset]), &(ind->rsp.value.factory_calibration));
            } break;

            case (CPP_CTNL_PT_SET_CUMUL_VAL):
            case (CPP_CTNL_PT_UPD_SENSOR_LOC):
            case (CPP_CTNL_PT_SET_CRANK_LENGTH):
            case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
            case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
            case (CPP_CTNL_PT_SET_SPAN_LENGTH):
            case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
            {
                // No parameters
            } break;

            default:
            {

            } break;
        }
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}

/*
 * HANDLER FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPC_ENABLE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cppc_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct cppc_enable_cmd *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Cycling Power Profile Collector Role Task Environment
    struct cppc_env_tag *cppc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Check if the provided conhdl is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Fill the Connection Information structure
        con_info.conidx = gapc_get_conidx(param->conhdl);
        con_info.prf_id = dest_id;
        con_info.appid  = src_id;

        // Add an environment for the provided connection
        status = PRF_CLIENT_ENABLE(con_info, param, cppc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

        // Keep the connection info
        memcpy(&cppc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            cppc_env->last_req  = ATT_SVC_CYCLING_POWER;
            // Force the operation value
            param->operation     = CPPC_ENABLE_OP_CODE;

            prf_disc_svc_send(&(cppc_env->con_info), ATT_SVC_CYCLING_POWER);

            // Keep the operation
            cppc_env->operation = (void *)param;
            // Do not free the message
            msg_status = KE_MSG_NO_FREE;

        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&cppc_env->cps, &param->cps, sizeof(struct cppc_cps_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&cppc_env->con_info, &cppc_env->cps.svc);

            // Send a complete event status to the application
            cppc_send_cmp_evt(cppc_env, CPPC_ENABLE_OP_CODE, PRF_ERR_OK);
        }

        // Go to BUSY state
        ke_state_set(dest_id, CPPC_BUSY);
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message will be forwarded towards the good task instance
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        cppc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CPPC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cppc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct cppc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        // Attribute Handle
        uint16_t hdl;
        // Status
        uint8_t status     = PRF_ERR_OK;

        ASSERT_ERR(ke_state_get(dest_id) != CPPC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == cppc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == CPPC_BUSY)
            {
                // Keep the request for later
                msg_status = KE_MSG_SAVED;
            }
            else    // State is CPPC_CONNECTED
            {
                ASSERT_ERR(cppc_env->operation == NULL);

                // Get the handler
                hdl = cppc_get_read_handle_req (cppc_env, param);

                // Check if handle is viable
                if (hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Force the operation value
                    param->operation = CPPC_READ_OP_CODE;

                    // Store the command structure
                    cppc_env->operation = param;
                    msg_status = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(cppc_env->con_info), cppc_env->cps.svc.shdl,
                                       cppc_env->cps.svc.ehdl, hdl);

                    // Go to the Busy state
                    ke_state_set(dest_id, CPPC_BUSY);
                }
                else
                {
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }

        if (status != PRF_ERR_OK)
        {
            // Send the complete event message to the task id stored in the environment
            cppc_send_cmp_evt(cppc_env, CPPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists, send the status to the requester
        cppc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CPPC_READ_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cppc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct cppc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_OK;
        // Handle
        uint16_t handle = ATT_INVALID_SEARCH_HANDLE;

        do
        {
            ASSERT_INFO(ke_state_get(dest_id) != CPPC_IDLE, ke_state_get(dest_id), 0);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != cppc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == CPPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // state = CPPC_CONNECTED
            ASSERT_ERR(cppc_env->operation == NULL);

            // Get handle
            status = cppc_get_write_desc_handle_req (param, cppc_env, &handle);
        } while (0);

        if (status == PRF_ERR_OK)
        {
            if (handle != ATT_INVALID_SEARCH_HANDLE)
            {
                // Set the operation code
                param->operation = CPPC_CFG_NTF_IND_OP_CODE;

                // Store the command structure
                cppc_env->operation = param;
                msg_status = KE_MSG_NO_FREE;

                // Go to the Busy state
                ke_state_set(dest_id, CPPC_BUSY);

                // Send GATT Write Request
                prf_gatt_write_ntf_ind(&cppc_env->con_info, handle, param->ntfind_cfg);
            }
            else
            {
                cppc_send_cmp_evt(cppc_env, CPPC_CFG_NTF_IND_OP_CODE, PRF_ERR_INEXISTENT_HDL);
            }
        }
        else
        {
            cppc_send_cmp_evt(cppc_env, CPPC_CFG_NTF_IND_OP_CODE, status);
        }
    }
    else
    {
        cppc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CPPC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPC_CTNL_PT_CFG_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cppc_ctnl_pt_cfg_cmd_handler(ke_msg_id_t const msgid,
                                         struct cppc_ctnl_pt_cfg_cmd *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > CPPC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != cppc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == CPPC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (cppc_env->cps.descs[CPPC_DESC_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Request array declaration
                uint8_t req[CPP_CP_CNTL_PT_REQ_MAX_LEN];
                // Pack request
                uint8_t nb = cppc_pack_ctnl_pt_req (param, req, &status);

                if (status == PRF_ERR_OK)
                {
                    // Set the operation code
                    param->operation = CPPC_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    cppc_env->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, CPPC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(cppc_env->con_info), cppc_env->cps.chars[CPP_CPS_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], nb, GATTC_WRITE);
                }
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send a complete event status to the application
            cppc_send_cmp_evt(cppc_env, CPPC_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        cppc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, CPPC_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cppc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        ASSERT_ERR(cppc_env->operation != NULL);
        ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        cppc_send_cmp_evt(cppc_env, CPPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_svc_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_disc_svc_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    // Check if the environment exists
    if (cppc_env != NULL)
    {
        ASSERT_ERR(cppc_env->operation != NULL);
        ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (cppc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            cppc_env->cps.svc.shdl = param->start_hdl;
            cppc_env->cps.svc.ehdl = param->end_hdl;
            cppc_env->nb_svc++;
        }
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_disc_char_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    // Check if the environment exists
    if (cppc_env != NULL)
    {
        ASSERT_ERR(cppc_env->operation != NULL);
        ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_ENABLE_OP_CODE);

        // Retrieve CPS characteristics
        prf_search_chars(cppc_env->cps.svc.ehdl, CPP_CPS_CHAR_MAX,
                         &cppc_env->cps.chars[0], &cppc_cps_char[0],
                         param, (uint8_t *)&cppc_env->last_char_code);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_desc_ind_handler(ke_msg_id_t const msgid,
                                           struct gattc_disc_char_desc_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    // Check if the environment exists
    if (cppc_env != NULL)
    {
        ASSERT_ERR(cppc_env->operation != NULL);
        ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_ENABLE_OP_CODE);

        // Retrieve CPS descriptors
        prf_search_descs(CPPC_DESC_MAX, &cppc_env->cps.descs[0], &cppc_cps_char_desc[0],
                         param, cppc_env->last_req);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        switch (param->req_type)
        {
            case (GATTC_DISC_BY_UUID_SVC):
            case (GATTC_DISC_ALL_CHAR):
            case (GATTC_DISC_DESC_CHAR):
            {
                // Status
                uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

                ASSERT_ERR(cppc_env->operation != NULL);
                ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_ENABLE_OP_CODE);

                if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (param->status == ATT_ERR_NO_ERROR))
                {
                    // Discovery
                    status = cppc_discovery_procedure (cppc_env);
                }
                else
                {
                    status = param->status;
                }

                if (status != PRF_ERR_OK)
                {
                    // Stop discovery procedure.
                    cppc_send_cmp_evt(cppc_env, CPPC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_READ):
            {
                // Send the complete event status
                cppc_send_cmp_evt(cppc_env, CPPC_READ_OP_CODE, param->status);
            } break;

            case (GATTC_WRITE):
            {
                uint8_t operation = ((struct cppc_cmd *)cppc_env->operation)->operation;

                if (operation == CPPC_CFG_NTF_IND_OP_CODE)
                {
                    // Send the complete event status
                    cppc_send_cmp_evt(cppc_env, operation, param->status);
                }

                else if (operation == CPPC_CTNL_PT_CFG_WR_OP_CODE)
                {
                    if (param->status == PRF_ERR_OK)
                    {
                        // Start Timeout Procedure
                        ke_timer_set(CPPC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                        // Wait for the response indication
                        ((struct cppc_cmd *)cppc_env->operation)->operation = CPPC_CTNL_PT_CFG_IND_OP_CODE;
                    }
                    else
                    {
                        // Send the complete event status
                        cppc_send_cmp_evt(cppc_env, operation, param->status);
                    }
                }
            } break;

            case (GATTC_REGISTER):
            case (GATTC_UNREGISTER):
            {
                // Do nothing
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        ASSERT_ERR(cppc_env->operation != NULL);
        ASSERT_ERR(((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_READ_OP_CODE);

        // Send the read value to the HL
        struct cppc_value_ind *ind = KE_MSG_ALLOC(CPPC_VALUE_IND,
                                                   cppc_env->con_info.appid,
                                                   cppc_env->con_info.prf_id,
                                                   cppc_value_ind);

        ind->conhdl   = gapc_get_conhdl(cppc_env->con_info.conidx);

        switch (((struct cppc_read_cmd *)cppc_env->operation)->read_code)
        {
            // Read CP Feature Characteristic value
            case (CPPC_RD_CP_FEAT):
            {
                ind->value.sensor_feat = co_read32p(&param->value[0]);

                // Mask the reserved bits
//                ind->value.sensor_feat &= CPP_FEAT_ALL_SUPP;
            } break;

            // Read Sensor Location Characteristic value
            case (CPPC_RD_SENSOR_LOC):
            {
                ind->value.sensor_loc = param->value[0];
            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (CPPC_RD_WR_CP_MEAS_CL_CFG):
            case (CPPC_RD_WR_CP_MEAS_SV_CFG):
            case (CPPC_RD_WR_VECTOR_CFG):
            case (CPPC_RD_WR_CTNL_PT_CFG):
            {
                co_write16p(&ind->value.ntf_cfg, param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct cppc_read_cmd *)cppc_env->operation)->read_code;

        // Send the message to the application
        ke_msg_send(ind);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                   struct gattc_event_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    if (cppc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {

                if (param->handle == cppc_env->cps.chars[CPP_CPS_MEAS_CHAR].val_hdl)
                {
                    //Unpack measurement
                    cppc_unpack_meas_ind (param, cppc_env);
                }
                else if (param->handle == cppc_env->cps.chars[CPP_CPS_VECTOR_CHAR].val_hdl)
                {
                    //Unpack vector
                    cppc_unpack_vector_ind (param, cppc_env);
                }
                else
                {
                    ASSERT_ERR(0);
                }
            } break;

            case (GATTC_INDICATE):
            {
                // Check if we were waiting for the indication
                if (cppc_env->operation != NULL)
                {
                    if (((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(CPPC_TIMEOUT_TIMER_IND, dest_id);

                        //Unpack control point
                        cppc_unpack_ctln_pt_ind (param, cppc_env);

                        // Send the complete event message
                        cppc_send_cmp_evt(cppc_env, CPPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_OK);
                    }
                    // else drop the message
                }
                // else drop the message
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to CPPC.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cppc_env_tag *cppc_env = PRF_CLIENT_GET_ENV(dest_id, cppc);

    ASSERT_ERR(cppc_env != NULL);

    // Free the stored operation if needed
    if (cppc_env->operation != NULL)
    {
        // Check if we were waiting for a SC Control Point indication
        if (((struct cppc_cmd *)cppc_env->operation)->operation == CPPC_CTNL_PT_CFG_IND_OP_CODE)
        {
            // Stop the procedure timeout timer
            ke_timer_clear(CPPC_TIMEOUT_TIMER_IND, dest_id);
        }

        ke_msg_free(ke_param2msg(cppc_env->operation));
        cppc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(cppc_envs, dest_id, CPPC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler cppc_default_state[] =
{
    {CPPC_ENABLE_CMD,               (ke_msg_func_t)cppc_enable_cmd_handler},
    {CPPC_READ_CMD,                 (ke_msg_func_t)cppc_read_cmd_handler},
    {CPPC_CFG_NTFIND_CMD,           (ke_msg_func_t)cppc_cfg_ntfind_cmd_handler},
    {CPPC_CTNL_PT_CFG_CMD,          (ke_msg_func_t)cppc_ctnl_pt_cfg_cmd_handler},

    {CPPC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)cppc_timeout_timer_ind_handler},

    {GATTC_DISC_SVC_IND,             (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,            (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,       (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                 (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                  (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler cppc_state_handler[CPPC_STATE_MAX] =
{
    [CPPC_IDLE]        = KE_STATE_HANDLER_NONE,
    [CPPC_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [CPPC_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler cppc_default_handler = KE_STATE_HANDLER(cppc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t cppc_state[CPPC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_CP_COLLECTOR)

/// @} CPPCTASK
