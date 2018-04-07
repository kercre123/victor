/**
 ****************************************************************************************
 *
 * @file prf_utils.c
 *
 * @brief Implementation of Profile Utilities
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup PRF_UTILS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_SERVER_PRF || BLE_CLIENT_PRF)

#include <stdint.h>
#include <stdbool.h>
#include "ke_task.h"
#include "co_error.h"
#include "attm_util.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "gap.h"
#include "gapc.h"
#include "app_prf_types.h"

#if (BLE_BM_SERVER)
#include "bmss.h"
#include "bmss_task.h"
#endif

#if (BLE_BM_CLIENT)
#include "bmsc.h"
#include "bmsc_task.h"
#endif

#if (BLE_CUSTOM_SERVER)
#include "user_custs_config.h"
#endif // (BLE_CUSTOM_SERVER)

#if (BLE_ACCEL)
#include "accel.h"
#include "accel_task.h"
#endif // (BLE_ACCEL)

#if (BLE_HT_THERMOM)
#include "htpt.h"
#include "htpt_task.h"
#endif // (BLE_HT_THERMOM)

#if (BLE_HT_COLLECTOR)
#include "htpc.h"
#include "htpc_task.h"
#endif // (BLE_HT_COLLECTOR)

#if (BLE_DIS_SERVER)
#include "diss.h"
#include "diss_task.h"
#endif // (BLE_HT_THERMOM)

#if (BLE_DIS_CLIENT)
#include "disc.h"
#include "disc_task.h"
#endif // (BLE_DIS_CLIENT)

#if (BLE_BP_SENSOR)
#include "blps.h"
#include "blps_task.h"
#endif // (BLE_BP_SENSOR)

#if (BLE_BP_COLLECTOR)
#include "blpc.h"
#include "blpc_task.h"
#endif // (BLE_BP_COLLECTOR)

#if (BLE_TIP_SERVER)
#include "tips.h"
#include "tips_task.h"
#endif // (BLE_TIP_SERVER)

#if (BLE_TIP_CLIENT)
#include "tipc.h"
#include "tipc_task.h"
#endif // (BLE_TIP_CLIENT)

#if (BLE_HR_SENSOR)
#include "hrps.h"
#include "hrps_task.h"
#endif // (BLE_HR_SENSOR)

#if (BLE_HR_COLLECTOR)
#include "hrpc.h"
#include "hrpc_task.h"
#endif // (BLE_HR_COLLECTOR)

#if (BLE_FINDME_LOCATOR)
#include "findl.h"
#include "findl_task.h"
#endif // (BLE_FINDME_LOCATOR)

#if (BLE_FINDME_TARGET)
#include "findt.h"
#include "findt_task.h"
#endif // (BLE_FINDME_TARGET)

#if (BLE_PROX_MONITOR)
#include "proxm.h"
#include "proxm_task.h"
#endif // (BLE_PROX_MONITOR)

#if (BLE_PROX_REPORTER)
#include "proxr.h"
#include "proxr_task.h"
#endif // (BLE_PROX_REPORTER)

#if (BLE_STREAMDATA_HOST)
#include "streamdatah.h"
#include "streamdatah_task.h"
#endif // (BLE_STREAMDATA_HOST)
#if (BLE_STREAMDATA_DEVICE)
#include "streamdatad.h"
#include "streamdatad_task.h"
#endif // (BLE_STREAMDATA_DEVICE)
#if (BLE_SPOTA_RECEIVER)
#include "spotar.h"
#include "spotar_task.h"
#endif // (BLE_SPOTA_RECEIVER)
#if (BLE_SPS_SERVER)
#include "sps_server.h"
#include "sps_server_task.h"
#endif // (BLE_SPS_SERVER)
#if (BLE_SPS_CLIENT)
#include "sps_client.h"
#include "sps_client_task.h"
#endif // (BLE_SPS_SERVER)
#if (BLE_SP_CLIENT)
#include "scppc.h"
#include "scppc_task.h"
#endif // (BLE_SP_CLENT)

#if (BLE_SP_SERVER)
#include "scpps.h"
#include "scpps_task.h"
#endif // (BLE_SP_SERVER)

#if (BLE_BAS_CLIENT)
#include "basc.h"
#include "basc_task.h"
#endif // (BLE_BAS_CLIENT)

#if (BLE_BAS_SERVER)
#include "bass.h"
#include "bass_task.h"
#endif // (BLE_BAS_SERVER)

#if (BLE_HID_DEVICE)
#include "hogpd.h"
#include "hogpd_task.h"
#endif // (BLE_HID_DEVICE)

#if (BLE_HID_BOOT_HOST)
#include "hogpbh.h"
#include "hogpbh_task.h"
#endif // (BLE_HID_BOOT_HOST)

#if (BLE_HID_REPORT_HOST)
#include "hogprh.h"
#include "hogprh_task.h"
#endif // (BLE_HID_REPORT_HOST)

#if (BLE_GL_COLLECTOR)
#include "glpc.h"
#include "glpc_task.h"
#endif // (BLE_GL_COLLECTOR)

#if (BLE_GL_SENSOR)
#include "glps.h"
#include "glps_task.h"
#endif // (BLE_GL_SENSOR)

#if (BLE_NEB_CLIENT)
#include "nbpc.h"
#include "nbpc_task.h"
#endif // (BLE_NEB_CLIENT)

#if (BLE_NEB_SERVER)
#include "nbps.h"
#include "nbps_task.h"
#endif // (BLE_NEB_SERVER)

#if (BLE_RSC_COLLECTOR)
#include "rscpc.h"
#include "rscpc_task.h"
#endif // (BLE_RSC_COLLECTOR)

#if (BLE_RSC_SENSOR)
#include "rscps.h"
#include "rscps_task.h"
#endif // (BLE_RSC_COLLECTOR)

#if (BLE_CSC_COLLECTOR)
#include "cscpc.h"
#include "cscpc_task.h"
#endif // (BLE_CSC_COLLECTOR)

#if (BLE_CSC_SENSOR)
#include "cscps.h"
#include "cscps_task.h"
#endif // (BLE_CSC_COLLECTOR)

#if (BLE_AN_CLIENT)
#include "anpc.h"
#include "anpc_task.h"
#endif // (BLE_AN_CLIENT)

#if (BLE_AN_SERVER)
#include "anps.h"
#include "anps_task.h"
#endif // (BLE_AN_SERVER)

#if (BLE_ANC_CLIENT)
#include "ancc.h"
#include "ancc_task.h"
#endif // (BLE_ANC_CLIENT)

#if (BLE_PAS_CLIENT)
#include "paspc.h"
#include "paspc_task.h"
#endif // (BLE_PAS_CLIENT)

#if (BLE_PAS_SERVER)
#include "pasps.h"
#include "pasps_task.h"
#endif // (BLE_PAS_SERVER)

#if (BLE_CP_COLLECTOR)
#include "cppc.h"
#include "cppc_task.h"
#endif //(BLE_CP_COLLECTOR)

#if (BLE_CP_SENSOR)
#include "cpps.h"
#include "cpps_task.h"
#endif //(BLE_CP_SENSOR)

#if (BLE_LN_COLLECTOR)
#include "lanc.h"
#include "lanc_task.h"
#endif //(BLE_CP_COLLECTOR)

#if (BLE_LN_SENSOR)
#include "lans.h"
#include "lans_task.h"
#endif //(BLE_CP_SENSOR)

#if (BLE_SAMPLE128)
#include "sample128.h"
#include "sample128_task.h"
#endif // (BLE_SAMPLE128)

#if (BLE_WPT_CLIENT)
#include "wptc.h"
#include "wptc_task.h"
#endif // (BLE_WPTC)

#if (BLE_WPTS)
#include "wpts.h"
#include "wpts_task.h"
#endif // (BLE_WPTS)

#if (BLE_IEU)  
#include "ieu.h"
#include "ieu_task.h"
#endif // (BLE_IEU)

#if (BLE_MPU)   
#include "mpu.h"
#include "mpu_task.h"
#endif // (BLE_MPU)

#if (BLE_UDS_SERVER)
#include "udss.h"
#include "udss_task.h"
#endif // (BLE_UDS_SERVER)

#if (BLE_UDS_CLIENT)
#include "udsc.h"
#include "udsc_task.h"
#endif // (BLE_UDS_CLIENT)

#if (BLE_WSS_SERVER)
#include "wsss.h"
#include "wsss_task.h"
#endif // (BLE_WSS_SERVER)

#if (BLE_WSS_COLLECTOR)
#include "wssc.h"
#include "wssc_task.h"
#endif // (BLE_WSS_COLLECTOR)

#if (BLE_ADC_NOTIFY)
#include "adc_notify.h"
#include "adc_notify_task.h"
#endif // (BLE_ADC_NOTIFY)

#if (BLE_DEVICE_CONFIG)
#include "device_config.h"
#include "device_config_task.h"
#endif // (DEVICE_CONFIG)

#if (BLE_BCS_SERVER)
#include "bcss.h"
#include "bcss_task.h"
#endif // (BLE_BCS_SERVER)

#if (BLE_BCS_CLIENT)
#include "bcsc.h"
#include "bcsc_task.h"
#endif // (BLE_BCS_SERVER)

#if (BLE_CTS_SERVER)
#include "ctss.h"
#include "ctss_task.h"
#endif // (BLE_CTS_SERVER)

#if (BLE_CTS_CLIENT)
#include "ctsc.h"
#include "ctsc_task.h"
#endif // (BLE_CTS_CLIENT)

#endif /* (BLE_SERVER_PRF || BLE_CLIENT_PRF) */

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

