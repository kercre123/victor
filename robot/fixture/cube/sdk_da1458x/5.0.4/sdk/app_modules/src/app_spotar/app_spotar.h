/**
 ****************************************************************************************
 *
 * @file app_spotar.h
 *
 * @brief SPOTA Receiver Application header file
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

#ifndef APP_SPOTAR_H_
#define APP_SPOTAR_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief SPOTA Receiver Application header file.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_profiles_config.h"

#if BLE_SPOTA_RECEIVER

#include <stdint.h>          // standard integer definition
#include <co_bt.h>
#if !defined (CFG_SPOTAR_SPI_DISABLE)
#include "spi.h"
#endif
#include "gpio.h"
#include "spotar.h"
#include "spotar_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

#if defined (CFG_SPOTAR_SPI_DISABLE)
#define SPOTAR_SPI_DISABLE      1
#else
#define SPOTAR_SPI_DISABLE      0
#endif

#if defined (CFG_SPOTAR_I2C_DISABLE)
#define SPOTAR_I2C_DISABLE      1
#else
#define SPOTAR_I2C_DISABLE      0
#endif

#if defined (CFG_SPOTAR_UPDATE_DISABLE)
#define SPOTAR_UPDATE_DISABLE   1
#else
#define SPOTAR_UPDATE_DISABLE   0
#endif

#if defined (CFG_SPOTAR_PATCH_DISABLE)
#define SPOTAR_PATCH_DISABLE   1
#else
#define SPOTAR_PATCH_DISABLE   0
#endif
 
#if ( SPOTAR_PATCH_DISABLE && SPOTAR_UPDATE_DISABLE )
    #error "SPOTAR application is enabled but both SPOTA and SUOTA are disabled"
#endif
#if ( SPOTAR_SPI_DISABLE && SPOTAR_I2C_DISABLE && !SPOTAR_UPDATE_DISABLE )
    #error "SUOTA application is enabled but both I2C and SPI are disabled"
#endif


// SPOTAR status indications
#define SPOTAR_START    1
#define SPOTAR_END      0
 
 // SPOTAR application types
#define SPOTAR_PATCH            0   // Software patch over the air (SPOTA)
#define SPOTAR_IMAGE            1   // Software image update over the air (SUOTA)

// Checks the application type (SPOTA or SUOTA) 
#define SPOTAR_IS_FOR_PATCH(mem_dev) ( (((mem_dev) & 0x10) >> 4 ) == SPOTAR_PATCH)
#define SPOTAR_IS_FOR_IMAGE(mem_dev) ( (((mem_dev) & 0x10) >> 4 ) == SPOTAR_IMAGE)

#define SPOTAR_IS_VALID_MEM_DEV(mem_dev) ( (mem_dev) < SPOTAR_MEM_INVAL_DEV )

// FLAGS
#define SPOTAR_READ_MEM_DEV_TYPE    0xFF000000
#define SPOTAR_READ_MEM_BASE_ADD    0x0000FFFF
#define SPOTAR_READ_NUM_OF_PATCHES  0xFFFF0000
#define SPOTAR_READ_MEM_PATCH_SIZE  0x0000FFFF

// Example FLASH/EEPROM base address and GPIO mapping
#define  SPOTAR_FLASH_PATCH_BASE_ADDR   0x03002800
#define  SPOTAR_FLASH_GPIO_MAP          0x05060300
#define  SPOTAR_EEPROM_PATCH_BASE_ADDR  0x02000800
#define  SPOTAR_EEPROM_GPIO_MAP         0x00500203

// SUOTAR defines for image manipulation
#define PRODUCT_HEADER_POSITION     0x1F000
#define PRODUCT_HEADER_SIGNATURE1   0x70
#define PRODUCT_HEADER_SIGNATURE2   0x52
#define IMAGE_HEADER_SIGNATURE1     0x70
#define IMAGE_HEADER_SIGNATURE2     0x51
#define CODE_OFFSET                 64
#define ADDITINAL_CRC_SIZE          1
#define IMAGE_HEADER_VERSION_SIZE   16
#define STATUS_VALID_IMAGE          0xAA
#define IMAGE_HEADER_OK             0
#define IMAGE_HEADER_INVALID        1
#define IMAGE_HEADER_SAME_VERSION   2
#define ANY_IMAGE_BANK              0
#define FISRT_IMAGE_BANK            1
#define SECOND_IMAGE_BANK           2
#define IMAGE_ID_0                  0
#define IMAGE_ID_1                  1
#define IMAGE_ID_2                  2

// Holds the retainable veriables of SPOTAR app
typedef struct
{
    uint8_t     mem_dev;
    uint32_t    mem_base_add;
    uint32_t    gpio_map;
    uint32_t    new_patch_len;
    uint8_t     spota_pd_idx;
    uint8_t     suota_image_bank;
    uint32_t    suota_block_idx;
    uint32_t    suota_img_idx;
    uint8_t     crc_clac;
    uint32_t    suota_image_len;
    void (*status_ind_func) (const uint8_t);
    uint8_t     reboot_requested;
}app_spota_state;
 

#if (!SPOTAR_SPI_DISABLE) 
// Defines the SPI GPIO type
typedef struct
{
    SPI_Pad_t cs;
    SPI_Pad_t mosi;
    SPI_Pad_t miso;
    SPI_Pad_t clk;
}spi_gpio_config_t;
#endif


typedef struct
{
	GPIO_PORT port;
	GPIO_PIN pin;
}I2C_Pad_t;

// Defines the i2c GPIO type
typedef struct
{
    I2C_Pad_t scl;
    I2C_Pad_t sda;
    uint32_t  slave_addr;    
}i2c_gpio_config_t;

// Product header for SUOTA 
typedef struct 
{
    uint8_t signature[2];
    uint8_t version[2];
    uint32_t offset1; // Start address of first image header
    uint32_t offset2; // Start address of second image header
}product_header_t;

// Image header for SUOTA 
typedef struct 
{
    uint8_t signature[2];
    uint8_t validflag;      // Set to STATUS_VALID_IMAGE at the end of the image update
    uint8_t imageid;        // it is used to determine which image is the newest
    uint32_t code_size;     // Image size
    uint32_t CRC ;          // Image CRC
    uint8_t version[16];    // Vertion of the image
    uint32_t timestamp;
    uint8_t encryption;
    uint8_t reserved[31];
}image_header_t;

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

/*
 * EXTERNAL DEFINITIONS
 ****************************************************************************************
 */

