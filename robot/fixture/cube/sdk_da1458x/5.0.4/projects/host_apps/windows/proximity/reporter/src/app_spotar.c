/**
 ****************************************************************************************
 *
 * @file app_spotar.c
 *
 * @brief Handling of ble spotar events and responses.
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

#include "app_spotar.h" 
#include "ble_msg.h"

/**
 ****************************************************************************************
 * @brief Initializes SPOTAR Application.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_init(void)
{	
    spota_state.spota_pd_idx = 0;
    spota_state.suota_block_idx = 0;
    spota_state.suota_img_idx = 0;
    spota_state.new_patch_len = 0;
    spota_state.crc_clac = 0;
    memset(spota_all_pd, 0x00, 4); // Set first WORD to 0x00
    spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;

	app_init_image_file();
}

/**
 ****************************************************************************************
 * @brief Send enable request for the SPOTAR profile.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_enable(void)
{
    // Allocate the message
    struct spotar_enable_req * req = BleMsgAlloc(SPOTAR_ENABLE_REQ, TASK_SPOTAR, TASK_GTL,
                                                  sizeof(struct spotar_enable_req));

    // Fill in the parameter structure
    req->conhdl = app_env.proxr_device.device.conhdl;
    req->sec_lvl = PERM(SVC, ENABLE);
    req->mem_dev = 0; // No Mem device specified yet.
    req->patch_mem_info = 0;  // SPOTA initiator needs to specify mem device first
    
    // Send the message
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Creates SPOTAR service database.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_create_db(void)
{
    // Add SPOTAR in the database
    struct spotar_create_db_req * req = BleMsgAlloc(SPOTAR_CREATE_DB_REQ, TASK_SPOTAR,
                                                        TASK_GTL, sizeof(struct spotar_enable_req));

    req->features = 0;

	// Send the message
    BleSendMsg((void *) req);
}

/**
 ****************************************************************************************
 * @brief Updates SPOTAR memory info characteristic.
 *
 * @param[in]   mem_info: Patch memory info. Number of patches and overall patch length.
 *
 * @return      void.
 ****************************************************************************************
 */
void spotar_send_mem_info_update_req(uint32_t mem_info)
{
    // Inform SPOTAR task. 
    struct spotar_patch_mem_info_upadet_req *req = BleMsgAlloc(SPOTAR_PATCH_MEM_INFO_UPDATE_REQ,
														TASK_SPOTAR, TASK_GTL,
														sizeof(struct spotar_patch_mem_info_upadet_req));

    req->mem_info = mem_info;

    // Send the message
    BleSendMsg(req);
}

/**
 ****************************************************************************************
 * @brief Updates SPOTAR status characteristic.
 *
 * @param[in]   SPOTAR application status.
 *
 * @return      void
 ****************************************************************************************
 */
void spotar_send_status_update_req(uint8_t status)
{   
    // Inform SPOTAR task. 
    struct spotar_status_upadet_req *req = BleMsgAlloc(SPOTAR_STATUS_UPDATE_REQ,
												TASK_SPOTAR, TASK_GTL,
												sizeof(struct spotar_status_upadet_req));

	req->conhdl = app_env.proxr_device.device.conhdl;
    req->status = status;

    // Send the message
    BleSendMsg(req);

// Error msg prints
#if 1
	switch (status) 
	{
		case SPOTAR_SRV_STARTED:      
			//printf("\nSPOTAR_SRV_STARTED\n");
			break;
		case SPOTAR_CMP_OK:
			//printf("\nSPOTAR_CMP_OK\n");
			break;
		case SPOTAR_SRV_EXIT:
			printf("\nSPOTAR_SRV_EXIT\n");
			break;
		case SPOTAR_CRC_ERR:
			printf("\nSPOTAR_CRC_ERR\n");
			break;
		case SPOTAR_PATCH_LEN_ERR:
			printf("\nSPOTAR_PATCH_LEN_ERR\n");
			break;
		case SPOTAR_EXT_MEM_WRITE_ERR:
			printf("\nSPOTAR_EXT_MEM_WRITE_ERR\n");
			break;
		case SPOTAR_INT_MEM_ERR:
			printf("\nSPOTAR_INT_MEM_ERR\n");
			break;
		case SPOTAR_INVAL_MEM_TYPE:
			printf("\nSPOTAR_INVAL_MEM_TYPE\n");
			break;
		case SPOTAR_APP_ERROR:
			printf("\nSPOTAR_APP_ERROR\n");
			break;  
		// SUOTAR application specific error codes
		case SPOTAR_IMG_STARTED:
			//printf("\nSPOTAR_IMG_STARTED\n");
			break;
		case SPOTAR_INVAL_IMG_BANK:
			printf("\nSPOTAR_INVAL_IMG_BANK\n");
			break;
		case SPOTAR_INVAL_IMG_HDR:
			printf("\nSPOTAR_INVAL_IMG_HDR\n");
			break;
		case SPOTAR_INVAL_IMG_SIZE:
			printf("\nSPOTAR_INVAL_IMG_SIZE\n");
			break;
		case SPOTAR_INVAL_PRODUCT_HDR:
			printf("\nSPOTAR_INVAL_PRODUCT_HDR\n");
			break;
		case SPOTAR_SAME_IMG_ERR:
			printf("\nSPOTAR_SAME_IMG_ERR\n");
			break;
		case SPOTAR_EXT_MEM_READ_ERR:
			printf("\nSPOTAR_EXT_MEM_READ_ERR\n");
			break;
		default:
			printf("\n Status code=[0x%X]\n", status);
			break;
	}
#endif
}