/**
 ****************************************************************************************
 * @brief This function is used to allocate a memory bloc containing a pool of pointers.
 * (Size = 4 *  jump_table_struct[nb_links_user])
 * These pointers are initialize to NULL.
 *
 * @param[in/out] ***p_envs     Pointer to a pool of pointer. It shall be a null pointer.
 *                              Its value will contain the address of the pool.
 *
 * @return  PRF_ERR_OK              If the allocation has been successfully done.
 *          PRF_ERR_REQ_DISALLOWED  If the allocation has failed because of a lack of memory.
 *          PRF_ERR_INVALID_PARAM   If the provided pointer is non NULL
 ****************************************************************************************
 */
static uint8_t prf_client_pool_envs_alloc(prf_env_struct ***p_envs);

/**
 ****************************************************************************************
 * @brief This function is used to free the memory bloc containing a pool of pointers.
 *
 * @param[in/out] ***p_envs     Pointer to a pool of pointer. It shall be a non-null pointer.
 ****************************************************************************************
 */
static void prf_client_pool_envs_free(prf_env_struct ***p_envs);

/**
 ****************************************************************************************
 * @brief This function is used to allocate enough memory to contain the environment of a
 * profile client role.
 *
 * @param[in/out] ***p_envs   Pointer to a pool of pointer. If its value is NULL, a pool will
 * be created.
 * @param[in]     conidx         Index of the connection for which the environment is created
 * @param[in]     env_size    Size of the environment structure.
 *
 * @return  PRF_ERR_OK              If the allocation has been successfully done.
 *          PRF_ERR_REQ_DISALLOWED  If the allocation has failed because of a lack of memory.
 *          PRF_ERR_INVALID_PARAM   Either the index is not valid or the profile client role
 *                                  is already enabled for this index.
 ****************************************************************************************
 */
static uint8_t prf_client_env_alloc(prf_env_struct ***p_envs, uint8_t conidx, uint16_t env_size);

/**
 ****************************************************************************************
 * @brief This function is used to free the memory bloc containing a profile client role task
 *        environment.
 *
 * @param[in/out] ***p_envs     Pointer to a pool of pointer. It shall be a non-null pointer.
 * @param[in]     conidx           Index of the connection for which the environment is freed.
 * @param[in]     cleanup       Clean-up function used to clean environment variables
 *                              allocated by profile.
 ****************************************************************************************
 */
static uint8_t prf_client_env_free(prf_env_struct ***p_envs, uint8_t conidx, prf_cleanup_cb cleanup);

#endif // #if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_BAS_SERVER)
void prf_pack_char_pres_fmt(uint8_t *packed_val, const struct prf_char_pres_fmt* char_pres_fmt)
{
    *packed_val =               char_pres_fmt->format;
    *(packed_val + 1) =         char_pres_fmt->exponent;
    co_write16p(packed_val + 2, char_pres_fmt->unit);
    *(packed_val + 4) =         char_pres_fmt->name_space;
    co_write16p(packed_val + 5, char_pres_fmt->description);
}
#endif // (BLE_BAS_SERVER)

#if (BLE_BAS_CLIENT)
void prf_unpack_char_pres_fmt(const uint8_t *packed_val, struct prf_char_pres_fmt* char_pres_fmt)
{

    char_pres_fmt->format       = *packed_val;
    char_pres_fmt->exponent     = *(packed_val + 1);
    char_pres_fmt->unit         = co_read16p(packed_val + 2);
    char_pres_fmt->name_space    = *(packed_val + 4);
    char_pres_fmt->description  = co_read16p(packed_val + 5);
}
#endif // (BLE_BAS_CLIENT)

#if (BLE_CLIENT_PRF)

void prf_read_char_send(struct prf_con_info* con_info,
                        uint16_t shdl, uint16_t ehdl, uint16_t valhdl)
{
    struct gattc_read_cmd * req  = KE_MSG_ALLOC(GATTC_READ_CMD, KE_BUILD_ID(TASK_GATTC, con_info->conidx),
            con_info->prf_id, gattc_read_cmd);
    //request type
    req->req_type                       = GATTC_READ;
    req->nb                             = 1;
    req->req.simple.offset              = 0;
    req->req.simple.length              = 0;
    req->req.simple.handle              = valhdl;

    //send request to GATT
    ke_msg_send(req);
}

void prf_register_atthdl2gatt(struct prf_con_info* con_info, struct prf_svc *svc)
{
    if(svc->shdl != ATT_INVALID_HANDLE)
    {
        //register profile task in gatt for indication/notifications
        struct gattc_reg_to_peer_evt_cmd * reg = KE_MSG_ALLOC(GATTC_REG_TO_PEER_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
                gattc_reg_to_peer_evt_cmd);

        reg->req_type =  GATTC_REGISTER;
        reg->start_hdl = svc->shdl;
        reg->end_hdl =   svc->ehdl;

        ke_msg_send(reg);
    }
}

