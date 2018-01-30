/**
 ****************************************************************************************
 *
 * @file app_spotar.c
 *
 * @brief SPOTA Reporter Application entry point
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
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_SPOTA_RECEIVER)

#include "app.h"                     // application definitions
#include "app_task.h"                // application task definitions
#include "spotar_task.h"             // SPOTA functions
#include "app_spotar.h" 
#include "gpio.h"
#if (!SPOTAR_SPI_DISABLE)
#include "spi.h"
#include "spi_flash.h"
#endif // (SPOTAR_SPI_DISABLE)
#include "arch_api.h"
#include "user_periph_setup.h"

#if (BLE_APP_KEYBOARD)    
#include "app_kbd_fsm.h"
#endif

#include "user_callback_config.h" 
#include "app_prf_perm_types.h"

//application SPOTA state structure
app_spota_state spota_state __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#ifdef __GNUC__
uint8_t spota_new_pd[SPOTA_NEW_PD_SIZE] __attribute__((section("spotar_patch_area"),aligned(4),zero_init));  // word aligned buffer to store new patch from initiator
uint8_t spota_all_pd[SPOTA_OVERALL_PD_SIZE] __attribute__((section("spotar_patch_area"),aligned(4),zero_init)); // word aligned buffer to read the patch to before patch execute
#else
__align(4) uint8_t spota_new_pd[SPOTA_NEW_PD_SIZE] __attribute__((section("spotar_patch_area"),zero_init));  // word aligned buffer to store new patch from initiator
__align(4) uint8_t spota_all_pd[SPOTA_OVERALL_PD_SIZE] __attribute__((section("spotar_patch_area"),zero_init)); // word aligned buffer to read the patch to before patch execute 
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/
#if (!SPOTAR_SPI_DISABLE)
void app_spotar_spi_config(spi_gpio_config_t *spi_conf);
void app_spi_flash_init(SPI_Pad_t *);
int8_t app_erase_flash_sectors(uint32_t, uint16_t); 
#endif

#if (!SPOTAR_I2C_DISABLE)
void app_spotar_i2c_config(i2c_gpio_config_t *i2c_conf);
#endif

int app_read_ext_mem( uint8_t *, uint32_t, uint32_t );
int app_read_image_headers(uint8_t, uint8_t*, uint32_t );
uint8_t app_set_image_valid_flag(void);
uint8_t app_get_image_id(uint8_t);
uint8_t app_find_old_img(uint8_t, uint8_t);

extern void platform_reset(uint32_t error);
 /**
 ****************************************************************************************
 * SPOTAR Application Functions
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @brief Initializes SPOTAR Application.
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_init( void )
{	
    //spota_state.status_ind_func = app_spotar_status;
    spota_state.spota_pd_idx = 0;
    spota_state.suota_block_idx = 0;
    spota_state.suota_img_idx = 0;
    spota_state.new_patch_len = 0;
    spota_state.crc_clac = 0;
    memset( spota_all_pd, 0x00, sizeof(uint32_t)); // Set first WORD to 0x00
    spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;
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
    spota_state.suota_block_idx = 0;
    spota_state.new_patch_len = 0;
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
    struct spotar_create_db_req * req = KE_MSG_ALLOC(SPOTAR_CREATE_DB_REQ, TASK_SPOTAR,
                                                        TASK_APP, spotar_create_db_req);

    req->features = 0;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief Inializes applacition and enables SPOTAR profile.
 *
 * @param[in]   conhdl  Connection handle
 *
 * @return      void
 ****************************************************************************************
 */
static uint16_t active_conhdl  __attribute__((section("retention_mem_area0"),zero_init));

void app_spotar_enable(uint16_t conhdl)
{
    // Allocate the message
    struct spotar_enable_req * req = KE_MSG_ALLOC(SPOTAR_ENABLE_REQ, TASK_SPOTAR, TASK_APP,
                                                 spotar_enable_req);

    // Fill in the parameter structure
    active_conhdl = conhdl;
    req->conhdl = conhdl;
    req->sec_lvl = get_user_prf_srv_perm(TASK_SPOTAR);
    req->mem_dev = 0; // No Mem device specified yet.
    req->patch_mem_info = 0;  // SPOTA initiator needs to specify mem device first
    
    // Send the message
    ke_msg_send(req);
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
    *mem_info = 0;
    
    uint8_t mem_dev_cmd = (uint8_t) ((mem_dev & SPOTAR_READ_MEM_DEV_TYPE) >> 24);
    
    // If valid memory device, store memory device type and base address
    if( mem_dev_cmd < SPOTAR_MEM_INVAL_DEV )
    {
        spota_state.mem_dev = (uint8_t) ((mem_dev & SPOTAR_READ_MEM_DEV_TYPE) >> 24);
        spota_state.mem_base_add = mem_dev & SPOTAR_READ_MEM_BASE_ADD;

        // Read memory device and set memory info only for internal RAM deviced. External devices need 
        // GPIO configuration first before the memory info can be returned        
        if( (spota_state.mem_dev == SPOTAR_MEM_INT_SYSRAM) || 
            (spota_state.mem_dev == SPOTAR_MEM_INT_RETRAM) )
        {
            // SYSRAM patch base address is the addres of the last index of the spota_all_pd array 
            spota_state.mem_base_add = (uint32_t)(spota_all_pd + (SPOTA_OVERALL_PD_SIZE - sizeof(uint32_t)));
            *mem_info = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), 0);
        } 

        // Store requested image bank
        if( (spota_state.mem_dev == SPOTAR_IMG_I2C_EEPROM) ||
            (spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH) )
        {
            if(spota_state.mem_base_add <= 2)
            {
                spota_state.suota_image_bank = spota_state.mem_base_add;
            }
            else
            {
                // Invalid image bank
                spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_IMG_BANK);
                return;
            }
        }
    }  
    
    switch( mem_dev_cmd )
    {
#if (!SPOTAR_PATCH_DISABLE)        
        case SPOTAR_MEM_INT_SYSRAM:
        case SPOTAR_MEM_INT_RETRAM:
        case SPOTAR_MEM_I2C_EEPROM: 
        case SPOTAR_MEM_SPI_FLASH:
#endif
#if (!SPOTAR_UPDATE_DISABLE)
        case SPOTAR_IMG_I2C_EEPROM: 
        case SPOTAR_IMG_SPI_FLASH:
#endif
            // Initialise SPOTA Receiver app
            app_spotar_start();
            break;

#if (!SPOTAR_UPDATE_DISABLE)        
        case SPOTAR_IMG_INT_SYSRAM:
        case SPOTAR_IMG_INT_RETRAM:
            // No support for SUOTA for SYSRAM/RETRAM
            spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_MEM_TYPE);
            break;
