/**
 ****************************************************************************************
 *
 * @file streamdatad.h
 *
 * @brief StreamData Device profile attribute declarations
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

#ifndef STREAMDATAD_H_
#define STREAMDATAD_H_

/**
 ****************************************************************************************
 * @addtogroup STREAMDATAD StreamData Device
 * @ingroup PROFILE
 * @brief Health Thermometer Profile
 *
 * The STREAMDATAD module is the responsible block for implementing the StreamData Device Profile
 * functionalities in the BLE Host.
 *
 *****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup STREAMDATAD_DEV StreamData Device Device
 * @ingroup STREAMDATAD
 * @brief Definition of StreamData Device attributes
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "atts.h"
#include "prf_types.h"

#if (BLE_STREAMDATA_DEVICE) || (BLE_STREAMDATA_HOST)

// Size of a single streamdatad packet in bytes
#define STREAMDATAD_PACKET_SIZE (20)
#define MAX_TRANSMIT_BUFFER_PACKETS (10)

extern uint8_t nb_buf_av;

// Proprietary UUIDs
enum
{
    STREAMDATAD_SERVICE_UUID = 0xFFA0,
    STREAMDATAD_ENABLE_UUID,
    STREAMDATAD_D0_UUID,
    STREAMDATAD_D1_UUID,
    STREAMDATAD_D2_UUID,
    STREAMDATAD_D3_UUID,
    STREAMDATAD_D4_UUID,
    STREAMDATAD_D5_UUID,
    STREAMDATAD_D6_UUID,
    STREAMDATAD_D7_UUID,
    STREAMDATAD_D8_UUID,
    STREAMDATAD_D9_UUID
    
};

#endif

#if (BLE_STREAMDATA_DEVICE)
/*
 * DEFINES
 ****************************************************************************************
 */

/// Get database attribute handle
#define STREAMDATAD_HANDLE(idx) \
    (streamdatad_env.shdl + (idx))

/// Get database attribute index
#define STREAMDATAD_IDX(hdl) \
    ((hdl) - streamdatad_env.shdl)

#define STREAMDATAD_DIR_EN_HANDLE(dir) \
    STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D0_EN + 4 * ((dir)))

#define STREAMDATAD_DIR_VAL_HANDLE(dir) \
    STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D0_VAL + 4 * ((dir)))


enum
{
    STREAMDATAD_D0 = 0,
    STREAMDATAD_D1,
    STREAMDATAD_D2,
    STREAMDATAD_D3,
    STREAMDATAD_D4,
    STREAMDATAD_D5,
    STREAMDATAD_D6,
    STREAMDATAD_D7,
    STREAMDATAD_D8,
    STREAMDATAD_D9,
    STREAMDATAD_MAX
};

/// Pointer to the connection clean-up function
#define STREAMDATAD_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct streamdatad_env_tag
{
    struct prf_con_info con_info;
    /// Application Task Id
    ke_task_id_t appid;
    /// Connection handle
    uint16_t conhdl;
    /// Start DB Handle
    uint16_t shdl;
	
	uint16_t nr_enabled_attributes;
	uint16_t next_attribute_idx;
	uint16_t stream_enabled;
	
	uint16_t nr_overflow_packets;
	uint16_t overflow_packets[STREAMDATAD_MAX][STREAMDATAD_PACKET_SIZE/2];
};

    /// Connection Info
struct stream_con_info_tag{
    struct prf_con_info con_info;
};

/*
 * MACROS
 ****************************************************************************************
 */

#define STREAMDATAD_ENABLE_DESC        "Enable StreamData Device"
#define STREAMDATAD_ENABLE_DESC_LEN    24
#define STREAMDATAD_D_DESC             "Data"
#define STREAMDATAD_D_DESC_LEN         4

/* StreamData Device service */
enum
{
    STREAMDATAD_IDX_PRIM_SVC,
    STREAMDATAD_IDX_ENABLE_CHAR,
    STREAMDATAD_IDX_ENABLE_VAL,
	STREAMDATAD_IDX_ENABLE_EN,
    STREAMDATAD_IDX_ENABLE_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D0_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D0_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D0_EN,
    STREAMDATAD_IDX_STREAMDATAD_D0_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D1_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D1_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D1_EN,
    STREAMDATAD_IDX_STREAMDATAD_D1_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D2_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D2_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D2_EN,
    STREAMDATAD_IDX_STREAMDATAD_D2_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D3_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D3_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D3_EN,
    STREAMDATAD_IDX_STREAMDATAD_D3_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D4_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D4_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D4_EN,
    STREAMDATAD_IDX_STREAMDATAD_D4_DESC,
    STREAMDATAD_IDX_STREAMDATAD_D5_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D5_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D5_EN,
    STREAMDATAD_IDX_STREAMDATAD_D5_DESC,
        STREAMDATAD_IDX_STREAMDATAD_D6_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D6_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D6_EN,
    STREAMDATAD_IDX_STREAMDATAD_D6_DESC,
        STREAMDATAD_IDX_STREAMDATAD_D7_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D7_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D7_EN,
    STREAMDATAD_IDX_STREAMDATAD_D7_DESC,
        STREAMDATAD_IDX_STREAMDATAD_D8_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D8_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D8_EN,
    STREAMDATAD_IDX_STREAMDATAD_D8_DESC,
        STREAMDATAD_IDX_STREAMDATAD_D9_CHAR,
    STREAMDATAD_IDX_STREAMDATAD_D9_VAL,
    STREAMDATAD_IDX_STREAMDATAD_D9_EN,
    STREAMDATAD_IDX_STREAMDATAD_D9_DESC,
    STREAMDATAD_IDX_NB
    
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct streamdatad_env_tag streamdatad_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize the streamdataderometer environment
 ****************************************************************************************
 */
void streamdatad_init(void);
void streamdatad_streamonoff(void);
int streamdatad_send_data_packet(uint8_t *data);


#endif /* BLE_STREAMDATA_DEVICE */

/// @} STREAMDATAD_DEV

#endif // STREAMDATAD_H_