void prf_unregister_atthdl2gatt(struct prf_con_info* con_info, struct prf_svc *svc)
{
    if(svc->shdl != ATT_INVALID_HANDLE)
    {
        //un register profile task in gatt for indication/notifications
        struct gattc_reg_to_peer_evt_cmd * reg = KE_MSG_ALLOC(GATTC_REG_TO_PEER_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
                gattc_reg_to_peer_evt_cmd);

        reg->req_type =  GATTC_UNREGISTER;
        reg->start_hdl = svc->shdl;
        reg->end_hdl =   svc->ehdl;

        ke_msg_send(reg);
    }
}

void prf_disc_svc_send(struct prf_con_info* con_info, uint16_t uuid)
{
    //send GATT discover primary services by UUID request: find by type request
    struct gattc_disc_cmd * svc_req = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD,
            KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
            gattc_disc_cmd, ATT_UUID_16_LEN);

    //gatt request type: by UUID
    svc_req->req_type         = GATTC_DISC_BY_UUID_SVC;
    //start handle;
    svc_req->start_hdl        = ATT_1ST_REQ_START_HDL;
    //end handle
    svc_req->end_hdl          = ATT_1ST_REQ_END_HDL;

    // UUID search
    svc_req->uuid_len = ATT_UUID_16_LEN;

    //set the first two bytes to the value array, LSB to MSB:Health Thermometer Service UUID first
    co_write16p(&(svc_req->uuid[0]), uuid);

    //send the message to GATT, which will send back the response when it gets it
    ke_msg_send(svc_req);
}

void prf_disc_incl_svc_send(struct prf_con_info* con_info, struct prf_svc *svc)
{
    // Send GATT discover included services
    struct gattc_disc_cmd * svc_req = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD,
            KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
            gattc_disc_cmd, ATT_UUID_16_LEN);

    //gatt request type: by UUID
    svc_req->req_type         = GATTC_DISC_INCLUDED_SVC;
    //start handle;
    svc_req->start_hdl        = svc->shdl;
    //end handle
    svc_req->end_hdl          = svc->ehdl;

    // UUID info - Don't care
    svc_req->uuid_len = ATT_UUID_16_LEN;
    //set the first two bytes to the value array, LSB to MSB:Health Thermometer Service UUID first
    co_write16p(&(svc_req->uuid[0]), ATT_INVALID_UUID);

    // Send the message to GATT, which will send back the response when it gets it
    ke_msg_send(svc_req);
}

void prf_disc_char_all_send(struct prf_con_info *con_info,
                            struct prf_svc *svc)
{
    struct gattc_disc_cmd * char_req = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD,
            KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
            gattc_disc_cmd, ATT_UUID_16_LEN);

   char_req->req_type  = GATTC_DISC_ALL_CHAR;
   char_req->start_hdl = svc->shdl;
   char_req->end_hdl   = svc->ehdl;

   // UUID info - Don't care
   char_req->uuid_len = ATT_UUID_16_LEN;
   //set the first two bytes to the value array, LSB to MSB:Health Thermometer Service UUID first
   co_write16p(&(char_req->uuid[0]), ATT_INVALID_UUID);

   //send the message to GATT, which will send back the response when it gets it
   ke_msg_send(char_req);
}

void prf_disc_char_desc_send(struct prf_con_info* con_info, struct prf_char_inf * charact)
{
    // Check if there is at least one descriptor to find
    if ((charact->val_hdl + 1) <= (charact->char_hdl + charact->char_ehdl_off - 1))
    {

        // Ask for handles of a certain uuid
        struct gattc_disc_cmd * dsc_req = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD,
                KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
                gattc_disc_cmd, ATT_UUID_16_LEN);

        dsc_req->req_type  = GATTC_DISC_DESC_CHAR;
        dsc_req->start_hdl = charact->val_hdl + 1;
        dsc_req->end_hdl   = charact->char_hdl + charact->char_ehdl_off - 1;

        // UUID info - Don't care
        dsc_req->uuid_len = ATT_UUID_16_LEN;
        //set the first two bytes to the value array, LSB to MSB:Health Thermometer Service UUID first
        co_write16p(&(dsc_req->uuid[0]), ATT_INVALID_UUID);

        //send the message to GATT;there should be only one handle response every time
        ke_msg_send(dsc_req);
    }
    else
    {
        
        // Send a GATT complete event message
        struct gattc_cmp_evt *evt = KE_MSG_ALLOC(GATTC_CMP_EVT, con_info->prf_id,
                                                 KE_BUILD_ID(TASK_GATTC , con_info->conidx),
                                                 gattc_cmp_evt);

        evt->status = PRF_ERR_OK;

        // Send the message
        ke_msg_send(evt);
    }
}

void prf_gatt_write(struct prf_con_info* con_info,
                    uint16_t handle, uint8_t* value, uint16_t length, uint8_t req_type)
{
    struct gattc_write_cmd *wr_char = KE_MSG_ALLOC_DYN(GATTC_WRITE_CMD,
            KE_BUILD_ID(TASK_GATTC, con_info->conidx), con_info->prf_id,
            gattc_write_cmd, length);

    // Offset
    wr_char->offset         = 0x0000;
    // cursor always 0
    wr_char->cursor         = 0x0000;
    // Write Type
    wr_char->req_type       = req_type;
    // Characteristic Value attribute handle
    wr_char->handle         = handle;
    // Value Length
    wr_char->length         = length;
    // Auto Execute
    wr_char->auto_execute   = true;
    // Value
    memcpy(&wr_char->value[0], value, length);

    // Send the message
    ke_msg_send(wr_char);
}

void prf_gatt_write_ntf_ind(struct prf_con_info* con_info, uint16_t handle, uint16_t ntf_ind_cfg)
{
    uint8_t value[2];

    // put value in air format
    co_write16p((&value[0]), ntf_ind_cfg);
    // write value over GATT
    prf_gatt_write(con_info, handle, value, 2, GATTC_WRITE);
}

uint8_t prf_check_svc_char_validity(uint8_t nb_chars,
                                    const struct prf_char_inf* chars,
                                    const struct prf_char_def* chars_req)
{
    uint8_t status = PRF_ERR_OK;
    uint8_t i;

    for(i = 0; ((i < nb_chars) && (status == PRF_ERR_OK)); i++)
    {
        if (chars[i].char_hdl == ATT_INVALID_HANDLE)
        {
            //If Characteristic is not present, check requirements
            if (chars_req[i].req_flag == ATT_MANDATORY)
            {
                status = PRF_ERR_STOP_DISC_CHAR_MISSING;
            }
        }
        else
        {
            //If Characteristic is present, check properties
            if((chars[i].prop & chars_req[i].prop_mand) != chars_req[i].prop_mand)
            {
                status = PRF_ERR_STOP_DISC_WRONG_CHAR_PROP;
            }
        }
    }

    return status;
}


