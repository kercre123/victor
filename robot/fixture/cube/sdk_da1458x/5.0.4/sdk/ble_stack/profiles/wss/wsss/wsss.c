/**
 ****************************************************************************************
 *
 * @file wsss.c
 *
 * @brief Weight Scale Service Server Implementation.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
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
 * @addtogroup wsss
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
    #if (BLE_WSS_SERVER)
    #include "attm_util.h"
    #include "atts_util.h"
    #include "prf_utils.h"
    #include "wsss.h"
    #include "wsss_task.h"

    /*
     * WSS ATTRIBUTES
     ****************************************************************************************
     */

    /// Full WSS Database Description - Used to add attributes into the database
    const struct attm_desc wsss_att_db[WSS_IDX_NB] =
    {
        // Weight Scale Service Declaration
        [WSS_IDX_SVC]             =
        {
            ATT_DECL_PRIMARY_SERVICE,
            PERM(RD, ENABLE),
            sizeof(wss_svc),
            sizeof(wss_svc),
            (uint8_t*) &wss_svc
        },
        // WSS Included Service Declaration
        [WSS_IDX_INCL_SVC]        =
        {
            ATT_DECL_INCLUDE,
            PERM(RD, ENABLE),
            sizeof(struct att_incl_desc), 0,
            NULL
        },

        // Weight Scale Feature Characteristic Declaration
        [WSS_IDX_FEATURE_CHAR]    = 
        {
            ATT_DECL_CHARACTERISTIC,
            PERM(RD, ENABLE),
            sizeof(wsss_feature_char),
            sizeof(wsss_feature_char),
            (uint8_t*) &wsss_feature_char
        },
        
        // Weight Scale Feature Characteristic Value
        [WSS_IDX_FEATURE_VAL]     =
        {
            ATT_CHAR_WS_FEAT,
            PERM(RD, ENABLE),
            sizeof(struct wss_feature),
            0,
            NULL
        },

        // Weight Measurement Characteristic Declaration
        [WSS_IDX_WT_MEAS_CHAR]    =
        {
            ATT_DECL_CHARACTERISTIC,
            PERM(RD, ENABLE),
            sizeof(wsss_measurement_char),
            sizeof(wsss_measurement_char),
            (uint8_t*) &wsss_measurement_char
        },
        
        // Weight Measurement Characteristic Value
        [WSS_IDX_WT_MEAS_VAL]     =
        {
            ATT_CHAR_WS_MEAS,
            PERM(IND, ENABLE),
            WSS_MEAS_VAL_MAX_LEN,
            0,
            NULL
        },

        // Weight Measurement Characteristic - Client Characteristic Configuration Descriptor
        [WSS_IDX_WT_MEAS_IND_CFG] =
        {
            ATT_DESC_CLIENT_CHAR_CFG,
            PERM(RD, ENABLE)|PERM(WR, ENABLE),
            sizeof(uint16_t),
            0,
            NULL
        },
    };

    const att_svc_desc_t wss_svc = ATT_SVC_WEIGHT_SCALE;

    const struct att_char_desc wsss_feature_char     = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                WSS_FEATURE_CHAR,
                                                                ATT_CHAR_WS_FEAT);

    const struct att_char_desc wsss_measurement_char = ATT_CHAR(ATT_CHAR_PROP_IND,
                                                                WSS_MEASUREMENT_CHAR,
                                                                ATT_CHAR_WS_MEAS);

    /*
     * GLOBAL VARIABLE DEFINITIONS
     ****************************************************************************************
     */

    struct wsss_env_tag wsss_env __attribute__((section("retention_mem_area0"), zero_init)); //@RETENTION MEMORY

    /// Device Information Service task descriptor
    static const struct ke_task_desc TASK_DESC_WSSS =
    {
        wsss_state_handler,
        &wsss_default_handler,
        wsss_state,
        WSSS_STATE_MAX,
        WSSS_IDX_MAX
    };


    /*
     * FUNCTION DEFINITIONS
     ****************************************************************************************
     */

    void wsss_init(void)
    {
        // Reset environment
        memset(&wsss_env, 0, sizeof(wsss_env));

        // Create wsss task
        ke_task_create(TASK_WSSS, &TASK_DESC_WSSS);

        // Set task in disabled state
        ke_state_set(TASK_WSSS, WSSS_DISABLED);
    }

    void wsss_pack_meas_value(uint8_t* packed_meas,
                              const struct wss_wt_meas* pmeas_val)
    {
        // Clear packet data
        memset(packed_meas, 0, WSS_MEAS_VAL_MAX_LEN);
        
        // Weight measurement flags
        *packed_meas++ = pmeas_val->flags;
        
        // Weight measurement
        if (pmeas_val->weight < 0)
            co_write16p(packed_meas, 0xFFFF);
        else
            co_write16p(packed_meas, pmeas_val->flags & WSS_MEAS_FLAG_UNIT_IMPERIAL
                                    ? pmeas_val->weight / 0.01
                                    : pmeas_val->weight / 0.005);
        packed_meas += sizeof(uint16_t);
        
        if (pmeas_val->flags & WSS_MEAS_FLAG_TIME_STAMP)
            // Time Stamp
            packed_meas += prf_pack_date_time(packed_meas, &pmeas_val->datetime);
        
        if (pmeas_val->flags & WSS_MEAS_FLAG_USERID_PRESENT)
            // User ID
            *packed_meas++ = pmeas_val->userid;

        if ((pmeas_val->flags & WSS_MEAS_FLAG_BMI_HT_PRESENT) && (pmeas_val->weight >= 0))
        {   // BMI
            co_write16p(packed_meas, pmeas_val->bmi / 0.1);
            packed_meas += sizeof(uint16_t);
            
            // Height
            co_write16p(packed_meas, pmeas_val->flags & WSS_MEAS_FLAG_UNIT_IMPERIAL
                                   ? pmeas_val->height / 0.1
                                   : pmeas_val->height / 0.001);
            packed_meas += sizeof(uint16_t);
        }
    }

    void wsss_meas_send_cfm_send(uint8_t status)
    {
        // Send CFM to APP that value has been sent or not
        struct wsss_meas_send_cfm *cfm = KE_MSG_ALLOC(WSSS_MEAS_SEND_CFM,
                                                      wsss_env.con_info.appid,
                                                      TASK_WSSS,
                                                      wsss_meas_send_cfm);

        cfm->conhdl = gapc_get_conhdl(wsss_env.con_info.conidx);
        cfm->status = status;

        ke_msg_send(cfm);
    }

    uint8_t wsss_calc_meas_value_size(uint8_t flags)
    {
        // Flags & Weight always present
        uint8_t size = sizeof(int8_t) + sizeof(uint16_t);
        
        if (flags & WSS_MEAS_FLAG_TIME_STAMP)
        {   // Time Stamp present
            struct prf_date_time datetime;
            size += sizeof(datetime.year) + sizeof(datetime.month) + sizeof(datetime.day)
                  + sizeof(datetime.hour) + sizeof(datetime.min) + sizeof(datetime.sec);
        }
        
        if (flags & WSS_MEAS_FLAG_USERID_PRESENT)
            // User ID present
            size += sizeof(uint8_t);

        if (flags & WSS_MEAS_FLAG_BMI_HT_PRESENT)
            // BMI & Height present
            size += sizeof(uint16_t) * 2;

        return size;
    }

    uint32_t wsss_compute_cfg_flag(uint32_t features)
    {
        //Service Declaration
        uint32_t cfg_flag = 1;

        for (uint8_t i = 0; i<WSS_CHAR_MAX; i++)
            if ((features >> i) & 1)
                cfg_flag |= (3 << (i*2 + 1));

        return cfg_flag;
    }

    uint8_t wsss_check_val_len(uint8_t char_code, uint8_t val_len)
    {
        uint8_t status = PRF_ERR_OK;

        // Check if length is upper than the general maximal length
        if (val_len > WSS_MEAS_VAL_MAX_LEN)
            status = PRF_ERR_UNEXPECTED_LEN;

        return status;
    }

    void wsss_disable(uint16_t conhdl) 
    {
        // Inform the application about the disconnection
        struct wsss_disable_ind *ind = KE_MSG_ALLOC(WSSS_DISABLE_IND,
                                                    wsss_env.con_info.appid,
                                                    TASK_WSSS,
                                                    wsss_disable_ind);

        ind->conhdl = conhdl;

        ke_msg_send(ind);

        //Disable DIS in database
        attmdb_svc_set_permission(wsss_env.shdl, PERM(SVC, DISABLE));

        //Go to idle state
        ke_state_set(TASK_WSSS, WSSS_IDLE);
    }
#endif //BLE_WSS_SERVER
/// @} WSSS