/**
 ****************************************************************************************
 * @brief Starts SPOTAR serivce and disables sleep.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_start(void)
{	        
    spotar_send_status_update_req((uint8_t) SPOTAR_IMG_STARTED);    

    // Initialise indexes
    spota_state.spota_pd_idx = 0;
    spota_state.suota_block_idx = 0;
    spota_state.suota_img_idx = 0;
    spota_state.new_patch_len = 0;
    spota_state.crc_clac = 0;
	spota_state.upgrade_file = NULL;
}

/**
 ****************************************************************************************
 * @brief Resets SPOTAR Apllication.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_reset(void)
{	
    spota_state.spota_pd_idx = 0;
    spota_state.new_patch_len = 0;
	spota_state.upgrade_file = NULL;
}

/**
 ****************************************************************************************
 * @brief Stops SPOTAR service and resets application
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_stop(void)
{
    // Set memory device to invalid type so that service will not 
    // start until the memory device is explicitly set upon service start
    spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;

	app_close_image_file(spota_state.upgrade_file);
}

/**
 ****************************************************************************************
 * @brief This function is called when the entire image has been received successfully
 *        to set the valid flag field on the image header and make the image valid for
 *        the bootloader.
 *
 * @param[in]   void
 *
 * @return      SPOTA Status values 
 *              
 ****************************************************************************************
 */
uint8_t app_set_image_valid_flag(void)
{
	int8_t ret = SPOTAR_CMP_OK;
    uint8_t flag = STATUS_VALID_IMAGE;
	ret = app_write_ext_mem(&flag, spota_state.upgrade_file, (uint32_t)(spota_state.mem_base_add + 2), 1);    
	if (ret != sizeof(uint8_t)) return SPOTAR_EXT_MEM_WRITE_ERR;
		else return SPOTAR_CMP_OK;
}