extern app_spota_state spota_state;
extern uint8_t spota_new_pd[SPOTA_NEW_PD_SIZE];
extern uint8_t spota_all_pd[SPOTA_OVERALL_PD_SIZE];


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initializes SPOTAR Apllication.
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_init( void );

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
 * @brief Inializes applacition and enables SPOTAR profile.
 *
 * @param[in]   void
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Reads memory device and writes memory info.
 *
 * @param[in]   MSbyte holds the Memory device type, rest is the base address.
 * @param[in]   16MSbits hold number of patches, 16LSbits hold overall mem len.
 *
 * @return      void
 ****************************************************************************************
 */
void app_spotar_read_mem(uint32_t mem_dev_type, uint32_t* mem_info);

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
 * @brief Handles patch execution. Should be called at system start up and after deep sleep.
 *
 * @param[in]   void
 *
 * @return      void
 *
 ****************************************************************************************
 */
void app_spotar_exec_patch(void);

/**
 ****************************************************************************************
 * @brief Updates SPOTAR status characteristic.
 *
 * @param[in]   SPOTAR application status.
 *
 * @return      void
 ****************************************************************************************
 */
void spotar_send_status_update_req( uint8_t status);

/**
 ****************************************************************************************
 * @brief Updates SPOTAR memory info characteristic.
 *
 * @param[in]   Patch memory info. Number of patches and overall patch length.
 *
 * @return      void.
 ****************************************************************************************
 */
void spotar_send_mem_info_update_req( uint32_t mem_info);

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
void app_spotar_pd_hdlr(void);
#endif // (!SPOTAR_PATCH_DISABLE)

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
void app_spotar_img_hdlr(void);
#endif //(!SPOTAR_UPDATE_DISABLE)

/*
 * ROM FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles patch execution
 *
 * @param[in]   defines the physical memory and patch base address
 * @param[in]   defines the mapping of the interface signals to IO pins
 * @param[in]   Points to the intermediate buffer address which will be used to temporary store the
 *              patch payload fetched from external Non Volatile memories. Only valid if I2C or SPI
 * @param[in]   patch_length: defines the size of the patch to be applied (in 32bit words)
 *
 * @return      void
 *
 ****************************************************************************************
 */
void exec_patching_spota(WORD mem_dev, WORD gpio_map,WORD* ptr,WORD patch_length);

/**
 ****************************************************************************************
 * @brief Returns the stored number of patches and the overall patch length 
 *
 * @param[in]   defines the physical memory and patch base address
 * @param[in]   defines the mapping of the interface signals to IO pins
 *
 * @return      The number and size (in 32-bit words) of the exising patch area 
 *
 ****************************************************************************************
 */
uint32_t get_patching_spota_length(uint32_t mem_dev, uint32_t gpio_map);

#endif //BLE_SPOTA_RECEIVER

/// @} APP

#endif // APP_H_
