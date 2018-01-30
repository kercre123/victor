/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Header file for Main application.
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

#ifndef APP_H_
#define APP_H_

#include "co_bt.h"
#include "gapc_task.h" 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>  // standard integer


/*
 * DEFINES
 ****************************************************************************************
 */

#define MAX_IMAGE_SIZE (0x10000) // 64Kb 
#define CHECKSUM_SIZE  (1)       // in bytes

#define MAX_SCAN_DEVICES      (9)
#define MAX_SCANNING_ATTEMPTS (3)

#define CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR_UUID 0x2902

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

// 128-bit UUID (stored in LE order)
typedef uint8_t uuid_128_t[16];

#define UUID_128_LE(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0) \
    {b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}


// SPOTA service UUIDs
#define SPOTA_PRIMARY_SERVICE_UUID 0xFEF5  // SPOTA standard UUID

extern const uuid_128_t SPOTA_MEM_DEV_UUID;
extern const uuid_128_t SPOTA_GPIO_MAP_UUID;
extern const uuid_128_t SPOTA_MEM_INFO_UUID;
extern const uuid_128_t SPOTA_PATCH_LEN_UUID;
extern const uuid_128_t SPOTA_PATCH_DATA_UUID;
extern const uuid_128_t SPOTA_SERV_STATUS_UUID;

// SPOTA_SERV_STATUS values
enum {
    SPOTA_STATUS_SRV_STARTED    = 0x01,
    SPOTA_STATUS_CMP_OK         = 0x02,
    SPOTA_STATUS_SRV_EXIT       = 0x03,
    SPOTA_STATUS_CRC_ERR        = 0x04,
    SPOTA_STATUS_PATCH_LEN_ERR  = 0x05,
    SPOTA_STATUS_EXT_MEM_ERR    = 0x06,
    SPOTA_STATUS_INT_MEM_ERR    = 0x07,
    SPOTA_STATUS_INVAL_MEM_TYPE = 0x08,
    SPOTA_STATUS_APP_ERROR      = 0x09,
    SPOTA_STATUS_IMG_STARTED    = 0x10,     // SPOTA started for downloading image (SUOTA application)
};

// SPOTA memory types 
enum
{
    SPOTA_MEM_DEV_SYSTEM_RAM = 0x00,
    SPOTA_MEM_DEV_RETENTION_RAM = 0x01,
    SPOTA_MEM_DEV_I2C = 0x02,
    SPOTA_MEM_DEV_SPI = 0x03,
    SUOTA_MEM_DEV_I2C = 0x12,
    SUOTA_MEM_DEV_SPI = 0x13
};


// GPIO's
enum
{
    INVALID_GPIO = 0xFF,

    P0_0 = 0x00,
    P0_1 = 0x01,
    P0_2 = 0x02,
    P0_3 = 0x03,
    P0_4 = 0x04,
    P0_5 = 0x05,
    P0_6 = 0x06,
    P0_7 = 0x07,

    P1_0 = 0x10,
    P1_1 = 0x11,
    P1_2 = 0x12,
    P1_3 = 0x13,
    
    P2_0 = 0x20,
    P2_1 = 0x21,
    P2_2 = 0x22,
    P2_3 = 0x23,
    P2_4 = 0x24,
    P2_5 = 0x25,
    P2_6 = 0x26,
    P2_7 = 0x27,
    P2_8 = 0x28,
    P2_9 = 0x29,
    
    P3_0 = 0x30,
    P3_1 = 0x31,
    P3_2 = 0x32,
    P3_3 = 0x33,
    P3_4 = 0x34,
    P3_5 = 0x35,
    P3_6 = 0x36,
    P3_7 = 0x37
};

typedef struct {
    char *name;
    uint8_t code;
} gpio_name_t;


extern uint16_t suota_block_size;


extern int patch_length; // counts bytes
extern uint8_t patch_data[MAX_IMAGE_SIZE + CHECKSUM_SIZE];

extern uint32_t block_offset; 
extern uint16_t block_length; 
extern uint32_t patch_chunck_offset; 
extern uint8_t patch_chunck_length;

extern expected_write_completion_events_counter;

typedef struct {
    uint32_t patch_base_address;
    uint16_t i2c_device_address;
    uint8_t scl_gpio;
    uint8_t sda_gpio;
} spota_i2c_options_t;