/**
 ****************************************************************************************
 * @brief Handles the SPOTAR_CREATE_DB_CFM message.
 *
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GTL).
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_create_db_cfm_handler(ke_task_id_t const dest_id, struct spotar_create_db_cfm *param)
{
    if (dest_id != TASK_GTL)
	{
		printf("Error creating SPOTAR DB. Destination ID is not TASK_GTL \n");
		return (KE_MSG_CONSUMED);
	}
	    
	if (param->status == CO_ERROR_NO_ERROR)
	{
		// If state is not idle, ignore the message
		if (app_env.state == APP_IDLE)
		{
			app_set_mode(); // initialize gap mode 
		}
	}else
		printf("Error creating SPOTAR DB \n");

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles memory device indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_mem_dev_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_mem_dev_ind *param)
{	
	uint32_t mem_info = 0;

    if (param->char_code)
    {
		 app_spotar_read_mem(param->mem_dev, &mem_info);
  
        // Write memory info to db        
        // Inform SPOTAR task. Allocate the memory info update indication
        spotar_send_mem_info_update_req(mem_info);     
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Reads memory device and writes memory info.
 *
 * @param[in]   mem_dev: MSbyte holds the Memory device type, rest is the base address.
 * @param[in]   mem_info: 16MSbits hold number of patches, 16LSbits hold overall mem len.
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_read_mem(uint32_t mem_dev, uint32_t* mem_info)
{  
	uint8_t mem_dev_cmd;
    *mem_info = 0;

	mem_dev_cmd = (uint8_t) ((mem_dev & SPOTAR_READ_MEM_DEV_TYPE) >> 24);
    
    // If valid memory device, store memory device type and base address
    if (mem_dev_cmd < SPOTAR_MEM_INVAL_DEV)
    {
        spota_state.mem_dev = mem_dev_cmd;
        spota_state.suota_image_bank = mem_dev & SPOTAR_READ_MEM_BASE_ADD;

        // Store requested image bank
        if ((spota_state.mem_dev == SPOTAR_IMG_I2C_EEPROM) ||
            (spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH))
        {
            if (spota_state.suota_image_bank > 2)
            {
                // Invalid image bank
                spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_IMG_BANK);
                return;
            }
        }
    }  
    
    switch (mem_dev_cmd)
    {
        case SPOTAR_IMG_I2C_EEPROM: 
        case SPOTAR_IMG_SPI_FLASH:
			printf("\nSoftware upgrade started at [%s]\n", app_print_time());
            // Initialise SPOTA Receiver app
            app_spotar_start();
            break;

        case SPOTAR_MEM_SERVICE_EXIT:
            app_spotar_stop();
            app_spotar_reset();
            // Initiator requested to exit service. Send notification to initiator
            spotar_send_status_update_req((uint8_t) SPOTAR_SRV_EXIT);
            break;
        
        case SPOTAR_IMG_END:
        {
            uint8_t ret;
			printf("\n");
            // Initiator requested to exit service. Calculate CRC if succesfull, send notification to initiator
            if (spota_state.crc_clac != 0)
            {
				printf("Software upgrade ended with CRC Error\n");
                spotar_send_status_update_req((uint8_t) SPOTAR_CRC_ERR);
            }
            else
            {  
				printf("Software upgrade ended successfully at: [%s]\n\n", app_print_time());
                ret = app_set_image_valid_flag();
                spotar_send_status_update_req((uint8_t) ret);
				app_print_image_data();
            }
            app_spotar_stop();
            app_spotar_reset();
            break;
        }
        
        case SPOTAR_REBOOT:
            // Reset the HOST
            break;
        
        default:
            spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_MEM_TYPE);
            spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;
            *mem_info = 0;
            break;
    }            
}

/**
 ****************************************************************************************
 * @brief Handles Patch Length indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_len_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_len_ind const *param)
{	  
    if (param->char_code)
    {   
        if (param->len == 0) 
        {
            // Invalid PATCH_LEN char value
            spotar_send_status_update_req((uint8_t) SPOTAR_PATCH_LEN_ERR);
        }
        else
        {   
			// Check if the memory device has been set correctly
            if( SPOTAR_IS_VALID_MEM_DEV (spota_state.mem_dev) )
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

    return (KE_MSG_CONSUMED);
}
									  
/**
 ****************************************************************************************
 * @brief Handles patch data indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_data_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_data_ind const *param)
{		
    if (spota_state.mem_dev < SPOTAR_MEM_INVAL_DEV)
    {    
        if (param->char_code)
        {
            if (spota_state.new_patch_len)
            {
                //---------------------------- Handle SPOTAR image payload -----------------------
                if ((spota_state.suota_block_idx + param->len) <= SPOTA_OVERALL_PD_SIZE)               
                {
					printf("#");
					if (spota_state.suota_img_idx && spota_state.new_patch_len)
						if (((spota_state.suota_img_idx / spota_state.new_patch_len) % 5 == 4) && (spota_state.suota_block_idx == spota_state.new_patch_len - param->len))
							printf("\n"); // Every 5 blocks

                    memcpy(&spota_all_pd[spota_state.suota_block_idx], param->pd, param->len);
                    spota_state.suota_block_idx += param->len;
                        
                    if (spota_state.new_patch_len == spota_state.suota_block_idx)
                    {
                        app_spotar_img_hdlr();
                    }
                        
                    if (spota_state.suota_block_idx > spota_state.new_patch_len)
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
            else
            {
                // Invalid PATCH_LEN char value
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
 * @brief SPOTA image block handler. Validates image block and stores it to 
 *        external memory device.
 *
 * @param[in]   void
 *
 * @return      void
 *
 ****************************************************************************************
 */