#endif
        
        case SPOTAR_MEM_SERVICE_EXIT:
            app_spotar_stop();
            app_spotar_reset();
            // Initiator requested to exit service. Send notification to initiator
            spotar_send_status_update_req((uint8_t) SPOTAR_SRV_EXIT);
            break;

#if (!SPOTAR_UPDATE_DISABLE)        
        case SPOTAR_IMG_END:
        {
            uint8 ret;
            // Initiator requested to exit service. Calculate CRC if succesfull, send notification to initiator
            if(  spota_state.crc_clac != 0 )
            {
                spotar_send_status_update_req((uint8_t) SPOTAR_CRC_ERR);
            }
            else
            {                
                ret = app_set_image_valid_flag();
                spotar_send_status_update_req((uint8_t) ret);                
            }
            app_spotar_stop();
            app_spotar_reset();
            /// Reset the device if the download image is OK
            /// if (!spota_state.crc_clac)  platform_reset(RESET_AFTER_SPOTA_UPDATE);
            break;
        }
#endif //(!SPOTAR_UPDATE_DISABLE)
        
        case SPOTAR_REBOOT:
        {
            // Reboot is requested
            spota_state.reboot_requested = 1;
        
            // Send disconnect command
            app_easy_gap_disconnect(app_env[0].conidx);

            break;
        }
        
        default:
            spotar_send_status_update_req((uint8_t) SPOTAR_INVAL_MEM_TYPE);
            spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;
            *mem_info = 0;
            break;
    }            
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
    //app_force_active_mode();
    
    EXECUTE_PROFILE_CALLBACK_PARAM (on_spotar_status_change, SPOTAR_START);

    if( SPOTAR_IS_FOR_PATCH( spota_state.mem_dev ) )
    {
        spotar_send_status_update_req((uint8_t) SPOTAR_SRV_STARTED); 
    }
    else
    {
        spotar_send_status_update_req((uint8_t) SPOTAR_IMG_STARTED);    
    }
        
    // Initialise indexes
    spota_state.spota_pd_idx = 0;
    spota_state.suota_block_idx = 0;
    spota_state.suota_img_idx = 0;
    spota_state.new_patch_len = 0;
    spota_state.crc_clac = 0;
    spota_state.crc_clac = 0;
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
#if (!SPOTAR_I2C_DISABLE)
    if( spota_state.mem_dev == SPOTAR_MEM_I2C_EEPROM ){
        i2c_eeprom_release();
    }
#endif
#if (!SPOTAR_SPI_DISABLE)
    if( spota_state.mem_dev == SPOTAR_MEM_SPI_FLASH ) {
        spi_release();
    }
#endif
    // Set memory device to invalid type so that service will not 
    // start until the memory device is explicitly set upon service start
    spota_state.mem_dev = SPOTAR_MEM_INVAL_DEV;
    
    EXECUTE_PROFILE_CALLBACK_PARAM (on_spotar_status_change, SPOTAR_END);
    //app_restore_sleep_mode();
}

/**
 ****************************************************************************************
 * @brief Updates SPOTAR status characteristic.
 *
 * @param[in]   status: SPOTAR application status.
 *
 * @return      void
 ****************************************************************************************
 */
