/**
 ****************************************************************************************
 *
 * @file app_spotar.h
 *
 * @brief Header file for application handlers for spotar ble events and responses.
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

#ifndef APP_SPOTAR_H_
#define APP_SPOTAR_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ke_task.h"         // kernel task
#include "ke_msg.h"          // kernel message
#include "gapc_task.h"
#include "spotar.h"
#include "spotar_task.h"
#include "app.h"
#include "app_task.h"
#include <stdint.h>          // standard integer
#include <stdio.h>			 // standard I/O
#include <stdlib.h>			 // standard lib

/*
 * DEFINES
 ****************************************************************************************
 */
// Upgrade files
#define UPGRADE_IMAGE1					"upgrade_image1.img"
#define UPGRADE_IMAGE2					"upgrade_image2.img"

// SPOTAR application types
#define SPOTAR_PATCH					0   // Software patch over the air (SPOTA)
#define SPOTAR_IMAGE					1   // Software image update over the air (SUOTA)

// Checks the application type (SPOTA or SUOTA) 
#define SPOTAR_IS_FOR_PATCH(mem_dev)	((((mem_dev) & 0x10) >> 4) == SPOTAR_PATCH)
#define SPOTAR_IS_FOR_IMAGE(mem_dev)	((((mem_dev) & 0x10) >> 4) == SPOTAR_IMAGE)

#define SPOTAR_IS_VALID_MEM_DEV(mem_dev) ((mem_dev) < SPOTAR_MEM_INVAL_DEV)

// FLAGS
#define SPOTAR_READ_MEM_DEV_TYPE		0xFF000000
#define SPOTAR_READ_MEM_BASE_ADD		0x0000FFFF
#define SPOTAR_READ_NUM_OF_PATCHES		0xFFFF0000
#define SPOTAR_READ_MEM_PATCH_SIZE		0x0000FFFF

// SUOTAR defines for image manipulation
#define PRODUCT_HEADER_POSITION			0x1F000
#define PRODUCT_HEADER_SIGNATURE1		0x70
#define PRODUCT_HEADER_SIGNATURE2		0x52
#define IMAGE_HEADER_SIGNATURE1			0x70
#define IMAGE_HEADER_SIGNATURE2			0x51
#define CODE_OFFSET						64
#define ADDITINAL_CRC_SIZE				1
#define STATUS_INVALID_IMAGE			0x0
#define STATUS_VALID_IMAGE				0xAA
#define IMAGE_HEADER_OK					0

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
// Product header for SUOTA 
typedef struct 
{
    uint8_t		signature[2];
    uint8_t		version[2];
    uint32_t	offset1;				// Start address of first image header
    uint32_t	offset2;				// Start address of second image header
}product_header_t;

// Image header for SUOTA 
typedef struct 
{
    uint8_t		signature[2];
    uint8_t		validflag;				// Set to STATUS_VALID_IMAGE at the end of the image update
    uint8_t		imageid;				// it is used to determine which image is the newest
    uint32_t	code_size;				// Image size
    uint32_t	CRC;					// Image CRC
    uint8_t		version[16];			// Vertion of the image
    uint32_t	timestamp;
    uint8_t		reserved[32];
}image_header_t;

// Holds the retainable veriables of SPOTAR app
typedef struct
{
    uint8_t     mem_dev;			// Memory device to upgrade.
    uint32_t    mem_base_add;		// Always 0x00 for this file upgrade example.
	FILE*		upgrade_file;		// File pointer of the file to be upgraded
    uint32_t    gpio_map;			// NOT USED in SUOTAR
    uint32_t    new_patch_len;		// The actual block size
    uint8_t     spota_pd_idx;		// NOT USED in SUOTAR
    uint8_t     suota_image_bank;	// Image bank to upgrade. 0x00: oldest, 0x01: Image 1, 0x02: Image 2
    uint32_t    suota_block_idx;	// The chuck index of the block. Has size 0 to new_patch_len
    uint32_t    suota_img_idx;		// Multiple of block size
    uint8_t     crc_clac;			// CRC calculation
    uint32_t    suota_image_len;	// Total image length
}app_spota_state;
app_spota_state spota_state;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

// SPOTAR/SUOTAR Physical memory device selection and commands 
enum
{
    // SPOTA is used to send patches
    SPOTAR_MEM_INT_SYSRAM = 0x00,
    SPOTAR_MEM_INT_RETRAM = 0x01,
    SPOTAR_MEM_I2C_EEPROM = 0x02, 
    SPOTAR_MEM_SPI_FLASH  = 0x03,
    
    // SUOTA is used to send entire image
    SPOTAR_IMG_INT_SYSRAM = 0x10,
    SPOTAR_IMG_INT_RETRAM = 0x11,
    SPOTAR_IMG_I2C_EEPROM = 0x12, 
    SPOTAR_IMG_SPI_FLASH  = 0x13,
    
    SPOTAR_MEM_INVAL_DEV  = 0x14, // DO NOT move. Must be before commands 
    