uint8_t prf_check_svc_char_desc_validity(uint8_t descs_size,
                                         const struct prf_char_desc_inf* descs,
                                         const struct prf_char_desc_def* descs_req,
                                         const struct prf_char_inf* chars)
{
    uint8_t status = PRF_ERR_OK;
    uint8_t i;

    for(i = 0; ((i < descs_size) && (status == PRF_ERR_OK)) ; i++)
    {
        if (descs[i].desc_hdl == ATT_INVALID_HANDLE)
        {
            //If Descriptor is missing, check if it is mandatory
            if (descs_req[i].req_flag == ATT_MANDATORY)
            {
                //Check if Char is present
                if (chars[descs_req[i].char_code].char_hdl != ATT_INVALID_HANDLE)
                {
                    //Char. is present and descriptor not, error
                    status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                }
            }
        }
    }

    return status;
}

void prf_search_chars(uint16_t svc_ehdl, uint8_t nb_chars,
                      struct prf_char_inf* chars, const struct prf_char_def* chars_req,
                      const struct gattc_disc_char_ind* param,
                      uint8_t* last_found_char)
{
    //Counters
    uint8_t cursor;

    //Look over requested characteristics
    for (cursor=0; cursor<nb_chars ; cursor++)
    {
        if(attm_uuid_match_16((uint8_t*)param->uuid, param->uuid_len, chars_req[cursor].uuid))
        {
            //Save properties and handles
            chars[cursor].char_hdl       = param->attr_hdl;
            chars[cursor].val_hdl        = param->pointer_hdl;
            chars[cursor].prop           = param->prop;

            //Compute number of attribute in Char. using SVC edhl - Limited to 255
            chars[cursor].char_ehdl_off    = (uint8_t)(svc_ehdl - chars[cursor].char_hdl + 1);

            if (cursor != *last_found_char)
            {
                //Update number of attributes for the last found char.
                chars[*last_found_char].char_ehdl_off =
                        (uint8_t)(chars[cursor].char_hdl - chars[*last_found_char].char_hdl);
            }

            //Save code of the Char. which has been found.
            *last_found_char = cursor;
        }

    }
}

void prf_search_descs(uint8_t nb_descs,
                      struct prf_char_desc_inf* descs, const struct prf_char_desc_def* descs_req,
                      const struct gattc_disc_char_desc_ind* param,
                      uint8_t last_char_code)
{
    //Counters
    uint8_t cursor;

    //Retrieve characteristic descriptor handle using UUID
    for(cursor = 0; cursor<nb_descs; cursor++)
    {
        if ((last_char_code == descs_req[cursor].char_code)
                && (attm_uuid_match_16((uint8_t*)param->uuid, param->uuid_len, descs_req[cursor].uuid))
                && (descs[cursor].desc_hdl == ATT_INVALID_HANDLE))
        {
            descs[cursor].desc_hdl = param->attr_hdl;
        }
    }
}

void prf_client_disable_ind_send(prf_env_struct ***p_envs, ke_msg_id_t msg_id,
                                 ke_task_id_t task_id, uint8_t state, uint16_t conhdl,
                                 prf_cleanup_cb cleanup)
{
    // retrieve current state
    uint8_t cur_state = ke_state_get(task_id);

    // prevent doing freeing structure if state already of profile is already free.
    if(cur_state != state)
    {
        // Get the address of the environment
        prf_env_struct *env = prf_client_get_env(*p_envs, task_id);

        ASSERT_WARN(env != NULL);

        if (env != NULL)
        {
            struct prf_client_disable_ind *ind = KE_MSG_ALLOC(msg_id,
                                                              env->con_info.appid, env->con_info.prf_id,
                                                              prf_client_disable_ind);

            ind->conhdl    = conhdl;
            ind->status    = prf_client_disable(p_envs, KE_IDX_GET(env->con_info.prf_id), cleanup);

            // Send the message
            ke_msg_send(ind);

            // Go to idle state
            ke_state_set(task_id, state);
        }
    }
}

#endif //(BLE_CLIENT_PRF)

#if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

static uint8_t prf_client_pool_envs_alloc(prf_env_struct ***p_envs)
{
    // Allocation status
    uint8_t status = PRF_ERR_OK;

    // Check if provided pointer is NULL
    if (*p_envs == NULL)
    {
        // Allocates buffer size for  jump_table_struct[nb_links_user] pointers
        *p_envs = (prf_env_struct **)ke_malloc( jump_table_struct[nb_links_user]*sizeof(prf_env_struct *), KE_MEM_ATT_DB);

        // Check if a non-NULL pointer has been returned
        if(*p_envs == NULL)
        {
            // Not enough memory to allocate the data structure.
            status = PRF_ERR_REQ_DISALLOWED;
        }
        else
        {
            // Initialize all pointers for the pool to NULL
            memset(*p_envs, 0,  jump_table_struct[nb_links_user]*sizeof(prf_env_struct *));
        }
    }
    else
    {
        // envs should be a NULL pointer
        status = PRF_ERR_REQ_DISALLOWED;
    }

    return status;
}

static void prf_client_pool_envs_free(prf_env_struct ***p_envs)
{
    // Check if the pool of environments exists
    if (*p_envs != NULL)
    {
        // No environment exists, free the array of environment pointers
        ke_free(*p_envs);
        // Reset the address of the pool
        *p_envs = NULL;
    }
}

static uint8_t prf_client_env_alloc(prf_env_struct ***p_envs, uint8_t conidx, uint16_t env_size)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Pointer to the environment
    prf_env_struct *env;

    /*
     * Check if the pool of environment already exists. This pool exists only if it contains
     * the address of at least one environment.
     */
    if (*p_envs == NULL)
    {
        // Allocate the pool which contains the address of each profile client role environment.
        status = prf_client_pool_envs_alloc(p_envs);
    }

    // If the pool exists, allocate a new environment
    if (status == PRF_ERR_OK)
    {
        // Check index
        if (conidx <  jump_table_struct[nb_links_user])
        {
            // Get the current address of the environment
            env = *(*p_envs + conidx);

            // Check if the environment matching this index already exists.
            if (env == NULL)
            {
                // Allocate a new environment.
                env = (prf_env_struct *)ke_malloc((uint32_t)env_size, KE_MEM_ATT_DB);

                // Check if the memory for the environment has been successfully allocated.
                if (env == NULL)
                {
                    // Not enough memory to allocate a new environment.
                    status = PRF_ERR_REQ_DISALLOWED;
                }
                else
                {
                    // Initialize values in the profile environment
                    memset(env, 0, env_size);

                    // Save the address of the environment in the pool.
                    *(*p_envs + conidx) = env;
                }
            }
            else
            {
                // The profile client role is already enable for this index.
                status = PRF_ERR_REQ_DISALLOWED;
            }
        }
        else
        {
            // The provided connection doesn't exist.
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    return status;
}

static uint8_t prf_client_env_free(prf_env_struct ***p_envs, uint8_t conidx, prf_cleanup_cb cleanup)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Pointer to a profile environment
    prf_env_struct *p_prf_env = NULL;

    // Check that the provided pointer in non-NULL and that conidx is OK
    if ((p_envs != NULL) && (conidx <  jump_table_struct[nb_links_user]))
    {
        // Check if the pool of environments exists
        if (*p_envs != NULL)
        {
            // Get the address of the environment for the provided connection
            p_prf_env = *(*p_envs + conidx);
        }

        // Check if this environment exists
        if (p_prf_env != NULL)
        {
            // request profile to clean-up variable allocated dynamically for current connection
            if(cleanup != NULL)
            {
                cleanup(p_prf_env);
            }

            // Free the profile environment
            ke_free(p_prf_env);

            // Reset the address of the environment in the pool
            *(*p_envs + conidx) = NULL;
        }
        else
        {
            // The environment doesn't exist.
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }


    return status;
}

uint8_t prf_client_enable(prf_env_struct ***p_envs, struct prf_con_info *p_con_info,
                          void const *p_param, uint16_t env_size)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Index
    uint8_t conidx;
    // Pointer to an environment structure
    prf_env_struct *env;

    // Get the index matching this connection handle
    conidx = p_con_info->conidx;

    // Check if the connection exists
    if (conidx != GAP_INVALID_CONIDX)
    {
        // Check if we are in the good task instance
        if (conidx != KE_IDX_GET(p_con_info->prf_id))
        {
            // Forward the message to the good instance of the task
            ke_msg_forward(p_param, KE_BUILD_ID(KE_TYPE_GET(p_con_info->prf_id), conidx), p_con_info->appid);

            /*
             * Here, the status PRF_ERR_FEATURE_NOT_SUPPORTED is used to inform that the message
             * has been forwarded.
             */
            status = PRF_ERR_FEATURE_NOT_SUPPORTED;
        }
        else
        {
            // Create the environment for the provided connection.
            status = prf_client_env_alloc(p_envs, conidx, env_size);

            // Check if the environment has been successfully created.
            if (status == PRF_ERR_OK)
            {
                // Get the profile environment address
                env = *(*p_envs + conidx);

                /*
                 * Save the connection information in the environment.
                 * The first parameter of a profile client role environment is:
                 * struct prf_con_info con_info
                 */
                memcpy(&env->con_info, p_con_info, sizeof(struct prf_con_info));
            }
        }
    }
    else
    {
        // The connection doesn't exists
        status = PRF_ERR_REQ_DISALLOWED;
    }

    return status;
}