void spotar_send_status_update_req( uint8_t status )
{   

    // Inform SPOTAR task. 
    struct spotar_status_upadet_req *req = KE_MSG_ALLOC(SPOTAR_STATUS_UPDATE_REQ,
                      TASK_SPOTAR, spotar_env.con_info.appid,
                      spotar_status_upadet_req);

    req->conhdl = active_conhdl;
    req->status = status;

    // Send the message
    ke_msg_send(req);
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
void spotar_send_mem_info_update_req( uint32_t mem_info)
{   

    // Inform SPOTAR task. 
    struct spotar_patch_mem_info_upadet_req *req = KE_MSG_ALLOC(SPOTAR_PATCH_MEM_INFO_UPDATE_REQ,
                      TASK_SPOTAR, spotar_env.con_info.appid,
                      spotar_patch_mem_info_upadet_req);

    req->mem_info = mem_info;

    // Send the message
    ke_msg_send(req);
}

#if (!SPOTAR_PATCH_DISABLE) 
/**
 ****************************************************************************************
 * @brief SPOTA data handler. Validates patch and stores patch data to memory device.
 *
 * @param[in]   void
 *
 * @return      void
 *
 ****************************************************************************************
 */
void app_spotar_pd_hdlr(void)
{
    
    uint32_t mem_info;
    uint32_t mem_info_ret;
    uint32_t mem_dev;
    uint16_t overall_len_in_bytes;
    uint16_t newpatch_copy_pos;
    uint16_t status = SPOTAR_CMP_OK;
#if (!SPOTAR_SPI_DISABLE)
    spi_gpio_config_t spi_conf;
#endif
#if (!SPOTAR_I2C_DISABLE)
    i2c_gpio_config_t i2c_conf;
#endif
#if (!SPOTAR_SPI_DISABLE || !SPOTAR_I2C_DISABLE)    
    uint32_t ret;
#endif
    
    // Check mem dev. 
    switch (spota_state.mem_dev)
    {            
        case SPOTAR_MEM_INT_SYSRAM:
        case SPOTAR_MEM_INT_RETRAM:
            mem_dev = (uint32_t)(spota_all_pd + (SPOTA_OVERALL_PD_SIZE - sizeof(uint32_t)));
            mem_info = get_patching_spota_length( ((spota_state.mem_dev << 24) | mem_dev), 0);
            overall_len_in_bytes = (mem_info & SPOTAR_READ_MEM_PATCH_SIZE) << 2;
        
            if( overall_len_in_bytes + spota_state.spota_pd_idx >  SPOTA_OVERALL_PD_SIZE){
                // New patch len + existing patch len is larger than the SYSRAM buffer !!
                status = SPOTAR_INT_MEM_ERR;
                break;
            }
                        
            // Note that in order to keep the same implementation as patching in OTP, the first patch        
            // header starts at the end of the allocated SYSRAM or RETRAM memory bank.
            
            // Counting from the end of the memory bank, write new patch starting at the location 
            // where the existing patch(es) end, plus the size of the new patch.
            // Note that patch data are copied in words.
            {
                uint32_t *ptr_up, *ptr_down;
                
                newpatch_copy_pos = (SPOTA_OVERALL_PD_SIZE - overall_len_in_bytes) - spota_state.spota_pd_idx;
                ptr_down  = (uint32_t *) &spota_new_pd[spota_state.spota_pd_idx - sizeof(uint32_t)];
                ptr_up  = (uint32_t *) &spota_all_pd[newpatch_copy_pos];
                while(spota_state.spota_pd_idx>0){
                        *ptr_up = *ptr_down;                        
                        ptr_up++;
                        ptr_down--;
                        spota_state.spota_pd_idx -= sizeof(uint32_t);
                }
            }
    
            mem_info_ret = get_patching_spota_length( ((spota_state.mem_dev << 24) | mem_dev), 0);
            
            // Check if patch has been written to memory successfully
            if( mem_info_ret == mem_info ){
                status = SPOTAR_INT_MEM_ERR;
                break;
            }
            spotar_send_mem_info_update_req(mem_info_ret);

            if( spota_state.mem_dev == SPOTAR_MEM_INT_SYSRAM )
            {
                // Apply patch if SYSRAM has been selected. Can not reset when patch is stored in SYSRAM.
                if( (mem_info_ret & 0xffff) > 0)
                {
                    //exec_patching_spota((mem_dev & SPOTAR_READ_MEM_BASE_ADD), 0x0,(WORD *) spota_all_pd, (mem_info_ret & SPOTAR_READ_MEM_PATCH_SIZE) );
                }
            }
            break;
        case SPOTAR_MEM_I2C_EEPROM:
#if (!SPOTAR_I2C_DISABLE)            
            mem_info = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), spota_state.gpio_map);
            overall_len_in_bytes = (mem_info & SPOTAR_READ_MEM_PATCH_SIZE) << 2;                                                          
            if( overall_len_in_bytes + spota_state.spota_pd_idx >  SPOTA_OVERALL_PD_SIZE){
                // New patch len + existing patch len is larger than the SYSRAM buffer !!
                status = SPOTAR_INT_MEM_ERR;
                break;
            }
            app_spotar_i2c_config(&i2c_conf);
            i2c_eeprom_init(i2c_conf.slave_addr, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR);            
            i2c_eeprom_write_data(spota_new_pd, (spota_state.mem_base_add + overall_len_in_bytes), spota_state.spota_pd_idx, &ret);
            i2c_wait_until_eeprom_ready();
            mem_info_ret = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), spota_state.gpio_map);
            // Check if patch has been written to memory successfully            
            if( (ret !=  spota_state.spota_pd_idx) || ( mem_info_ret == mem_info) ){
                status = SPOTAR_EXT_MEM_WRITE_ERR;
            }  
            spotar_send_mem_info_update_req(mem_info_ret);
#else
            status = SPOTAR_INVAL_MEM_TYPE; 
#endif // (!SPOTAR_I2C_DISABLE)            
            break;
        case SPOTAR_MEM_SPI_FLASH:
#if (!SPOTAR_SPI_DISABLE)
            mem_info = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), spota_state.gpio_map);
            overall_len_in_bytes = (mem_info & SPOTAR_READ_MEM_PATCH_SIZE) << 2;     
            if( overall_len_in_bytes + spota_state.spota_pd_idx >  SPOTA_OVERALL_PD_SIZE){
                // New patch len + existing patch len is larger than the SYSRAM buffer !!
                status = SPOTAR_INT_MEM_ERR;
                break;
            }
            app_spotar_spi_config(&spi_conf);             
            app_spi_flash_init(&spi_conf.cs);                                
            ret = spi_flash_write_data (spota_new_pd, (spota_state.mem_base_add + overall_len_in_bytes), spota_state.spota_pd_idx);                   
            mem_info_ret = get_patching_spota_length( ((spota_state.mem_dev << 24) | spota_state.mem_base_add), spota_state.gpio_map);
            // Check if patch has been written to memory successfully
            if( (ret !=  spota_state.spota_pd_idx) || ( mem_info_ret == mem_info) ){
                status = SPOTAR_EXT_MEM_WRITE_ERR;
            }             
            spotar_send_mem_info_update_req(mem_info_ret);
#else
            status = SPOTAR_INVAL_MEM_TYPE;
#endif //(!SPOTAR_SPI_DISABLE)            
            break;

        default:
            status = SPOTAR_INVAL_MEM_TYPE;
            break;
    }
             
    if( status == SPOTAR_CMP_OK )
    {
        app_spotar_stop();
        app_spotar_reset();
    }
    
    // SPOTA finished successfully. Send Indication to initiator
    spotar_send_status_update_req((uint8_t) status);
}

/**
 ****************************************************************************************
 * @brief Handles patch execution. Should be called at system start up and after deep sleep.
 *        NOTE!!. This function uses example FLASH and EEPROM base address and GPIO mapping.
 *        The following values need to change to suit the device configuration:
 *          SPOTAR_FLASH_PATCH_BASE_ADDR, SPOTAR_EEPROM_PATCH_BASE_ADDR,
 *          SPOTAR_FLASH_GPIO_MAP, SPOTAR_EEPROM_GPIO_MAP.
 *
 * @param[in]   void
 *
 * @return      void
 *
 ****************************************************************************************
 */
void app_spotar_exec_patch(void)
{
    // Example execute patch function. Executes patch from SPI
    uint32_t  len;
    
    // read patch len from spi flash
    len = get_patching_spota_length(SPOTAR_FLASH_PATCH_BASE_ADDR, SPOTAR_FLASH_GPIO_MAP );
    // read patch len from i2c eeprom
    //len = get_patching_spota_length( SPOTAR_EEPROM_PATCH_BASE_ADDR, SPOTAR_EEPROM_GPIO_MAP );
    
    if( (len & SPOTAR_READ_MEM_PATCH_SIZE) > 0)
    {               
        // execute patch from spi flash
        exec_patching_spota(SPOTAR_FLASH_PATCH_BASE_ADDR, SPOTAR_FLASH_GPIO_MAP ,(WORD *) spota_all_pd , (len & 0xffff) );
        // execute patch from i2c eeprom
        //exec_patching_spota(SPOTAR_EEPROM_PATCH_BASE_ADDR, SPOTAR_EEPROM_GPIO_MAP ,(WORD *) spota_all_pd , (len & 0xffff) );
    }
}
#endif //(!SPOTAR_PATCH_DISABLE)

#if (!SPOTAR_UPDATE_DISABLE)
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
#if (!SPOTAR_SPI_DISABLE)
    spi_gpio_config_t spi_conf;
#endif
#if (!SPOTAR_I2C_DISABLE)      
    i2c_gpio_config_t i2c_conf;