void app_spotar_img_hdlr(void)
{
    uint32_t mem_info;
    uint16_t status = SPOTAR_CMP_OK;
    uint32_t ret;
    uint32_t i;

    // Update CRC
    for (i=0; i<spota_state.suota_block_idx; i++) {       
		spota_state.crc_clac ^= spota_all_pd[i];                                                                        
    }

    // When the first block is received, read image header first
    if (spota_state.suota_block_idx != 0 && spota_state.suota_img_idx == 0)
    {
        // Read image headers and determine active image.
        ret = app_read_image_headers(spota_state.suota_image_bank, spota_all_pd, spota_state.suota_block_idx);
        if (ret != IMAGE_HEADER_OK) 
        {
            status = ret;
        }
        else
        {
            // Update block index
            spota_state.suota_img_idx += spota_state.suota_block_idx;
        }
    }
    else
    {   
        // Check file size
        if ((spota_state.suota_image_len + ADDITINAL_CRC_SIZE) >= (spota_state.suota_img_idx + spota_state.suota_block_idx))
        {
            if (spota_state.suota_image_len < (spota_state.suota_img_idx + spota_state.suota_block_idx))
                spota_state.suota_block_idx = spota_state.suota_image_len - spota_state.suota_img_idx;

            ret = app_write_ext_mem(spota_all_pd, spota_state.upgrade_file, (spota_state.mem_base_add + spota_state.suota_img_idx), spota_state.suota_block_idx); 
            if (ret != spota_state.suota_block_idx) {
                status = SPOTAR_EXT_MEM_WRITE_ERR;
            }
            else
            {
                // Update block index
                spota_state.suota_img_idx += spota_state.suota_block_idx;
            }
        }
        else
        {
            status = SPOTAR_EXT_MEM_WRITE_ERR;
        }
    }
    spota_state.suota_block_idx = 0;
    mem_info = spota_state.suota_img_idx;
    spotar_send_mem_info_update_req(mem_info);
      
    // SPOTA finished successfully. Send Indication to initiator
    spotar_send_status_update_req((uint8_t) status);
}

/**
 ****************************************************************************************
 * @brief This function is called when the first SUOTA block is received.
 *        Firstly, the image header is extracted from the first block, then the external memmory
 *        is checked to determin where to write this new image and finaly the header and the first
 *        image data are written to external memory.
 *
 * @param[in]   bank:      User can select bank=1 or bank=2 to force where to write the image
 *                         bypassing the check to update the oldest image in the ext memory
 *              data:      Points to the first data block received over SUOTAR protocol
 *              data_len:  Length of the data block
 *
 * @return      0 for success, otherwise error codes
 *              
 ****************************************************************************************
 */