    /* SPOTAR/SUOTAR commands
    */
    SPOTAR_REBOOT         = 0xFD,
    SPOTAR_IMG_END        = 0xFE,
    // When initiator selects 0xff, it wants to exit SPOTAR service.
    // This is used in case of unexplained failures. If SPOTAR process 
    // finishes correctly it will exit automatically.
    SPOTAR_MEM_SERVICE_EXIT   = 0xFF,    
};

uint8_t spota_new_pd[SPOTA_NEW_PD_SIZE];
uint8_t spota_all_pd[SPOTA_OVERALL_PD_SIZE];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initializes SPOTAR Apllication.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_init(void);

/**
 ****************************************************************************************
 * @brief Send enable request for the SPOTAR profile.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_enable(void);

/**
 ****************************************************************************************
 * @brief Creates SPOTAR service database.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_create_db(void);

/**
 ****************************************************************************************
 * @brief Updates SPOTAR memory info characteristic.
 *
 * @param[in]   mem_info: Patch memory info. Number of patches and overall patch length.
 *
 * @return      void.
 ****************************************************************************************
 */
void spotar_send_mem_info_update_req(uint32_t mem_info);

/**
 ****************************************************************************************
 * @brief Updates SPOTAR status characteristic.
 *
 * @param[in]   SPOTAR application status.
 *
 * @return      void
 ****************************************************************************************
 */
void spotar_send_status_update_req(uint8_t status);

/**
 ****************************************************************************************
 * @brief Stops SPOTAR service and resets application
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_stop(void);

/**
 ****************************************************************************************
 * @brief Starts SPOTAR serivce and disables sleep.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_start(void);

/**
 ****************************************************************************************
 * @brief Resets SPOTAR Apllication.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_reset(void);

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
uint8_t app_set_image_valid_flag(void);

/**
 ****************************************************************************************
 * @brief Handles the SPOTAR_CREATE_DB_CFM message.
 *
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GTL).
 *
 * @return				If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_create_db_cfm_handler(ke_task_id_t const dest_id, struct spotar_create_db_cfm *param);

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
void app_spotar_read_mem(uint32_t mem_dev, uint32_t* mem_info);

/**
 ****************************************************************************************
 * @brief Read data from a given starting address 
 *
 * @param[in] *rd_data_ptr:	Points to the position the read data will be stored
 * @param[in] address:		Starting address of data to be read
 * @param[in] size:			Size of the data to be read
 * 
 * @return					Number of read bytes or error code
 ****************************************************************************************
 */
int app_read_ext_mem(uint8_t *rd_data_ptr, FILE *fp, uint32_t address, uint32_t size);

/**
 ****************************************************************************************
 * @brief Write data from a given starting address 
 *
 * @param[in] *rd_data_ptr:	Points to the position the read data will be stored
 * @param[in] fp:			File pointer of the image file
 * @param[in] size:         Size of the data to be read
 * 
 * @return					Number of read bytes or error code
 ****************************************************************************************
 */
int app_write_ext_mem(uint8_t *rd_data_ptr, FILE *fp, uint32_t address, uint32_t size);

/**
 ****************************************************************************************
 * @brief Handles memory device indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return				If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_mem_dev_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_mem_dev_ind *param);

/**
 ****************************************************************************************
 * @brief Handles Patch Length indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return				If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_len_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_len_ind const *param);

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
void app_spotar_img_hdlr(void);

/**
 ****************************************************************************************
 * @brief This function is called when the first SUOTA block is received.
 *        Firstly, the image header is extracted from the first block, then the external memmory
 *        is checked to determine where to write this new image and finally the header and the first
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
int app_read_image_headers(uint8_t image_bank, uint8_t* data, uint32_t data_len);

/**
 ****************************************************************************************
 * @brief Handles patch data indication from SPOTA receiver profile
 *
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] param     Pointer to the parameters of the message.
 *
 * @return				If the message was consumed or not.
 ****************************************************************************************
 */
int spotar_patch_data_ind_handler(ke_task_id_t const dest_id, struct spotar_patch_data_ind const *param);

/**
 ****************************************************************************************
 * @brief This function is called to find the older image of the two in the external memmory 
 *
 * @param[in]	imgid: current ids of the two images
 *
 * @return      older image id 
 *              
 ****************************************************************************************
 */
uint8_t app_find_old_img(uint8_t id1, uint8_t id2);

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
uint8_t app_get_image_id(uint8_t imgid);

/**
 ****************************************************************************************
 * @brief This function is called to open a disk file to save the upgrade image.
 *			It simulates a memory device in a Host platform.
 *
 * @param[in]   filename: The filename to be opened
 *
 * @return      The file pointer of the open file. NULL if error occurs.
 *              
 ****************************************************************************************
 */
FILE *app_open_image_file(char *filename);

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
void app_close_image_file(FILE *fp);

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
void app_print_image_data(void);

/**
 ****************************************************************************************
 * @brief Prints the local time in ms accuracy
 *
 * @param[in]   void
 *
 * @return		A string array containing the time.
 *              
 ****************************************************************************************
 */
char *app_print_time(void);

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
void app_init_image_file(void);

                            
/// @} APPPROXR

#endif // APP_PROXR_H_
