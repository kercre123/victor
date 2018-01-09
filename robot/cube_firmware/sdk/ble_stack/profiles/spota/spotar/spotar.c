/**
 ****************************************************************************************
 *
 * @file spotar.c
 *
 * @brief Software Programming Over The Air (SPOTA) Receiver Implementation.
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
 * @addtogroup SPOTAR
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_SPOTA_RECEIVER)

#include "gapc.h"
#include "spotar.h"
#include "spotar_task.h"
#include "attm_db.h"
#include "gap.h"
/*
 * SPOTA PROFILE ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

// SIG has assigned a 16bit UUID for SPOTAR service (0xFEF5). However we use proprietary UUIDs for the characteristics.
#define ATT_SVC_SPOTAR  0xFEF5 
#define SPOTA_MEM_DEV_UUID      {0x34, 0xcc, 0x54, 0xb9, 0xf9, 0x56, 0xc6, 0x91, 0x21, 0x40, 0xa6, 0x41, 0xa8, 0xca, 0x82, 0x80}
#define SPOTA_GPIO_MAP_UUID     {0x51, 0x86, 0xf0, 0x5a, 0x34, 0x42, 0x04, 0x88, 0x5f, 0x4b, 0xc3, 0x5e, 0xf0, 0x49, 0x42, 0x72}
#define SPOTA_MEM_INFO_UUID     {0xd4, 0x4f, 0x33, 0xfb, 0x92, 0x7c, 0x22, 0xa0, 0xfe, 0x45, 0xa1, 0x47, 0x25, 0xdb, 0x53, 0x6c}
#define SPOTA_PATCH_LEN_UUID    {0x31, 0xda, 0x3f, 0x67, 0x5b, 0x85, 0x83, 0x91, 0xd8, 0x49, 0x0c, 0x00, 0xa3, 0xb9, 0x84, 0x9d}
#define SPOTA_PATCH_DATA_UUID   {0xb2, 0x9c, 0x7b, 0xb1, 0xd0, 0x57, 0x16, 0x91, 0xa1, 0x4c, 0x16, 0xd5, 0xe8, 0x71, 0x78, 0x45}
#define SPOTA_SERV_STATUS_UUID  {0x88, 0x5c, 0x06, 0x6a, 0xeb, 0xb3, 0x0a, 0x99, 0xf5, 0x46, 0x8c, 0x79, 0x94, 0xdf, 0x78, 0x5f}

const char pdm_value_descr[] = "Mem Dev: B3=Mem, B[2:0]=Addr";
const char pdgpio_value_descr[] = "GPIO: b[3:0]= Pin, b[7:4]= Port Number";
const char pdmi_value_descr[] = "Mem Info: B[3:2]=#Patches, B[1:0]=Entire_Length";
const char pd_value_descr[] = "Patch Data: 20 bytes";
const char pdl_value_descr[] = "New patch len: 16 bits";
const char pds_value_descr[] = "SPOTA serv status: 8 bits";

/*
 *  SPOTA PROFILE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */
/// SPOTA Service
const uint8_t spotar_service[ATT_UUID_16_LEN]     = {0xF5,0xFE};

/// SPOTA Service - Patch Memory Device Characteristic
struct att_char128_desc spotar_patch_mem_dev_char = {(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR),
                                                                    {0,0},
                                                                    SPOTA_MEM_DEV_UUID};
const struct att_uuid_128 spotar_patch_mem_dev_val = {SPOTA_MEM_DEV_UUID};


/// SPOTA Service - GPIO mapping Characteristic
struct att_char128_desc spotar_gpio_map_char = {(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR),
                                                                    {0,0},
                                                                    SPOTA_GPIO_MAP_UUID};
const struct att_uuid_128 spotar_gpio_map_val = {SPOTA_GPIO_MAP_UUID};


/// SPOTA Service - Patch Memory Information Characteristic
struct att_char128_desc spotar_patch_mem_info_char = {(ATT_CHAR_PROP_RD),
                                                                    {0,0},
                                                                    SPOTA_MEM_INFO_UUID};
const struct att_uuid_128 spotar_patch_mem_info_val = {SPOTA_MEM_INFO_UUID};
                                                                    
                                                                                                                                        
/// SPOTA Service - Patch Length Characteristic
struct att_char128_desc spotar_patch_len_char = {(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR),
                                                                    {0,0},
                                                                    SPOTA_PATCH_LEN_UUID};
const struct att_uuid_128 spotar_patch_len_val = {SPOTA_PATCH_LEN_UUID};

/// SPOTA Service - Patch Data Characteristic
struct att_char128_desc spotar_patch_data_char = {(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_WR),
                                                                    {0,0},
                                                                    SPOTA_PATCH_DATA_UUID};
const struct att_uuid_128 spotar_patch_data_val = {SPOTA_PATCH_DATA_UUID};


/// SPOTA Service - Patch Status Characteristic
struct att_char128_desc spotar_patch_status_char = {(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF),
                                                                    {0,0},
                                                                    SPOTA_SERV_STATUS_UUID};
const struct att_uuid_128 spotar_patch_status_val = {SPOTA_SERV_STATUS_UUID};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct spotar_env_tag spotar_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// SPOTAR task descriptor
static const struct ke_task_desc TASK_DESC_SPOTAR = {spotar_state_handler, &spotar_default_handler, spotar_state, SPOTAR_STATE_MAX, SPOTAR_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void spotar_init(void)
{
    // Reset Environment
    memset(&spotar_env, 0, sizeof(spotar_env));

    // Create SPOTAR task
    ke_task_create(TASK_SPOTAR, &TASK_DESC_SPOTAR);

    ke_state_set(TASK_SPOTAR, SPOTAR_DISABLED);
}

void spotar_patched_data_rcved(uint8_t char_code)
{
    // Allocate the Patch Data value change indication
   struct spotar_patch_data_ind *ind = KE_MSG_ALLOC(SPOTAR_PATCH_DATA_IND,
                                              spotar_env.con_info.appid, TASK_SPOTAR,
                                              spotar_patch_data_ind);
   // Fill in the parameter structure
   ind->conhdl =  gapc_get_conhdl(spotar_env.con_info.conidx);

   ind->char_code = char_code;

   // Send the message
   ke_msg_send(ind);
}

void spotar_disable(uint16_t conhdl)
{
    // Disable SPOTA service in database
    attmdb_svc_set_permission(spotar_env.spota_shdl, PERM_RIGHT_DISABLE);

    struct spotar_disable_ind *ind = KE_MSG_ALLOC(SPOTAR_DISABLE_IND,
                                                 spotar_env.con_info.appid, TASK_SPOTAR,
                                                 spotar_disable_ind);
    
    ind->conhdl  = conhdl;
    
    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_SPOTAR, SPOTAR_IDLE);
}

#endif //BLE_SPOTA_RECEIVER

/// @} SPOTAR