int app_read_image_headers(uint8_t image_bank, uint8_t* data, uint32_t data_len) 
{
    image_header_t *pImageHeader;
#if 0
    product_header_t *pProductHeader;
#endif
    image_header_t *pfwHeader;
    uint32_t codesize;
    int32_t ret;
    uint8_t mem_data_buff[64];
	uint32_t imageposition1;
    uint32_t imageposition2;
    FILE *imagefile1;
    FILE *imagefile2;
    uint8_t new_bank = 0;
    uint8_t is_invalid_image1 = 0;
    uint8_t is_invalid_image2 = 0;
    uint8_t imageid = 0;
    uint8_t imageid1 = 0;
    uint8_t imageid2 = 0;
    
    
    if (data_len < sizeof(image_header_t)) 
    {
        // block size should be at least image header size
        return SPOTAR_INVAL_IMG_HDR;  
    }
    else
    {
        // read header from first data block
        pfwHeader = (image_header_t*)data;
    }

    // check firmware header 
    if ((pfwHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) || (pfwHeader->signature[1] != IMAGE_HEADER_SIGNATURE2))
    {
        return SPOTAR_INVAL_IMG_HDR;
    }
    
    // Get image size
    codesize = pfwHeader->code_size;
    spota_state.suota_image_len = pfwHeader->code_size + sizeof(image_header_t);

	// Used in DA14580 product header format
#if 0
    // read product header
    pProductHeader = (product_header_t*) mem_data_buff;
    ret = app_read_ext_mem((uint8_t*)mem_data_buff, (unsigned long)PRODUCT_HEADER_POSITION, (unsigned long)sizeof(product_header_t));
    if( ret != sizeof(product_header_t) ) return SPOTAR_EXT_MEM_READ_ERR;
        
    // verify product header
    if ((pProductHeader->signature[0]!=PRODUCT_HEADER_SIGNATURE1) || 
        (pProductHeader->signature[1]!=PRODUCT_HEADER_SIGNATURE2))
    {
		return SPOTAR_INVAL_PRODUCT_HDR;
    }

	// Store the start addresses of both images positions
    imageposition1 = pProductHeader->offset1;
    imageposition2 = pProductHeader->offset2;
    
    // verify image positions
    // the code size must be less than the space between images 
    if (!codesize || codesize > (imageposition2 - imageposition1))
    {
        return SPOTAR_INVAL_IMG_SIZE;
    }
#else
	// Since files are used, image access is from the first position of each file.
	imageposition1 = 0;
    imageposition2 = 0;
#endif

    pImageHeader = (image_header_t*)mem_data_buff;

	if ((imagefile1 = app_open_image_file(UPGRADE_IMAGE1)) == NULL) 
	{
		return SPOTAR_EXT_MEM_READ_ERR;
	}

    // read image header 1, image id must be stored for the bank selection
    ret = app_read_ext_mem((uint8_t*)pImageHeader, imagefile1, imageposition1, (unsigned long)sizeof(image_header_t));
    if (ret != sizeof(image_header_t)) return SPOTAR_EXT_MEM_READ_ERR;

    imageid1 = pImageHeader->imageid;
    if( (pImageHeader->validflag != STATUS_VALID_IMAGE)         ||
        (pImageHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) ||
        (pImageHeader->signature[1] != IMAGE_HEADER_SIGNATURE2))
    {
        is_invalid_image1 = 1;
        imageid1 = 0;
    } else {
        // compare version strings and timestamps
        if (!memcmp(pfwHeader->version, pImageHeader->version, 16) && (pfwHeader->timestamp == pImageHeader->timestamp))
        {
            is_invalid_image1 = 2;
        }
    }

	if ((imagefile2 = app_open_image_file(UPGRADE_IMAGE2)) == NULL) 
	{
		return SPOTAR_EXT_MEM_READ_ERR;
	}

    // read image header 2, image id must be stored for the bank selection
    ret = app_read_ext_mem((uint8_t*)pImageHeader, imagefile2, imageposition2, (unsigned long)sizeof(image_header_t));                
    if (ret != sizeof(image_header_t)) return SPOTAR_EXT_MEM_READ_ERR;

    imageid2 = pImageHeader->imageid;
    if ((pImageHeader->validflag != STATUS_VALID_IMAGE)         ||
        (pImageHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) ||
        (pImageHeader->signature[1] != IMAGE_HEADER_SIGNATURE2))
    {
        is_invalid_image2 = 1;
        imageid2 = 0;
    } else {
        //compare version strings and timestamps
        if (!memcmp(pfwHeader->version, pImageHeader->version, 16) && (pfwHeader->timestamp == pImageHeader->timestamp))
        {
            is_invalid_image2 = 2;
        }
    }

    if (image_bank == 0 || image_bank > 2)
    {
        if (is_invalid_image1 == 1) new_bank = 1;
        else if (is_invalid_image2 == 1) new_bank = 2;
        else new_bank = app_find_old_img(imageid1, imageid2);
    }
    else
    {
        new_bank = image_bank;
    }
 
    memset(mem_data_buff, 0xFF, sizeof(mem_data_buff));
    if (new_bank == 2)
    { 
		spota_state.upgrade_file = imagefile2; // before error return, to keep the fd for closing
        if (is_invalid_image2 == 2) return SPOTAR_SAME_IMG_ERR;
		spota_state.mem_base_add = 0x0;

        if (is_invalid_image1 == 0 || is_invalid_image1 == 2)
            imageid = app_get_image_id(imageid1);
        else
            imageid = 1;
    }
    else
    {
		spota_state.upgrade_file = imagefile1; 
        if (is_invalid_image1 == 2) return SPOTAR_SAME_IMG_ERR;
		spota_state.mem_base_add = 0x0;
        
        if (is_invalid_image2 == 0 || is_invalid_image2==2)
            imageid = app_get_image_id(imageid2);
        else
            imageid = 1;
    }

	// Only the upgrade file should be opened.
	if (spota_state.upgrade_file == imagefile1)
	{
		app_close_image_file(imagefile2);
	}

	// Erase header and old image
	ret = app_write_ext_mem(mem_data_buff, spota_state.upgrade_file, spota_state.mem_base_add, sizeof(mem_data_buff)); 
    if (ret != CODE_OFFSET) return SPOTAR_EXT_MEM_WRITE_ERR;

    // Write header apart from validflag. Set validflag when the entire image has been received
    pImageHeader->imageid = imageid;
    pImageHeader->code_size = pfwHeader->code_size; 
    pImageHeader->CRC = pfwHeader->CRC;  
    memcpy(pImageHeader->version, pfwHeader->version, 16);  
    pImageHeader->timestamp = pfwHeader->timestamp; 
    pImageHeader->signature[0] = pfwHeader->signature[0]; 
    pImageHeader->signature[1] = pfwHeader->signature[1]; 

    // write image
	ret = app_write_ext_mem((uint8_t*)&data[CODE_OFFSET], spota_state.upgrade_file, spota_state.mem_base_add + CODE_OFFSET, data_len - CODE_OFFSET); 
	if (ret != (data_len - CODE_OFFSET)) return SPOTAR_EXT_MEM_WRITE_ERR;

	// write header
	ret = app_write_ext_mem((uint8_t*)pImageHeader, spota_state.upgrade_file, spota_state.mem_base_add, sizeof(image_header_t));
	if( ret != sizeof(image_header_t) )return SPOTAR_EXT_MEM_WRITE_ERR;
     
    return IMAGE_HEADER_OK;
}