uint8_t prf_client_disable(prf_env_struct ***p_envs, uint8_t idx, prf_cleanup_cb cleanup)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;
    // Counter
    uint8_t prf_id;
    // Pointer to a profile environment
    prf_env_struct *p_prf_env;

    // Check that the provided pointer in non-NULL and that idx is OK
    if ((p_envs != NULL) && (idx <  jump_table_struct[nb_links_user]))
    {
        // If the pool of environment doesn't exist, do nothing
        if (*p_envs != NULL)
        {
            status = prf_client_env_free(p_envs, idx, cleanup);

            /*
             * If the removed environment was the last existing one in the environment pool, remove also
             * the pool.
             */
            if (status == PRF_ERR_OK)
            {
                // Reset the pointer to the environment.
                p_prf_env = NULL;

                // Check if at least one environment exists for the profile client role
                for (prf_id = 0; ((prf_id <  jump_table_struct[nb_links_user]) && (p_prf_env == NULL)); prf_id++)
                {
                    p_prf_env = *(*p_envs + prf_id);
                }

                if (p_prf_env == NULL)
                {
                    prf_client_pool_envs_free(p_envs);
                }
            }
        }
    }

    return status;
}

void prf_client_reset(prf_env_struct ***p_envs, ke_task_id_t task_type,
        ke_state_t state, struct ke_task_desc const * p_task_desc,
        prf_cleanup_cb cleanup)
{
    // Index
    uint8_t conidx;

    // Check if the provided pool of environments exists
    if (*p_envs != NULL)
    {
        // Remove all existing environments
        for (conidx = 0; conidx <  jump_table_struct[nb_links_user]; conidx++)
        {
            prf_client_env_free(p_envs, conidx, cleanup);
        }

        // Remove the pool of environments
        prf_client_pool_envs_free(p_envs);
    }

    // Create task
    ke_task_create(task_type, p_task_desc);

    // Set the state to IDLE for each task instance
    for (conidx = 0; conidx <  jump_table_struct[nb_links_user]; conidx++)
    {
        // Go to IDLE state
        ke_state_set(KE_BUILD_ID(task_type, conidx), state);
    }
}

prf_env_struct *prf_client_get_env(prf_env_struct **p_envs, ke_task_id_t task_id)
{
    // Address of the environement
    prf_env_struct *env = NULL;

    // Check if the provided pool of environments has been allocated
    if (p_envs != NULL)
{
        // Get the address stored in the pool
        env = *(p_envs + KE_IDX_GET(task_id));
    }

    // Return the address of the environment
    return env;
}

void prf_client_enable_error(prf_env_struct ***p_envs, ke_task_id_t prf_task_id,
                             ke_state_t disc_state, ke_state_t idle_state,
                             prf_cleanup_cb cleanup)
{
    /* An error has been raised, three possibilities:
     *      - Either the state is IDLE, no environment has been created, nothing to do,
     *      - Or the state is DISCOVERING, we need to come back to the IDLE state and to remove the
     * environment,
     *      - Or the state is CONNECTED, the profile was already connected for the provided connection,
     * nothing to do
     */
    if (ke_state_get(prf_task_id) == disc_state)
    {
        // Come back to IDLE State
        ke_state_set(prf_task_id, idle_state);

        // Remove the allocated environment
        prf_client_disable(p_envs, KE_IDX_GET(prf_task_id), cleanup);
    }
}

void prf_client_att_info_rsp(prf_env_struct *p_env, uint16_t msg_id,
                             uint8_t status, struct gattc_read_ind const* read_ind)
{
    // retrieve value length
    uint16_t length = 0;
    if(status == GATT_ERR_NO_ERROR)
    {
        length = read_ind->length;
    }

    // prepare response
    struct prf_att_info *rsp = KE_MSG_ALLOC_DYN(msg_id,
            p_env->con_info.appid, p_env->con_info.prf_id,
            prf_att_info, length);

    rsp->conhdl     = gapc_get_conhdl(p_env->con_info.conidx);
    rsp->status     = status;
    rsp->handle     = ATT_INVALID_HDL;
    rsp->length     = length;

    // set value array
    if (read_ind != NULL)
    {
        rsp->handle = read_ind->handle;
        memcpy(&(rsp->value[0]), &(read_ind->value[0]), length);
    }

    ke_msg_send(rsp);
}
#endif //(BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_AN_SERVER || BLE_PAS_SERVER)

#if (BLE_SERVER_PRF)

void prf_server_error_ind_send(prf_env_struct *p_env, uint8_t status,
                               ke_msg_id_t ind_msg_id, ke_msg_id_t msg_id)
{
    struct prf_server_error_ind *ind = KE_MSG_ALLOC(ind_msg_id,
                                                    p_env->con_info.appid, p_env->con_info.prf_id,
                                                    prf_server_error_ind);

    ind->conhdl    = gapc_get_conhdl(p_env->con_info.conidx);
    ind->status    = status;
    ind->msg_id    = msg_id;

    // send the message
    ke_msg_send(ind);
}


void prf_server_send_event(prf_env_struct *p_env, bool indication,
                           uint16_t handle)
{
    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *req = KE_MSG_ALLOC(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC,p_env->con_info.conidx), p_env->con_info.prf_id,
            gattc_send_evt_cmd);

    // Fill in the parameter structure
    req->req_type = ((indication) ? GATTC_INDICATE : GATTC_NOTIFY);
    req->handle = handle;

    // Send the event
    ke_msg_send(req);
}