#endif
    
    uint32_t    ret;
    uint32_t     i;
       
    // Update CRC
    for(i=0;i<spota_state.suota_block_idx;i++){       
        spota_state.crc_clac ^= spota_all_pd[i];                                                                        
    }
    
    // Check mem dev. 
    switch (spota_state.mem_dev)
    {                    
        case SPOTAR_IMG_I2C_EEPROM:            
#if (!SPOTAR_I2C_DISABLE)
            app_spotar_i2c_config(&i2c_conf);
            i2c_eeprom_init(i2c_conf.slave_addr, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR); 
        
            // When the first block is received, read image header first
            if( spota_state.suota_block_idx != 0 && spota_state.suota_img_idx == 0 )
            {
                // Read image headers and determine active image.
                ret = app_read_image_headers( spota_state.suota_image_bank, spota_all_pd, spota_state.suota_block_idx );
                if( ret != IMAGE_HEADER_OK ) 
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
                //check file size
                if (( spota_state.suota_image_len+ADDITINAL_CRC_SIZE ) >= ( spota_state.suota_img_idx + spota_state.suota_block_idx ))
                {
                    if (spota_state.suota_image_len < (spota_state.suota_img_idx + spota_state.suota_block_idx))
                        spota_state.suota_block_idx = spota_state.suota_image_len - spota_state.suota_img_idx;
                    
                    i2c_eeprom_write_data(spota_all_pd, (spota_state.mem_base_add + spota_state.suota_img_idx), spota_state.suota_block_idx, &ret);
                    if( ret !=  spota_state.suota_block_idx){
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
#else
            status = SPOTAR_INVAL_MEM_TYPE;
#endif // (!SPOTAR_I2C_DISABLE)          
            break;
        case SPOTAR_IMG_SPI_FLASH:
#if (!SPOTAR_SPI_DISABLE)
            app_spotar_spi_config(&spi_conf);             
            app_spi_flash_init(&spi_conf.cs);   
        
            // When the first block is received, read image header first
            if( spota_state.suota_block_idx != 0 && spota_state.suota_img_idx == 0 )
            {
                // Read image headers and determine active image.
                ret = app_read_image_headers( spota_state.suota_image_bank, spota_all_pd, spota_state.suota_block_idx );
                if( ret != IMAGE_HEADER_OK ) 
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
                //check file size
                if (( spota_state.suota_image_len+ADDITINAL_CRC_SIZE ) >= ( spota_state.suota_img_idx + spota_state.suota_block_idx ))
                {
                    if (spota_state.suota_image_len < (spota_state.suota_img_idx + spota_state.suota_block_idx))
                        spota_state.suota_block_idx = spota_state.suota_image_len - spota_state.suota_img_idx;
                    
                    ret = spi_flash_write_data (spota_all_pd, (spota_state.mem_base_add + spota_state.suota_img_idx), spota_state.suota_block_idx);
                    if( ret !=  spota_state.suota_block_idx){
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
#else
            status = SPOTAR_INVAL_MEM_TYPE;
#endif //(!SPOTAR_SPI_DISABLE)
            break;

        default:
            status = SPOTAR_INVAL_MEM_TYPE;
            break;
    }
    
    // SPOTA finished successfully. Send Indication to initiator
    spotar_send_status_update_req((uint8_t) status);
}
#endif //(!SPOTAR_UPDATE_DISABLE)

/**
 ****************************************************************************************
 * @brief Reserves SPI GPIO pins, reads the GPIO map set by initiator, and configures the SPI pins.
 *
 * @param[in]   spi_conf: Pointer to port/pin pad structure.
 *
 * @return      void.
 *
 ****************************************************************************************
 */
#if (!SPOTAR_SPI_DISABLE)
void app_spotar_spi_config(spi_gpio_config_t *spi_conf)
{
    spi_conf->clk.port      = ((GPIO_PORT)((spota_state.gpio_map & 0x000000f0) >>  4));
    spi_conf->clk.pin       = ((GPIO_PIN) ((spota_state.gpio_map & 0x0000000f)));
    spi_conf->cs.port       = ((GPIO_PORT)((spota_state.gpio_map & 0x0000f000) >> 12));
    spi_conf->cs.pin        = ((GPIO_PIN) ((spota_state.gpio_map & 0x00000f00) >> 8));
    spi_conf->mosi.port     = ((GPIO_PORT)((spota_state.gpio_map & 0x00f00000) >> 20));
    spi_conf->mosi.pin      = ((GPIO_PIN) ((spota_state.gpio_map & 0x000f0000) >> 16));
    spi_conf->miso.port     = ((GPIO_PORT)((spota_state.gpio_map & 0xf0000000) >> 28));
    spi_conf->miso.pin      = ((GPIO_PIN) ((spota_state.gpio_map & 0x0f000000) >> 24));

#if DEVELOPMENT_DEBUG
    RESERVE_GPIO( SPI_CLK, spi_conf->clk.port, spi_conf->clk.pin, PID_SPI_CLK);
    RESERVE_GPIO( SPI_DO, spi_conf->mosi.port, spi_conf->mosi.pin, PID_SPI_DO);
    RESERVE_GPIO( SPI_DI, spi_conf->miso.port, spi_conf->miso.pin, PID_SPI_DI);
    RESERVE_GPIO( SPI_EN, spi_conf->cs.port, spi_conf->cs.pin, PID_SPI_EN);
#endif    
    
    
    GPIO_ConfigurePin( spi_conf->cs.port, spi_conf->cs.pin, OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( spi_conf->clk.port, spi_conf->clk.pin, OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( spi_conf->mosi.port, spi_conf->mosi.pin, OUTPUT, PID_SPI_DO, false );	
    GPIO_ConfigurePin( spi_conf->miso.port, spi_conf->miso.pin, INPUT, PID_SPI_DI, false );
}

/**
 ****************************************************************************************
 * @brief SPI and SPI flash Initialization fucntion
 *
 * @param[in]   cs_pad_param: Pointer to chip select port/pin pad structure.
 *
 * @return      void.
 ****************************************************************************************
 */
void app_spi_flash_init(SPI_Pad_t *cs_pad_param)
{
    uint16_t man_dev_id = 0;
    uint32_t flash_size = SPI_FLASH_DEFAULT_SIZE; // Default memory size in bytes
    uint32_t flash_page = SPI_FLASH_DEFAULT_PAGE; // Default memory page size in bytes


    // Enable SPI & SPI FLASH
    spi_init(cs_pad_param, SPI_MODE_8BIT, SPI_ROLE_MASTER, SPI_CLK_IDLE_POL_LOW,SPI_PHA_MODE_0, SPI_MINT_DISABLE, SPI_XTAL_DIV_8);

    man_dev_id = spi_read_flash_memory_man_and_dev_id();
    switch(man_dev_id)
    {
        case W25X10_MAN_DEV_ID:
            flash_size = W25X10_TOTAL_FLASH_SIZE;
            flash_page = W25X10_PAGE_SIZE;
            break; 
        case W25X20_MAN_DEV_ID:
            flash_size = W25X20_TOTAL_FLASH_SIZE;
            flash_page = W25X20_PAGE_SIZE;
            break; 
        case MX25V1006E_MAN_DEV_ID:
            flash_size = MX25V1006E_TOTAL_FLASH_SIZE;
            flash_page = MX25V1006E_PAGE_SIZE;
            break;
        default:
            flash_size = SPI_FLASH_DEFAULT_SIZE; 
            flash_page = SPI_FLASH_DEFAULT_PAGE; 
            break; 
        
    }
    spi_flash_init(flash_size, flash_page);
}
#endif //(!SPOTAR_SPI_DISABLE)

#if (!SPOTAR_I2C_DISABLE)
/**
 ****************************************************************************************
 * @brief Reserves I2C GPIO pins, reads the GPIO map set by initiator, and configures the the I2C pins.
 *
 * @param[in]   i2c_conf:  pointer to port/pin pad structure.
 *
 * @return      void
 *
 ****************************************************************************************
 */
void app_spotar_i2c_config(i2c_gpio_config_t *i2c_conf)
{
    i2c_conf->sda.port      = ((GPIO_PORT)((spota_state.gpio_map & 0x000000f0) >>  4));
    i2c_conf->sda.pin       = ((GPIO_PIN) ((spota_state.gpio_map & 0x0000000f)));
    i2c_conf->scl.port      = ((GPIO_PORT)((spota_state.gpio_map & 0x0000f000) >> 12));
    i2c_conf->scl.pin       = ((GPIO_PIN) ((spota_state.gpio_map & 0x00000f00) >> 8));
    i2c_conf->slave_addr    = ((spota_state.gpio_map & 0xffff0000) >> 16);
#if DEVELOPMENT_DEBUG
#if (!BLE_APP_KEYBOARD) 
    RESERVE_GPIO( I2C_SCL, i2c_conf->scl.port, i2c_conf->scl.pin, PID_I2C_SCL);
    RESERVE_GPIO( I2C_SDA, i2c_conf->sda.port, i2c_conf->sda.pin, PID_I2C_SDA);
#endif
#endif
    
    GPIO_ConfigurePin(i2c_conf->scl.port, i2c_conf->scl.pin, INPUT, PID_I2C_SCL, false);
    GPIO_ConfigurePin(i2c_conf->sda.port, i2c_conf->sda.pin, INPUT, PID_I2C_SDA, false);
}
#endif //(!SPOTAR_I2C_DISABLE)

#if (!SPOTAR_UPDATE_DISABLE)
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
    product_header_t *pProductHeader;
    image_header_t *pfwHeader;
    uint32 codesize;
    int32_t ret;
    uint8_t mem_data_buff[64];
    uint32_t imageposition1;
    uint32_t imageposition2;
    uint8 new_bank = ANY_IMAGE_BANK;
    uint8 is_invalid_image1 = IMAGE_HEADER_OK;
    uint8 is_invalid_image2 = IMAGE_HEADER_OK;
    uint8 imageid  = IMAGE_ID_0;
    uint8 imageid1 = IMAGE_ID_0;
    uint8 imageid2 = IMAGE_ID_0;
    
    
    if( data_len < sizeof(image_header_t) ) 
    {
        // block size should be at least image header size
        return SPOTAR_INVAL_IMG_HDR;  
    }
    else
    {
        // read header form first data block
        pfwHeader = (image_header_t*)data;
    }
           
    // check firmware header 
    if( (pfwHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) || (pfwHeader->signature[1] != IMAGE_HEADER_SIGNATURE2) )
    {
        return SPOTAR_INVAL_IMG_HDR;
    }
    
    // Get image size
    codesize = pfwHeader->code_size;
    spota_state.suota_image_len = pfwHeader->code_size+sizeof(image_header_t); 
    // read product header
    pProductHeader = (product_header_t*) mem_data_buff;
    
    ret = app_read_ext_mem( (uint8_t*)mem_data_buff, (unsigned long)PRODUCT_HEADER_POSITION, (unsigned long)sizeof(product_header_t) );
    if( ret != sizeof(product_header_t) ) return SPOTAR_EXT_MEM_READ_ERR;
        
    // verify product header
    if( (pProductHeader->signature[0] != PRODUCT_HEADER_SIGNATURE1) || 
        (pProductHeader->signature[1] != PRODUCT_HEADER_SIGNATURE2)    )
    {
        return SPOTAR_INVAL_PRODUCT_HDR;
    }
    
    // Store the start addresses of both images positions
    imageposition1 = pProductHeader->offset1;
    imageposition2 = pProductHeader->offset2;
    
    // verify image positions
    // the code size must be less than the space between images 
    if (!codesize || codesize > (imageposition2 - imageposition1) )
    {
        return SPOTAR_INVAL_IMG_SIZE;
    }

    pImageHeader = (image_header_t*)mem_data_buff;
    
    // read first image header, image id must be stored for the bank selection
    ret = app_read_ext_mem( (uint8_t*)pImageHeader, (unsigned long)imageposition1, (unsigned long)sizeof(image_header_t) );
    if( ret != sizeof(image_header_t) ) return SPOTAR_EXT_MEM_READ_ERR;

    imageid1 = pImageHeader->imageid;
    if( (pImageHeader->validflag != STATUS_VALID_IMAGE)         ||
        (pImageHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) ||
        (pImageHeader->signature[1] != IMAGE_HEADER_SIGNATURE2)    )
    {
        is_invalid_image1 = IMAGE_HEADER_INVALID;
        imageid1 = IMAGE_ID_0;
    } else {
        //compate version string and timestamp
        if (!memcmp(pfwHeader->version, pImageHeader->version, IMAGE_HEADER_VERSION_SIZE) && (pfwHeader->timestamp==pImageHeader->timestamp))
        {
            is_invalid_image1 = IMAGE_HEADER_SAME_VERSION;
        }
    }

    // read second image header, image id must be stored for the bank selection
    ret = app_read_ext_mem((uint8_t*)pImageHeader, (unsigned long)imageposition2, (unsigned long)sizeof(image_header_t));                
    if( ret != sizeof(image_header_t) ) return SPOTAR_EXT_MEM_READ_ERR;

    imageid2=pImageHeader->imageid;
    if( (pImageHeader->validflag != STATUS_VALID_IMAGE)         ||
        (pImageHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) ||
        (pImageHeader->signature[1] != IMAGE_HEADER_SIGNATURE2)    )
    {
        is_invalid_image2 = IMAGE_HEADER_INVALID;
        imageid2 = IMAGE_ID_0;
    } else {
        //compate version string and timestamp
        if (!memcmp(pfwHeader->version, pImageHeader->version, IMAGE_HEADER_VERSION_SIZE) && (pfwHeader->timestamp==pImageHeader->timestamp))
        {
            is_invalid_image2 = IMAGE_HEADER_SAME_VERSION;
        }
    }

    if (image_bank == ANY_IMAGE_BANK ||  image_bank > SECOND_IMAGE_BANK)
    {
        if (is_invalid_image1 == IMAGE_HEADER_INVALID) new_bank = FISRT_IMAGE_BANK;
        else if (is_invalid_image2 == IMAGE_HEADER_INVALID) new_bank = SECOND_IMAGE_BANK;
        else new_bank = app_find_old_img(imageid1, imageid2);
    }
    else
    {
        new_bank = image_bank;
    }
 
    memset(mem_data_buff, 0xFF, sizeof(mem_data_buff));
    if (new_bank == SECOND_IMAGE_BANK)
    {
        if (is_invalid_image2==IMAGE_HEADER_SAME_VERSION) return SPOTAR_SAME_IMG_ERR;
        spota_state.mem_base_add = imageposition2;

        if (is_invalid_image1 == IMAGE_HEADER_OK || is_invalid_image1 == IMAGE_HEADER_SAME_VERSION)
            imageid = app_get_image_id(imageid1);
        else
            imageid = IMAGE_ID_1;
    }
    else
    {
        if (is_invalid_image1==IMAGE_HEADER_SAME_VERSION) return SPOTAR_SAME_IMG_ERR;
        spota_state.mem_base_add = imageposition1;
        
        if (is_invalid_image2 == IMAGE_HEADER_OK || is_invalid_image2 == IMAGE_HEADER_SAME_VERSION)
            imageid = app_get_image_id(imageid2);
        else
            imageid = IMAGE_ID_1;
    }

    // Erase header and image 
    if( spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH )
    {
#if (!SPOTAR_SPI_DISABLE)
        ret = app_erase_flash_sectors(spota_state.mem_base_add, codesize + CODE_OFFSET);
        if( ret != ERR_OK) return SPOTAR_EXT_MEM_WRITE_ERR;
#else
        return SPOTAR_EXT_MEM_WRITE_ERR;
#endif        
    }
    else
    {
#if (!SPOTAR_I2C_DISABLE)
        uint32_t ret_i2c;
        // For i2c eeprom, no need to delete entire image, just write 0xff to the header to invalidate image
        if (i2c_eeprom_write_data(mem_data_buff, spota_state.mem_base_add, sizeof(mem_data_buff), &ret_i2c) != I2C_NO_ERROR)
        {
            ret = -1;
        }
        if( ret_i2c !=  CODE_OFFSET ) return SPOTAR_EXT_MEM_WRITE_ERR;
#else
        return SPOTAR_EXT_MEM_WRITE_ERR;
#endif        
    }
        
    //write header apart from validflag. set validflag when the entire image has been received
    pImageHeader->imageid = imageid;
    pImageHeader->code_size= pfwHeader->code_size; 
    pImageHeader->CRC=pfwHeader->CRC;  
    memcpy(pImageHeader->version,pfwHeader->version, IMAGE_HEADER_VERSION_SIZE);  
    pImageHeader->timestamp=pfwHeader->timestamp; 
    pImageHeader->signature[0]=pfwHeader->signature[0]; 
    pImageHeader->signature[1]=pfwHeader->signature[1]; 
    pImageHeader->encryption = pfwHeader->encryption;
    
    // write image and header
    if( spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH )
    {
#if (!SPOTAR_SPI_DISABLE)        
        ret = spi_flash_write_data((uint8_t*) &data[CODE_OFFSET], spota_state.mem_base_add + CODE_OFFSET, data_len - CODE_OFFSET);
        if( ret != (data_len - CODE_OFFSET) ) return SPOTAR_EXT_MEM_WRITE_ERR;
       
        // write header
        ret = spi_flash_write_data((uint8_t*)pImageHeader, spota_state.mem_base_add, sizeof(image_header_t));
        if( ret != sizeof(image_header_t) ) return SPOTAR_EXT_MEM_WRITE_ERR;
#else
        return SPOTAR_EXT_MEM_WRITE_ERR; 
#endif
    }
    else
    {
#if (!SPOTAR_I2C_DISABLE)
        uint32_t ret_i2c;
        if (i2c_eeprom_write_data((uint8_t*) &data[CODE_OFFSET], spota_state.mem_base_add + CODE_OFFSET, data_len - CODE_OFFSET, &ret_i2c) != I2C_NO_ERROR)
        {
            ret = -1;
        }
        if( ret_i2c != (data_len - CODE_OFFSET) ) return SPOTAR_EXT_MEM_WRITE_ERR;
       
        // write header
        if (i2c_eeprom_write_data((uint8_t*)pImageHeader, spota_state.mem_base_add, sizeof(image_header_t), &ret_i2c) != I2C_NO_ERROR)
        {
            ret = -1;
        }
        if( ret_i2c != sizeof(image_header_t) ) return SPOTAR_EXT_MEM_WRITE_ERR;
#else
        return SPOTAR_EXT_MEM_WRITE_ERR;
#endif        
    }
        
    return IMAGE_HEADER_OK;
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
    int8_t ret = SPOTAR_EXT_MEM_WRITE_ERR;
    uint8_t flag = STATUS_VALID_IMAGE;
     
    if( spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH )
    {
#if (!SPOTAR_SPI_DISABLE)
        spi_gpio_config_t spi_conf;
        app_spotar_spi_config(&spi_conf);
        app_spi_flash_init(&spi_conf.cs);
        ret = spi_flash_write_data((uint8_t*) &flag, spota_state.mem_base_add + 2, sizeof(uint8_t));		
        if (ret == sizeof(uint8_t)) 
            ret = SPOTAR_CMP_OK;
#endif
    }
    else
    {
#if (!SPOTAR_I2C_DISABLE)        
        i2c_gpio_config_t i2c_conf;
        app_spotar_i2c_config(&i2c_conf);
        i2c_eeprom_init(i2c_conf.slave_addr, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR);
        i2c_eeprom_write_byte( (uint32_t) (spota_state.mem_base_add + 2), flag );
        ret = SPOTAR_CMP_OK;
#endif
    }
    
    return ret;    
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
	uint8_t new_imgid = IMAGE_ID_0;
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
	if (id1 == 0xFF && id2 == 0xFF) return IMAGE_ID_1;
	if (id1 == 0xFF && id2 == 0) return IMAGE_ID_1;
	if (id2 == 0xFF && id1 == 0) return IMAGE_ID_2;
	if (id1 > id2) return IMAGE_ID_2;
	else return 1;
}
#endif //(!SPOTAR_UPDATE_DISABLE)

/**
 ****************************************************************************************
 * @brief This function is called to erase the SPI sectors before writing the new image 
 *
 * @param[in]   starting_address:  Starting address to start erasing
 *
 * @return      ERR_OK on success
 *              
 ****************************************************************************************
 */
#if (!SPOTAR_SPI_DISABLE)
int8_t app_erase_flash_sectors(uint32_t starting_address, uint16_t size)
{
	int i;
	uint32_t starting_sector;
	uint32_t sector;
	int8_t ret = -1;
 
	starting_sector = (starting_address/SPI_SECTOR_SIZE) * SPI_SECTOR_SIZE;
	
	sector=(size / SPI_SECTOR_SIZE);
	if (size % SPI_SECTOR_SIZE) 
		sector++;
	if (!sector) sector++;
	for (i=0; i < sector; i++)
	{
 	 	ret = spi_flash_block_erase(starting_sector, SECTOR_ERASE);
		starting_sector += SPI_SECTOR_SIZE;
		if (ret != ERR_OK) break;
	}
	return ret;
}
#endif

#if (!SPOTAR_UPDATE_DISABLE)
/**
 ****************************************************************************************
 * @brief Read data from a given starting address 
 *
 * @param[in] *rd_data_ptr:  Points to the position the read data will be stored
 * @param[in] address:       Starting address of data to be read
 * @param[in] size:          Size of the data to be read
 * 
 * @return  Number of read bytes or error code
 ****************************************************************************************
 */
int app_read_ext_mem( uint8_t *rd_data_ptr, uint32_t address, uint32_t size )
{
    int ret = -1;
    
    if( spota_state.mem_dev == SPOTAR_IMG_SPI_FLASH )
    {
#if (!SPOTAR_SPI_DISABLE)
        ret = spi_flash_read_data( (uint8_t*)rd_data_ptr, (unsigned long)address, (unsigned long)size);
#endif
    }
    else
    {
#if (!SPOTAR_I2C_DISABLE)
        uint32_t ret_i2c;
        if (i2c_eeprom_read_data((uint8_t*)rd_data_ptr, (uint32_t)address, (uint32_t)size, &ret_i2c) != I2C_NO_ERROR)
        {
            ret = -1;
        }
        else
        {
            ret = ret_i2c;
        }
#endif
    }
    return ret;
}
#endif //(!SPOTAR_UPDATE_DISABLE)

#endif //BLE_SPOTA_RECEIVER

/// @} APP