/**
 ****************************************************************************************
 * @brief This function is called to get the correct image id to write to the image header
 *
 * @param[in]   imgid: current latest image id
 *
 * @return      new image id 
 *              
 ****************************************************************************************
 */
uint8_t app_get_image_id(uint8_t imgid)
{
	uint8_t new_imgid = 0;
	if (imgid == 0xff) return new_imgid;
    else {
 		new_imgid = imgid + 1;
		return new_imgid;
	}
}

/**
 ****************************************************************************************
 * @brief This function is called to find the older image of the two in the external memmory 
 *
 * @param[in]   imgid: current ids of the two images
 *
 * @return      older image id 
 *              
 ****************************************************************************************
 */
uint8_t app_find_old_img(uint8_t id1, uint8_t id2)
{
	if (id1 == 0xFF && id2 == 0xFF) return 1;
	if (id1 == 0xFF && id2 == 0) return 1;
	if (id2 == 0xFF && id1 == 0) return 2;
	if (id1 > id2) return 2;
	else return 1;
}

/**
 ****************************************************************************************
 * @brief Read data from a given starting address 
 *
 * @param[in] *rd_data_ptr:  Points to the position the read data will be stored
 * @param[in] fp:			 File pointer of the image file
 * @param[in] size:          Size of the data to be read
 * 
 * @return  Number of read bytes or error code
 ****************************************************************************************
 */
int app_read_ext_mem(uint8_t *rd_data_ptr, FILE *fp, uint32_t address, uint32_t size)
{
	if (fp == NULL) {
		printf("Cannot find upgrade file to read.\n");
		return -1;
	}
	if (fseek(fp, address, SEEK_SET) == 0)
	{
		return fread(rd_data_ptr, 1, size, fp);
	}
	return 0;
}

/**
 ****************************************************************************************
 * @brief Write data from a given starting address 
 *
 * @param[in] *rd_data_ptr:  Points to the position the read data will be stored
 * @param[in] fp:			 File pointer of the image file
 * @param[in] size:          Size of the data to be read
 * 
 * @return  Number of read bytes or error code
 ****************************************************************************************
 */
int app_write_ext_mem(uint8_t *rd_data_ptr, FILE *fp, uint32_t address, uint32_t size)
{
	if (fp == NULL) {
		printf("Cannot find upgrade file to write.\n");
		return -1;
	}
	if (fseek(fp, address, SEEK_SET) == 0)
	{
		return fwrite(rd_data_ptr, 1, size, fp);
	}
	return 0;		 
}

/**
 ****************************************************************************************
 * @brief Write data from a given starting address 
 *
 * @param[in] *rd_data_ptr:  Points to the position the read data will be stored
 * @param[in] fp:			 File pointer of the image file
 * @param[in] size:          Size of the data to be read
 * 
 * @return  Number of read bytes or error code
 ****************************************************************************************
 */