#endif //(BLE_SERVER_PRF)

#if ((BLE_SERVER_PRF || BLE_CLIENT_PRF))

uint8_t prf_pack_date_time(uint8_t *packed_date, const struct prf_date_time* date_time)
{
    co_write16p(packed_date, date_time->year);
    *(packed_date + 2) = date_time->month;
    *(packed_date + 3) = date_time->day;
    *(packed_date + 4) = date_time->hour;
    *(packed_date + 5) = date_time->min;
    *(packed_date + 6) = date_time->sec;

    return 7;
}
uint8_t prf_unpack_date_time(uint8_t *packed_date, struct prf_date_time* date_time)
{
    date_time->year = co_read16p(&(packed_date[0]));
    date_time->month = packed_date[2];
    date_time->day   = packed_date[3];
    date_time->hour  = packed_date[4];
    date_time->min   = packed_date[5];
    date_time->sec   = packed_date[6];

    return 7;
}

/**
 ****************************************************************************************
 * @brief Fix the Characteristics Properties of the Service Changed Characteristic 
 *        according to the  Erratum 3833, described in "ESR05 - ERRATA SERVICE RELEASE 
 *        TO BLUETOOTH SPECIFICATIONS".
 * @return void
 ****************************************************************************************
 */
static void fix_service_changed_char_prop(void)
{
    // Support only "Indicate"
    uint8_t value = 0x20;
    
    attmdb_att_partial_value_update(GATT_GET_ATT_HANDLE(GATT_IDX_CHAR_SVC_CHANGED), 0, 1,
                                        (uint8_t*)&value);
    attmdb_att_set_permission(GATT_GET_ATT_HANDLE(GATT_IDX_SVC_CHANGED), PERM(IND, ENABLE));
}

void prf_init_func(void)
{
    fix_service_changed_char_prop();
    
    #if (BLE_BM_SERVER)
    bmss_init();
    #endif

    #if (BLE_BM_CLIENT)
    bmsc_init();
    #endif

    #if (BLE_ACCEL)
    accel_init();
    #endif // (BLE_ACCEL)

    #if (BLE_HT_THERMOM)
    htpt_init();
    #endif // (BLE_HT_THERMOM)

    #if (BLE_HT_COLLECTOR)
    htpc_init();
    #endif // (BLE_HT_COLLECTOR)

    #if (BLE_DIS_CLIENT)
    disc_init();
    #endif // (BLE_DIS_CLIENT)

    #if (BLE_BP_SENSOR)
    blps_init();
    #endif // (BLE_BP_SENSOR)

    #if (BLE_BP_COLLECTOR)
    blpc_init();
    #endif // (BLE_BP_COLLECTOR)

    #if (BLE_TIP_SERVER)
    tips_init();
    #endif // (BLE_TIP_SERVER)

    #if (BLE_TIP_CLIENT)
    tipc_init();
    #endif // (BLE_TIP_CLIENT)

    #if (BLE_HR_SENSOR)
    hrps_init();
    #endif // (BLE_HR_SENSOR)

    #if (BLE_HR_COLLECTOR)
    hrpc_init();
    #endif // (BLE_HR_COLLECTOR)

    #if (BLE_PROX_MONITOR)
    proxm_init();
    #endif // (BLE_PROX_MONITOR)

    #if (BLE_STREAMDATA_HOST)
    streamdatah_init();
    #endif // (BLE_STREAMDATA_HOST)
    
    #if (BLE_STREAMDATA_DEVICE)
    streamdatad_init();
    #endif // (BLE_STREAMDATA_DEVICE)

    #if (BLE_SPS_CLIENT)
    sps_client_init();
    #endif // (BLE_SPS_CLIENT)
    
    #if (BLE_SPS_SERVER)
    sps_server_init();
    #endif // (BLE_SPS_SERVER)
    
    #if (BLE_SP_SERVER)
    scpps_init();
    #endif // (BLE_SP_SERVER)

    #if (BLE_SP_CLIENT)
    scppc_init();
    #endif // (BLE_SP_CLIENT)

    #if (BLE_BAS_CLIENT)
    basc_init();
    #endif // (BLE_BAS_CLIENT)

    #if (BLE_HID_DEVICE)
    hogpd_init();
    #endif // (BLE_HID_DEVICE)

    #if (BLE_HID_BOOT_HOST)
    hogpbh_init();
    #endif // (BLE_HID_BOOT_HOST)

    #if (BLE_HID_REPORT_HOST)
    hogprh_init();
    #endif // (BLE_HID_REPORT_HOST)

    #if (BLE_GL_COLLECTOR)
    glpc_init();
    #endif // (BLE_GL_COLLECTOR)

    #if (BLE_GL_SENSOR)
    glps_init();
    #endif // (BLE_GL_SENSOR)

    #if (BLE_NEB_CLIENT)
    nbpc_init();
    #endif // (BLE_NEB_CLIENT)

    #if (BLE_NEB_SERVER)
    nbps_init();
    #endif // (BLE_NEB_SERVER)

    #if (BLE_RSC_COLLECTOR)
    rscpc_init();
    #endif // (BLE_RSC_COLLECTOR)

    #if (BLE_RSC_SENSOR)
    rscps_init();
    #endif // (BLE_RSC_SENSOR)

    #if (BLE_CSC_COLLECTOR)
    cscpc_init();
    #endif // (BLE_CSC_COLLECTOR)

    #if (BLE_CSC_SENSOR)
    cscps_init();
    #endif // (BLE_CSC_SENSOR)

    #if (BLE_CP_COLLECTOR)
    cppc_init();
    #endif // (BLE_CP_COLLECTOR)

    #if (BLE_CP_SENSOR)
    cpps_init();
    #endif // (BLE_CP_SENSOR)

    #if (BLE_LN_COLLECTOR)
    lanc_init();
    #endif // (BLE_LN_COLLECTOR)

    #if (BLE_LN_SENSOR)
    lans_init();
    #endif // (BLE_LN_SENSOR)

    #if (BLE_AN_CLIENT)
    anpc_init();
    #endif // (BLE_AN_CLIENT)

    #if (BLE_AN_SERVER)
    anps_init();
    #endif // (BLE_AN_SERVER)

    #if (BLE_ANC_CLIENT)
    ancc_init();
    #endif // (BLE_ANC_CLIENT)
    
    #if (BLE_PAS_CLIENT)
    paspc_init();
    #endif // (BLE_PAS_CLIENT)

    #if (BLE_PAS_SERVER)
    pasps_init();
    #endif // (BLE_PAS_SERVER)
    
    #if (BLE_SAMPLE128)
    sample128_init();
    #endif // (BLE_SAMPLE128)
    
    #if (BLE_WPT_CLIENT)
    wptc_init();
    #endif // WPT_CLIENT
    
    #if (BLE_WPTS)
    wpts_init();
    #endif // BLE_WPTS

    #if (BLE_IEU)  
    ieu_init();
    #endif // (BLE_IEU)

    #if (BLE_MPU)   
    mpu_init();
    #endif // (BLE_MPU)

    #if (BLE_UDS_SERVER)
    udss_init();
    #endif // (BLE_UDS_SERVER)

    #if (BLE_UDS_CLIENT)
    udsc_init();
    #endif // (BLE_UDS_CLIENT)

    #if (BLE_WSS_SERVER)
    wsss_init();
    #endif // (BLE_WSS_SERVER)
    
    #if (BLE_WSS_COLLECTOR)
    wssc_init();
    #endif // (BLE_WSS_COLLECTOR)

    #if (BLE_ADC_NOTIFY)
    adc_notify_init();
    #endif // (BLE_ADC_NOTIFY)
    
    #if (BLE_DEVICE_CONFIG)
    device_config_init();
    #endif // (BLE_DEVICE_CONFIG)
    
    #if (BLE_PROX_REPORTER)
    proxr_init();
    #endif // (BLE_PROX_REPORTER)
    
    #if (BLE_BAS_SERVER)
    bass_init();
    #endif // (BLE_BAS_SERVER)
    
    #if (BLE_SPOTA_RECEIVER)
    spotar_init();
    #endif // (BLE_SPOTA_RECEIVER)
    
    #if (BLE_DIS_SERVER)
    diss_init();
    #endif // (BLE_DIS_SERVER)
    
    #if (BLE_FINDME_LOCATOR)
    findl_init();
    #endif // (BLE_FINDME_LOCATOR)
    
    #if (BLE_FINDME_TARGET)
    findt_init();
    #endif // (BLE_FINDME_TARGET)
   
    #if (BLE_BCS_SERVER)
    bcss_init();
    #endif // (BLE_BCS_SERVER)
    
    #if (BLE_BCS_CLIENT)
    bcsc_init();
    #endif // (BLE_BCS_CLIENT)

    #if (BLE_CTS_SERVER)
    ctss_init();
    #endif // (BLE_CTS_SERVER)

    #if (BLE_CTS_CLIENT)
    ctsc_init();
    #endif // (BLE_CTS_CLIENT)

    #if (BLE_CUSTOM_SERVER)
    uint8_t i=0;
    while( cust_prf_funcs[i].task_id != TASK_NONE )
    {
        if( cust_prf_funcs[i].init_func != NULL )
        {
            cust_prf_funcs[i++].init_func();         
        }
        else i++;
    }
    #endif
}

