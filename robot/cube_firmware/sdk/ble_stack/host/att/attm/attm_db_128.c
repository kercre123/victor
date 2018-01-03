/**
 ****************************************************************************************
 *
 * @file attm_db_128.c
 *
 * @brief ATTM functions that handle the ctration of service database of 128bits long UUID.
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

/*
 * INCLUDES
 ****************************************************************************************
 */
#include "attm_db_128.h"
#include "attm_db.h"
#include "atts_util.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 
 /**
 ****************************************************************************************
 * @brief Handles creation of database with 128bit long UUIDs
 * @param[in] cfg_flag configuration flag
 * @param[in] max_nb_att max number of database attributes
 * @param[in] dest_id task ID that requested the creation of the database.
 * @param[in] att_db pointer to the database description table
 * @param[out] shdl base handle of the database
 * @param[out] att_tbl if not NULL, it returns the attribute handles
 * @return status.
 ****************************************************************************************
 */

uint8_t attm_svc_create_db_128(uint16_t *shdl, uint8_t *cfg_flag, uint8_t max_nb_att,
                               uint8_t *att_tbl, ke_task_id_t const dest_id,
                               const struct attm_desc_128 *att_db)
{
    int32_t db_cfg;
    struct att_char_desc* char_desc;
    uint8_t nb_att_uuid_16 = 0;
    uint8_t nb_att_uuid_128 = 0;
    uint16_t total_size = 0;
    uint16_t handle;
    uint8_t status;
    uint8_t i;
    
    uint16_t att_decl_char = ATT_DECL_CHARACTERISTIC;
    
    if (cfg_flag != NULL)
    {
        //Number of attributes is limited to 32
        memcpy(&db_cfg, cfg_flag, sizeof(uint32_t));
    }
    else
    {
        //Set all bits to 1
        db_cfg = -1;
    }

    //Compute number of attributes and maximal payload size
    for (i = 0; i<max_nb_att; i++)
    {
        // check within db_cfg flag if attribute is enabled or not
        if (((db_cfg >> i) & 1) == 1)
        {
            switch(att_db[i].uuid_size)
            {
                case ATT_UUID_16_LEN:
                    // increment the total number of 16-bit UUID attributes to be added
                    nb_att_uuid_16++;
                    break;
                case ATT_UUID_128_LEN:
                    // increment the total number of 128-bit UUID attributes to be added
                    nb_att_uuid_128++;
                    break;
            }
            // set total size
            total_size += att_db[i].max_length;
        }
    }

    //Require memory allocation in database
    status = attmdb_add_service(shdl, dest_id, nb_att_uuid_16, 0, nb_att_uuid_128, total_size);

    for (i = 0; ((i<max_nb_att) && (status==ATT_ERR_NO_ERROR)); i++)
    {
        // check within db_cfg flag if attribute is enabled or not
        if (((db_cfg >> i) & 1) == 1)
        {
            //Add attribute
            status = attmdb_add_attribute(*shdl, att_db[i].max_length, att_db[i].uuid_size,
                                               att_db[i].uuid, att_db[i].perm, &handle);

            //Set attribute value
            if(status == ATT_ERR_NO_ERROR)
            {
                if(att_db[i].length > 0)
                {
                    if(att_db[i].uuid_size == ATT_UUID_16_LEN &&
                      (memcmp(att_db[i].uuid, &att_decl_char, sizeof(att_decl_char)) == 0))
                    {

                        //Cast attribute characteristic
                        char_desc = (struct att_char_desc*)att_db[i].value;
                        
                        //If char_desc.attr_hdl[0] = 0xFF, no need to save the handle offset
                        if ((char_desc->attr_hdl[0] != 0xFF) && (att_tbl != NULL))
                        {
                            //Save handle offset in attribute table
                            *(att_tbl + char_desc->attr_hdl[0]) = handle - *shdl;
                        }
                        
                        do {
                            //Handle of Characteristic Value Descriptor attribute
                            uint16_t char_val_desc_handle = handle + 1;
                            
                            status = attmdb_att_set_value(handle, att_db[i].length, att_db[i].value);
                            if (status != ATT_ERR_NO_ERROR) break;
                            
                            status = attmdb_att_partial_value_update(handle, 1, sizeof(char_val_desc_handle),
                                                                     (uint8_t*) &char_val_desc_handle);
                        } while(0);
                    }
                    else
                    {
                        status = attmdb_att_set_value(handle, att_db[i].length, att_db[i].value);
                    }
                }
            }
        }
    }

    return status;
}