typedef struct {
    uint32_t patch_base_address;
    uint8_t miso_gpio;
    uint8_t mosi_gpio;
    uint8_t cs_gpio;
    uint8_t sck_gpio;
} spota_spi_options_t;


typedef struct {
    int comport;
    struct bd_addr target_spotar_bdaddr;
    char *bin_file_name;

    uint8_t mem_type;
    union {
        spota_i2c_options_t i2c_options;
        spota_spi_options_t spi_options;
    };

    uint8_t *tk;
    uint8_t tk_len;
} spota_options_t;


extern  spota_options_t spota_options;


typedef struct 
{
    unsigned char  free;
    struct bd_addr adv_addr;
    uint8_t adv_addr_type;
    unsigned short conidx;
    unsigned short conhdl;
    unsigned char idx;
    char  rssi;
    unsigned char  data_len;
    unsigned char  data[ADV_DATA_LEN + 1];

} ble_dev;


typedef struct {
    uint8_t svc_found;
    uint16_t svc_start_handle;
    uint16_t svc_end_handle;

    // SPOTA_MEM_DEV value descriptor handle
    uint16_t mem_dev_handle;
    // SPOTA_GPIO_MAP value descriptor handle
    uint16_t gpio_map_handle;
    // SPOTA_MEM_INFO value descriptor handle
    uint16_t mem_info_handle;
    // SPOTA_PATCH_LEN value descriptor handle
    uint16_t patch_len_handle;
    // SPOTA_PATCH_DATA value descriptor handle
    uint16_t patch_data_handle;
    // SPOTA_SERV_STATUS value descriptor handle
    uint16_t serv_status_handle;

    // SPOTA_SERV_STATUS Client Characteristic Configuration Descriptor handle
    uint16_t serv_status_cli_char_cfg_desc_handle;

    uint8_t mem_dev_value[4];
    uint8_t gpio_map_value[4];
    uint8_t mem_info_value[4];
    uint8_t patch_len_value[2];
    uint8_t patch_data_value[20];
    uint8_t serv_status_value;
} spota_env;

// SPOTA Receiver connected device
typedef struct 
{
    ble_dev device;
    unsigned char bonded;
    unsigned short ediv;
    struct rand_nb rand_nb[RAND_NB_LEN];
    struct gapc_ltk ltk;
    struct gapc_irk irk;
    struct gap_sec_key csrk;

    spota_env spota;
} peer_dev;


// application environment structure
struct app_env_tag
{
    unsigned char state;
    unsigned char num_of_devices;
    ble_dev devices[MAX_SCAN_DEVICES];
    peer_dev peer_device;

    uint8_t scan_attempts_made;
    int target_idx; // index of target in devices array
};



// Application environment
extern struct app_env_tag app_env;

extern bool gapm_reset_cmd_completed;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

void app_set_mode(void);

void app_inq(void);

void app_connect(unsigned char indx);

void app_connect_confirm(uint8_t auth);

void app_disconnect(void);

unsigned char app_device_recorded(struct bd_addr *addr);

void app_cancel(void);

void app_security_enable(void);

void app_start_encryption(void);

void app_read_rem_fetures(void);

void app_rst_gap(void);

void app_gap_bond_cfm(void);

void app_gap_bond_tk_cfm(uint8_t tk_type);

void app_read_rssi(void);

bool bdaddr_compare(struct bd_addr *bd_address1, struct bd_addr *bd_address2);

void app_discover_svc_by_uuid_16(uint16_t conidx, uint16_t uuid);

void app_discover_all_char(uint16_t conidx, uint16_t start_handle, uint16_t end_handle);

void app_characteristic_write(uint16_t conidx, uint16_t char_val_handle, uint8_t *value, uint8_t value_length);

void app_characteristic_read(uint16_t conidx, uint16_t char_val_handle);

void app_discover_char_desc(uint16_t conidx, uint16_t start_handle, uint16_t end_handle);

void app_exit();

// SUOTA related functions
bool is_last_block(void);
void next_block(void);
bool is_last_chunk(void);
void next_chunk(void);
void app_suota_write_mem_dev(void);
void app_suota_write_gpio_map();
void app_suota_write_patch_len(uint16_t patch_len);
void app_suota_write_chunks(void);
void app_suota_end(void);

void print_time(void);

#endif // APP_H_