FILE *app_open_image_file(char *filename)
{
	FILE *fp;
	fp = fopen(filename, "rb+");
	if (fp == NULL)
		printf("Cannot open file: [%s] \n", filename);
	return fp;	
}

/**
 ****************************************************************************************
 * @brief This function is called to close a disk file after the 
 *				upgraded image has been saved to it or after an error on the suota process.
 *
 * @param[in]   fp: The file pointer to be closed
 *
 * @return      void
 *              
 ****************************************************************************************
 */
void app_close_image_file(FILE *fp)
{
	int ret;
	if (fp) {
		ret = fclose(fp);
		if (ret)
			printf("Error closing image file \n");
		fp = NULL;
	}
}

/**
 ****************************************************************************************
 * @brief Prints upgraded image data taken from its header.
 *
 * @param[in]   void
 *
 * @return      void
 *              
 ****************************************************************************************
 */
void app_print_image_data(void)
{
	image_header_t pImageHeader;
	int32_t ret;
	uint32_t imageposition1 = 0;

	// read image header 1, image id must be stored for the bank selection
    ret = app_read_ext_mem((uint8_t*)&pImageHeader, spota_state.upgrade_file, imageposition1, (unsigned long)sizeof(image_header_t));
    if (ret != sizeof(image_header_t)) {
		printf("Cannot read the header of the upgraded file \n");
		return;
	}
	
	printf("Total image length:\t[%d]\n", pImageHeader.code_size + sizeof(image_header_t));
	printf("Version:\t\t[%s]\n", pImageHeader.version);
	printf("Image ID:\t\t[%d]\n", pImageHeader.imageid);
	printf("CRC:\t\t\t[0x%X]\n", pImageHeader.CRC);
	printf("Timestamp:\t\t[%d]\n", pImageHeader.timestamp);
	printf("Signature[0]:\t\t[0x%X]\n", pImageHeader.signature[0]);
	printf("Signature[1]:\t\t[0x%X]\n", pImageHeader.signature[1]);
	printf("Valid flag:\t\t[0x%X]\n", pImageHeader.validflag);
	printf("\n");
}

/**
 ****************************************************************************************
 * @brief Prints the local time in ms accuracy
 *
 * @param[in]   void
 *
 * @return      void
 *              
 ****************************************************************************************
 */
#include <Windows.h>
char currentTime[84] = "";
char *app_print_time(void)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(currentTime, "%d:%d:%d %dms", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return currentTime;
}

/**
 ****************************************************************************************
 * @brief This function is called to initialize the disk files.
 *			If the files exists, does not alter them.
 *			Otherwise it adds 0 bytes as long as the size of the image header.
 *
 * @param[in]   void
 *
 * @return      void
 *              
 ****************************************************************************************
 */
void app_init_image_file(void)
{
	FILE *fp;
	char init_buf[(unsigned long)sizeof(image_header_t)];
	int ret = 0;

	memset(init_buf, 0x0, (unsigned long)sizeof(image_header_t));

	fp = fopen(UPGRADE_IMAGE1, "r");
	if (fp)
	{
		printf("%s already exists.\n", UPGRADE_IMAGE1);
		fclose(fp);
		fp = NULL;
	}else {
		fp = fopen(UPGRADE_IMAGE1, "wb");
		if (fp == NULL)
		{
			printf("%s cannot open.\n", UPGRADE_IMAGE1);
			return;
		}
		ret = fwrite(init_buf, 1, sizeof(init_buf), fp);
		if (ret == sizeof(init_buf))
			printf("%s header init OK. \n", UPGRADE_IMAGE1);
		else
			printf("%s header init ERROR. \n", UPGRADE_IMAGE1);
		fclose(fp);
		fp = NULL;
	}

	fp = fopen(UPGRADE_IMAGE2, "r");
	if (fp)
	{
		printf("%s already exists.\n", UPGRADE_IMAGE2);
		fclose(fp);
		fp = NULL;
	}else {
		fp = fopen(UPGRADE_IMAGE2, "wb");
		if (fp == NULL)
		{
			printf("%s cannot open.\n", UPGRADE_IMAGE2);
			return;
		}
		ret = fwrite(init_buf, 1, sizeof(init_buf), fp);
		if (ret == sizeof(init_buf))
			printf("%s header init OK. \n", UPGRADE_IMAGE2);
		else
			printf("%s header init ERROR. \n", UPGRADE_IMAGE2);
		fclose(fp);
		fp = NULL;
	}
}

