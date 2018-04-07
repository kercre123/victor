/**
 ****************************************************************************************
 *
 * @file hogpbh.h
 *
 * @brief Header file - HID Over GATT Profile - Boot Host Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HOGPBH_H_
#define _HOGPBH_H_


/**
 ****************************************************************************************
 * @addtogroup HOGPBH HID Over GATT Profile Client
 * @ingroup HOGP
 * @brief HID Over GATT Profile Client
 * @{
 ****************************************************************************************
 */


/// HOGPBH HID Over GATT Profile Client Role
#define BLE_HID_BOOT_HOST           1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HID_BOOT_HOST)

#include "hogp_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of hids instances that can be handled
#define HOGPBH_NB_HIDS_INST_MAX              (2)

/// Maximal Length of Boot Report Char. Value
#define HOGPBH_BOOT_REPORT_MAX_LEN           (8)

/*
 * MACROS
 ****************************************************************************************
 */

#define HOGPBH_FOUND_CHAR(p_hogpbh_env, char_code) \
    (hogpbh_env->hids[hogpbh_env->last_svc_inst_req].chars[char_code].char_hdl != ATT_INVALID_SEARCH_HANDLE)

#define HOGPBH_FOUND_DESC(p_hogpbh_env, desc_code) \
    (hogpbh_env->hids[hogpbh_env->last_svc_inst_req].descs[desc_code].desc_hdl != ATT_INVALID_SEARCH_HANDLE)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Protocol Mode
    HOGPBH_CHAR_PROTO_MODE,
    /// Boot Keyboard Input Report
    HOGPBH_CHAR_BOOT_KB_IN_REPORT,
    /// Boot Keyboard Output Report
    HOGPBH_CHAR_BOOT_KB_OUT_REPORT,
    /// Boot Mouse Input Report
    HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT,

    HOGPBH_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Boot Keyboard Input Report Client Config
    HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG,
    /// Boot Mouse Input Report Client Config
    HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG,

    HOGPBH_DESC_MAX,

    HOGPBH_DESC_MASK = 0x10,
};

/// Internal codes for reading or writing a HIDS characteristic
enum
{
    /// Read/Write HIDS Protocol Mode
    HOGPBH_RD_WR_HIDS_PROTO_MODE            = HOGPBH_CHAR_PROTO_MODE,
    /// Read/Write HIDS Boot Keyboard Input Report
    HOGPBH_RD_WR_HIDS_BOOT_KB_IN_REPORT     = HOGPBH_CHAR_BOOT_KB_IN_REPORT,
    /// Read/Write HIDS Boot Keyboard Output Report
    HOGPBH_RD_WR_HIDS_BOOT_KB_OUT_REPORT    = HOGPBH_CHAR_BOOT_KB_OUT_REPORT,
    /// Read/Write HIDS Boot Mouse Input Report
    HOGPBH_RD_WR_HIDS_BOOT_MOUSE_IN_REPORT  = HOGPBH_CHAR_BOOT_MOUSE_IN_REPORT,

    /// Read/Write HIDS Boot Keyboard Input Report Client Cfg. Desc
    HOGPBH_RD_WR_HIDS_BOOT_KB_IN_REPORT_CFG      = (HOGPBH_DESC_MASK | HOGPBH_DESC_BOOT_KB_IN_REPORT_CFG),
    /// Read/Write HIDS Boot Mouse Input Report Client Cfg. Desc
    HOGPBH_RD_WR_HIDS_BOOT_MOUSE_IN_REPORT_CFG   = (HOGPBH_DESC_MASK | HOGPBH_DESC_BOOT_MOUSE_IN_REPORT_CFG),
};

/// Indication types codes

enum
{
    ///Notification
    HOGPBH_IND_NTF                    = 0x00,
    ///Read Response
    HOGPBH_IND_RD_RSP,
};

/// Pointer to the connection clean-up function
#define HOGPBH_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct hids_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Protocol Mode
    ///  - Boot Keyboard Input Report
    ///  - Boot Keyboard Output Report
    ///  - Boot Mouse Input Report
    struct prf_char_inf chars[HOGPBH_CHAR_MAX];

    /// Descriptor handles:
    ///  - Boot Keyboard Input Client Cfg
    ///  - Boot Mouse Input Client Cfg
    struct prf_char_desc_inf descs[HOGPBH_DESC_MAX];
};

/// HID Over Gatt Profile Boot Host environment variable
struct hogpbh_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID
    uint16_t last_uuid_req;
    ///Last instance of service for which something was requested
    uint8_t last_svc_inst_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    ///Number of HIDS instances found
    uint8_t hids_nb;
    ///HIDS characteristics
    struct hids_content hids[HOGPBH_NB_HIDS_INST_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct hogpbh_env_tag **hogpbh_envs;

/*
 * GLOBAL FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HOGPBH module.
 * This function performs all the initializations of the HOGPBH module.
 ****************************************************************************************
 */
void hogpbh_init(void);

/**
 ****************************************************************************************
 * @brief Send HID ATT DB discovery results to HOGPBH host.
 ****************************************************************************************
 */
void hogpbh_enable_cfm_send(struct hogpbh_env_tag *hogpbh_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void hogpbh_char_req_rsp_send(struct hogpbh_env_tag *hogpbh_env, ke_msg_id_t msg_id, uint8_t status);

#endif /* (BLE_HID_BOOT_HOST) */

/// @} HOGPBH

#endif /* _HOGPBH_H_ */