void prf_cleanup_func(uint8_t conidx, uint16_t conhdl, uint8_t reason)
{
    #if (BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_PAS_SERVER || BLE_AN_SERVER)
    ke_task_id_t prf_task_id;
    #endif //(BLE_CLIENT_PRF || BLE_TIP_SERVER || BLE_PAS_SERVER || BLE_AN_SERVER)

    //All profiles get this event, they must disable clean
    #if (BLE_BM_SERVER)
    if (ke_state_get(TASK_BMSS) == BMSS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_BMSS);
    }
    #endif // (BLE_BM_SERVER)

    #if (BLE_BM_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_BMSC, conidx);

    if (ke_state_get(prf_task_id) != BMSC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_BM_CLIENT)

    #if (BLE_ACCEL)
    if (ke_state_get(TASK_ACCEL) == ACCEL_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_ACCEL);
    }
    #endif // (BLE_ACCEL)



    #if (BLE_PROX_MONITOR)
    prf_task_id = KE_BUILD_ID(TASK_PROXM, conidx);

    if (ke_state_get(prf_task_id) != PROXM_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_PROX_MONITOR
	#if (BLE_STREAMDATA_DEVICE)
    if (ke_state_get(TASK_STREAMDATAD) == STREAMDATAD_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_STREAMDATAD);
    }
    #endif // #if BLE_STREAMDATA_DEVICE
    #if (BLE_STREAMDATA_HOST)
    prf_task_id = KE_BUILD_ID(TASK_STREAMDATAH, conidx);
    if (ke_state_get(prf_task_id) != STREAMDATAH_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_STREAMDATA_HOST
    
    #if (BLE_SPS_SERVER)
    if (ke_state_get(TASK_SPS_SERVER) == SPS_SERVER_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_SPS_SERVER);
    }
    #endif // #if BLE_SPS_SERVER
    #if (BLE_SPS_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_SPS_CLIENT, conidx);
    if (ke_state_get(prf_task_id) != SPS_CLIENT_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id); // changed status -> conidx
    }
    #endif // #if BLE_SPS_CLIENT
    
    #if (BLE_FINDME_TARGET)
    if (ke_state_get(TASK_FINDT) == FINDT_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_FINDT);
    }
    #endif // #if BLE_FINDME_TARGET

    #if (BLE_FINDME_LOCATOR)
    prf_task_id = KE_BUILD_ID(TASK_FINDL, conidx);
    if (ke_state_get(prf_task_id) != FINDL_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_FINDME_LOCATOR

    #if (BLE_HT_THERMOM)
    if (ke_state_get(TASK_HTPT) == HTPT_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_HTPT);
    }
    #endif // #if BLE_HT_THERMOM

    #if (BLE_HT_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_HTPC, conidx);

    if (ke_state_get(prf_task_id) != HTPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_HT_COLLECTOR


    #if (BLE_DIS_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_DISC, conidx);

    if (ke_state_get(prf_task_id) != DISC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_DIS_CLIENT

    #if (BLE_TIP_SERVER)
    prf_task_id = KE_BUILD_ID(TASK_TIPS, conidx);

    if (ke_state_get(prf_task_id) == TIPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #BLE_TIP_SERVER

    #if (BLE_TIP_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_TIPC, conidx);

    if (ke_state_get(prf_task_id) != TIPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_TIP_CLIENT

    #if (BLE_BP_SENSOR)
    if (ke_state_get(TASK_BLPS) == BLPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_BLPS);
    }
    #endif // #BLE_BP_SENSOR

    #if (BLE_BP_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_BLPC, conidx);

    if (ke_state_get(prf_task_id) != BLPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_BP_COLLECTOR

    #if (BLE_HR_SENSOR)
    if (ke_state_get(TASK_HRPS) == HRPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_HRPS);
    }
    #endif // #BLE_HR_SENSOR

    #if (BLE_HR_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_HRPC, conidx);

    if (ke_state_get(prf_task_id) != HRPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_HR_COLLECTOR

    #if (BLE_SP_SERVER)
    if (ke_state_get(TASK_SCPPS) == SCPPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_SCPPS);
    }
    #endif // #BLE_SP_SERVER

    #if (BLE_SP_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_SCPPC, conidx);

    if (ke_state_get(prf_task_id) != SCPPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_SP_CLIENT

    #if (BLE_BAS_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_BASC, conidx);

    if (ke_state_get(prf_task_id) != BASC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_BAS_CLIENT

    #if (BLE_HID_DEVICE)
    if (ke_state_get(TASK_HOGPD) == HOGPD_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_HOGPD);
    }
    #endif // #if BLE_HID_DEVICE

    #if (BLE_HID_BOOT_HOST)
    prf_task_id = KE_BUILD_ID(TASK_HOGPBH, conidx);

    if (ke_state_get(prf_task_id) != HOGPBH_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_HID_BOOT_HOST

    #if (BLE_HID_REPORT_HOST)
    prf_task_id = KE_BUILD_ID(TASK_HOGPRH, conidx);

    if (ke_state_get(prf_task_id) != HOGPRH_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_HID_REPORT_HOST

    #if (BLE_GL_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_GLPC, conidx);

    if (ke_state_get(prf_task_id) != GLPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_GL_COLLECTOR)

    #if (BLE_GL_SENSOR)
    if (ke_state_get(TASK_GLPS) == GLPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_GLPS);
    }
    #endif // (BLE_GL_SENSOR)

    #if (BLE_NEB_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_NBPC, conidx);

    if (ke_state_get(prf_task_id) >= NBPC_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_NEB_CLIENT)

    #if (BLE_NEB_SERVER)
    if ((ke_state_get(TASK_NBPS) >= NBPS_CONNECTED))
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_NBPS);
    }
    #endif // (BLE_NEB_SERVER)

    #if (BLE_RSC_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_RSCPC, conidx);

    if (ke_state_get(prf_task_id) != RSCPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
	}
    #endif // (BLE_RSC_COLLECTOR)

    #if (BLE_RSC_SENSOR)
    if (ke_state_get(TASK_RSCPS) >= RSCPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_RSCPS);
    }
    #endif // (BLE_RSC_SENSOR)

    #if (BLE_CSC_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_CSCPC, conidx);

    if (ke_state_get(prf_task_id) != CSCPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_CSC_COLLECTOR)

    #if (BLE_CSC_SENSOR)
    if (ke_state_get(TASK_CSCPS) >= CSCPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_CSCPS);
    }
    #endif // (BLE_CSC_SENSOR)

    #if (BLE_CP_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_CPPC, conidx);

    if (ke_state_get(prf_task_id) != CPPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_CP_COLLECTOR)

    #if (BLE_CP_SENSOR)
    if (ke_state_get(TASK_CPPS) >= CPPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_CPPS);
    }
    #endif // (BLE_CP_SENSOR)

    #if (BLE_LN_COLLECTOR)
    prf_task_id = KE_BUILD_ID(TASK_LANC, conidx);

    if (ke_state_get(prf_task_id) != LANC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // (BLE_LN_COLLECTOR)

    #if (BLE_LN_SENSOR)
    if (ke_state_get(TASK_LANS) >= LANS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_LANS);
    }
    #endif // (BLE_LN_SENSOR)

    #if (BLE_AN_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_ANPC, conidx);

    if (ke_state_get(prf_task_id) != ANPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_AN_CLIENT)

    #if (BLE_AN_SERVER)
    prf_task_id = KE_BUILD_ID(TASK_ANPS, conidx);

    if (ke_state_get(prf_task_id) >= ANPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_AN_SERVER)

    #if (BLE_ANC_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_ANCC, conidx);

    if (ke_state_get(prf_task_id) != ANCC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_ANC_CLIENT)

    #if (BLE_PAS_SERVER)
    prf_task_id = KE_BUILD_ID(TASK_PASPS, conidx);

    if (ke_state_get(prf_task_id) == PASPS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_PAS_SERVER)

    #if (BLE_PAS_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_PASPC, conidx);

    if (ke_state_get(prf_task_id) != PASPC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_PAS_CLIENT)

    #if (BLE_SAMPLE128)
    if (ke_state_get(TASK_SAMPLE128) == SAMPLE128_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_SAMPLE128);
    }
    #endif //(BLE_PAS_SERVER)
    
    #if (BLE_IEU)  
    if (ke_state_get(TASK_IEU) == IEU_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_IEU);
    }
    #endif // (BLE_IEU)
    
    #if (BLE_MPU)   
    if (ke_state_get(TASK_MPU) == MPU_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_MPU);
    }
    #endif // (BLE_MPU)
    
	#if (BLE_WPT_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_WPTC, conidx);
    if (ke_state_get(prf_task_id) != WPTC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif //(BLE_WPT_CLIENT)

    #if (BLE_WPTS)
    if (ke_state_get(TASK_WPTS) == WPTS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_WPTS);
    }
    #endif //(BLE_WPTS)

    #if (BLE_UDS_SERVER)
    if (ke_state_get(TASK_UDSS) >= UDSS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_UDSS);
    }
    #endif // (BLE_UDS_SERVER)
    
    #if (BLE_UDS_CLIENT)
    if (ke_state_get(TASK_UDSC) >= UDSC_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_UDSC);
    }
    #endif // (BLE_UDS_CLIENT)

    #if (BLE_WSS_SERVER)
    if (ke_state_get(TASK_WSSS) >= WSSS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_WSSS);
    }
    #endif // (BLE_WSS_SERVER)
    
    #if (BLE_WSS_COLLECTOR)
    if (ke_state_get(TASK_WSSC) >= WSSC_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_WSSC);
    }
    #endif // (BLE_WSS_COLLECTOR)

    #if (BLE_ADC_NOTIFY)
    if (ke_state_get(TASK_ADC_NOTIFY) >= ADC_NOTIFY_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_ADC_NOTIFY);
    }
    #endif // #if BLE_STREAMDATA_HOST
    
    #if (BLE_DEVICE_CONFIG)
    if (ke_state_get(TASK_DEVICE_CONFIG) >= DEVICE_CONFIG_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_DEVICE_CONFIG);
    }
    #endif // #if BLE_STREAMDATA_HOST
    
    #if (BLE_BAS_SERVER)
    if (ke_state_get(TASK_BASS) == BASS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_BASS);
    }
    #endif // #BLE_SP_SERVER
    #if (BLE_DIS_SERVER)
    if (ke_state_get(TASK_DISS) == DISS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_DISS);
    }
    #endif // #BLE_DIS_SERVER
    #if (BLE_PROX_REPORTER)
    if (ke_state_get(TASK_PROXR) == PROXR_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_PROXR);
    }
    #endif // #if BLE_PROX_REPORTER

    #if (BLE_SPOTA_RECEIVER)
    if (ke_state_get(TASK_SPOTAR) == SPOTAR_ACTIVE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_SPOTAR);
    }
    #endif // #if BLE_SPOTA_RECEIVER

    #if (BLE_CUSTOM_SERVER)
    {
        uint8_t i=0;
        ke_task_id_t prf_task_id;
        while( cust_prf_funcs[i].task_id != TASK_NONE )
        {
            prf_task_id = KE_BUILD_ID(cust_prf_funcs[i++].task_id, conidx);
            gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);

        }
    }
    #endif         
 
    #if (BLE_BCS_SERVER)
    if (ke_state_get(TASK_BCSS) >= BCSS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_BCSS);
    }
    #endif // (BLE_WSS_SERVER)
    #if (BLE_BCS_CLIENT)
    prf_task_id = KE_BUILD_ID(TASK_BCSC, conidx);

    if (ke_state_get(prf_task_id) != BCSC_IDLE)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, prf_task_id);
    }
    #endif // #if BLE_BCS_CLIENT

    #if (BLE_CTS_SERVER)
    if (ke_state_get(TASK_CTSS) >= CTSS_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_CTSS);
    }
    #endif // (BLE_CTS_SERVER)

    #if (BLE_CTS_CLIENT)
    if (ke_state_get(TASK_CTSC) >= CTSC_CONNECTED)
    {
        gapc_send_disconect_ind(conidx, reason, conhdl, TASK_CTSC);
    }
    #endif // (BLE_CTS_CLIENT)
}

#endif /* ((BLE_SERVER_PRF || BLE_CLIENT_PRF)) */

/// @} PRF_UTILS

