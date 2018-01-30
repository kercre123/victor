/**
 ****************************************************************************************
 *
 * @file app_spotar_task.c
 *
 * @brief SPOTA receiver application Message Handlers.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer
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
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"               // SW configuration

#if (BLE_APP_PRESENT)

#if (BLE_SPOTA_RECEIVER)

#include "app_spotar.h"
#include "app_spotar_task.h"
#include "app_task.h"                  // Application Task API
#include "gpio.h"
#include "app_entry_point.h"
#include "app.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles start indication from the SPOTAR profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_create_db_cfm_handler(ke_msg_id_t const msgid,
                                        struct spotar_create_db_cfm const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // If state is not idle, ignore the message
    if (ke_state_get(dest_id) == APP_DB_INIT)
    {
        // Inform the Application Manager
        struct app_module_init_cmp_evt *cfm = KE_MSG_ALLOC(APP_MODULE_INIT_CMP_EVT,
                                                       TASK_APP, TASK_APP,
                                                       app_module_init_cmp_evt);

        cfm->status = param->status;

        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles patch data indication from SPOTA receiver profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_patch_data_ind_handler(ke_msg_id_t const msgid,
                                         struct spotar_patch_data_ind const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    if( spota_state.mem_dev < SPOTAR_MEM_INVAL_DEV )
    {
        if (param->char_code)
        {
            if( spota_state.new_patch_len )
            {
#if (!SPOTAR_PATCH_DISABLE)
                if( SPOTAR_IS_FOR_PATCH( spota_state.mem_dev ) )
                {
                    //---------------------------- Handles SPOTAR patch payload -----------------------
                    if( (spota_state.spota_pd_idx + param->len) <= SPOTA_NEW_PD_SIZE)
                    {
                        memcpy(&spota_new_pd[spota_state.spota_pd_idx], param->pd, param->len );
                        spota_state.spota_pd_idx += param->len;

                        if( spota_state.new_patch_len == spota_state.spota_pd_idx )
                        {
                            app_spotar_pd_hdlr();
                        }

                        if( spota_state.spota_pd_idx > spota_state.new_patch_len  )
                        {
                            // Received patch len not equal to PATCH_LEN char value
                            spotar_send_status_update_req((uint8_t) SPOTAR_PATCH_LEN_ERR);
                        }
                    }
                    else
                    {
                        spotar_send_status_update_req((uint8_t) SPOTAR_INT_MEM_ERR);
                    }
                }
#endif //(!SPOTAR_PATCH_DISABLE)
#if (!SPOTAR_UPDATE_DISABLE)
                if( SPOTAR_IS_FOR_IMAGE( spota_state.mem_dev ) )
                {
                    //---------------------------- Handle SPOTAR image payload -----------------------
                    if( (spota_state.suota_block_idx + param->len) <= SPOTA_OVERALL_PD_SIZE)
                    {
                        memcpy(&spota_all_pd[spota_state.suota_block_idx], param->pd, param->len );
                        spota_state.suota_block_idx += param->len;

                        if( spota_state.new_patch_len == spota_state.suota_block_idx )
                        {
                            app_spotar_img_hdlr();
                        }

                        if( spota_state.suota_block_idx > spota_state.new_patch_len  )
                        {
                            // Received patch len not equal to PATCH_LEN char value
                            spotar_send_status_update_req((uint8_t) SPOTAR_PATCH_LEN_ERR);
                        }
                    }
                    else
                    {
                        spotar_send_status_update_req((uint8_t) SPOTAR_INT_MEM_ERR);
                    }
                }
#endif //(!SPOTAR_UPDATE_DISABLE)
            }
            else
            {
                // Inavild PATCH_LEN char value
                spotar_send_status_update_req((uint8_t) SPOTAR_PATCH_LEN_ERR);
            }
        }
    }
    else
    {
        spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_MEM_TYPE);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles memory device indication from SPOTA receiver profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_patch_mem_dev_ind_handler(ke_msg_id_t const msgid,
                                            struct spotar_patch_mem_dev_ind const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    uint32_t mem_info;

    if (param->char_code)
    {
        app_spotar_read_mem(param->mem_dev, &mem_info);

        // write memory info to db
        // Inform SPOTAR task. Allocate the  memory info updadte indication
        spotar_send_mem_info_update_req(mem_info);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GPIO mapping indication from SPOTA receiver profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_gpio_map_ind_handler(ke_msg_id_t const msgid,
                                       struct spotar_gpio_map_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint32_t mem_info;

    if (param->char_code)
    {
         spota_state.gpio_map = (uint32_t)param->gpio_map;

        // Setting GPIO map is only valid for external devices
        if( (spota_state.mem_dev == SPOTAR_MEM_I2C_EEPROM) ||
            (spota_state.mem_dev == SPOTAR_MEM_SPI_FLASH)     )
        {
            mem_info = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), spota_state.gpio_map);
            spotar_send_mem_info_update_req(mem_info);
        }
        else if( (spota_state.mem_dev == SPOTAR_IMG_I2C_EEPROM)  ||
                 (spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH )     )
        {
            // return ACTIVE_IMAGE info

        }
        else
        {
            spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_MEM_TYPE);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles Patch Length indication from SPOTA receiver profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int spotar_patch_len_ind_handler(ke_msg_id_t const msgid,
                                        struct spotar_patch_len_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    if (param->char_code)
    {   // Patchs are send in words (usually 4 bytes). So overall length in bytes should be multiple of 4.
        if( (param->len == 0) || (( SPOTAR_IS_FOR_PATCH( spota_state.mem_dev ) ) && param->len % sizeof(uint32_t) != 0) )
        {
            // Inavid PATCH_LEN char value
            spotar_send_status_update_req((uint8_t) SPOTAR_PATCH_LEN_ERR);
        }
        else
        {
            // Check if the memory device has been set correctly
            if( SPOTAR_IS_VALID_MEM_DEV (spota_state.mem_dev) )
            {
                if( SPOTAR_IS_FOR_PATCH( spota_state.mem_dev ) )
                {
                    // SPOTAR is configured to receive patch
                    if( param->len > SPOTA_NEW_PD_SIZE )
                    {
                        // Not enough memory for new patch. Update status.
                        spotar_send_status_update_req((uint8_t) SPOTAR_INT_MEM_ERR);
                    }
                    else
                    {
                        // When initiator sets the PATCH_LEN value, the SPOTA receiver should reset
                        app_spotar_reset();
                        spota_state.new_patch_len = param->len;
                    }
                }
                else
                {
                    // SPOTAR is configured to recive image, check SYSRAM buffer size to determine if the block can be stored
                    if( param->len > SPOTA_OVERALL_PD_SIZE )
                    {
                        // Not enough memory for new patch. Update status.
                        spotar_send_status_update_req((uint8_t) SPOTAR_INT_MEM_ERR);
                    }
                    else
                    {
                        spota_state.new_patch_len = param->len;
                    }

                }
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_spotar_process_handlers[]=
{
    {SPOTAR_PATCH_MEM_DEV_IND,              (ke_msg_func_t)spotar_patch_mem_dev_ind_handler},
    {SPOTAR_GPIO_MAP_IND,                   (ke_msg_func_t)spotar_gpio_map_ind_handler},
    {SPOTAR_PATCH_LEN_IND,                  (ke_msg_func_t)spotar_patch_len_ind_handler},
    {SPOTAR_PATCH_DATA_IND,                 (ke_msg_func_t)spotar_patch_data_ind_handler},
    {SPOTAR_CREATE_DB_CFM,                  (ke_msg_func_t)spotar_create_db_cfm_handler},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_spotar_process_handler(ke_msg_id_t const msgid,
                                                       void const *param,
                                                       ke_task_id_t const dest_id,
                                                       ke_task_id_t const src_id,
                                                       enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_spotar_process_handlers,
                                         sizeof(app_spotar_process_handlers) / sizeof(struct ke_msg_handler));
}

#endif //BLE_SPOTA_RECEIVER

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
