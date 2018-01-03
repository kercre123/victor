/**
 ****************************************************************************************
 *
 * @file hogpd.h
 *
 * @brief Header file - HID Over GATT Profile Device Role
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _HOGPD_H_
#define _HOGPD_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPD HID Over GATT Profile Device Role
 * @ingroup HOGP
 * @brief HID Over GATT Profile Device Role
 * @{
 ****************************************************************************************
 */


/// HOGPD HID Over GATT Profile Device Role
#define BLE_HID_DEVICE          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HID_DEVICE)
#include "hogp_common.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of HIDS that can be added in the DB
#ifndef USE_ONE_HIDS_INSTANCE
#define HOGPD_NB_HIDS_INST_MAX              (2)
#else
#define HOGPD_NB_HIDS_INST_MAX              (1)
#endif

/// Maximal number of Report Char. that can be added in the DB for one HIDS - Up to 11
#define HOGPD_NB_REPORT_INST_MAX            (5)

/// Maximal length of Report Char. Value
#define HOGPD_REPORT_MAX_LEN                (45)
/// Maximal length of Report Map Char. Value
#define HOGPD_REPORT_MAP_MAX_LEN            (512)

/// Length of Boot Report Char. Value Maximal Length
#define HOGPD_BOOT_REPORT_MAX_LEN           (8)

/// Boot KB Input Report Notification Configuration Bit Mask
#define HOGPD_BOOT_KB_IN_NTF_CFG_MASK       (0x40)
/// Boot KB Input Report Notification Configuration Bit Mask
#define HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK    (0x80)
/// Boot Report Notification Configuration Bit Mask
#define HOGPD_REPORT_NTF_CFG_MASK           (0x20)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// HID Service Attributes Indexes
enum
{
    HOGPD_IDX_SVC,

    // Included Service
    HOGPD_IDX_INCL_SVC,

    // HID Information
    HOGPD_IDX_HID_INFO_CHAR,
    HOGPD_IDX_HID_INFO_VAL,

    // HID Control Point
    HOGPD_IDX_HID_CTNL_PT_CHAR,
    HOGPD_IDX_HID_CTNL_PT_VAL,

    // Report Map
    HOGPD_IDX_REPORT_MAP_CHAR,
    HOGPD_IDX_REPORT_MAP_VAL,
    HOGPD_IDX_REPORT_MAP_EXT_REP_REF,

    // Protocol Mode
    HOGPD_IDX_PROTO_MODE_CHAR,
    HOGPD_IDX_PROTO_MODE_VAL,

    // Boot Keyboard Input Report
    HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR,
    HOGPD_IDX_BOOT_KB_IN_REPORT_VAL,
    HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG,

    // Boot Keyboard Output Report
    HOGPD_IDX_BOOT_KB_OUT_REPORT_CHAR,
    HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL,

    // Boot Mouse Input Report
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_CHAR,
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL,
    HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG,

    // Report
    HOGPD_IDX_REPORT_CHAR,
    HOGPD_IDX_REPORT_VAL,
    HOGPD_IDX_REPORT_REP_REF,
    HOGPD_IDX_REPORT_NTF_CFG,

    HOGPD_IDX_NB,
};

/// Attribute Table Indexes
enum
{
    HOGPD_HID_INFO_CHAR,
    HOGPD_HID_CTNL_PT_CHAR,
    HOGPD_REPORT_MAP_CHAR,
    HOGPD_PROTO_MODE_CHAR,
    HOGPD_BOOT_KB_IN_REPORT_CHAR,
    HOGPD_BOOT_KB_OUT_REPORT_CHAR,
    HOGPD_BOOT_MOUSE_IN_REPORT_CHAR,
    HOGPD_REPORT_CHAR,

    HOGPD_CHAR_MAX = HOGPD_REPORT_CHAR + HOGPD_NB_REPORT_INST_MAX,
};

/// Client Characteristic Configuration Codes
enum
{
    HOGPD_DESC_MASK = 0x10,

    HOGPD_BOOT_KB_IN_REPORT_CFG     = HOGPD_BOOT_KB_IN_REPORT_CHAR | HOGPD_DESC_MASK,
    HOGPD_BOOT_MOUSE_IN_REPORT_CFG  = HOGPD_BOOT_MOUSE_IN_REPORT_CHAR | HOGPD_DESC_MASK,
    HOGPD_REPORT_CFG                = HOGPD_REPORT_CHAR | HOGPD_DESC_MASK,
};

/// Features Flag Values
enum
{
    HOGPD_CFG_KEYBOARD      = 0x01,
    HOGPD_CFG_MOUSE         = 0x02,
    HOGPD_CFG_PROTO_MODE    = 0x04,
    HOGPD_CFG_MAP_EXT_REF   = 0x08,
    HOGPD_CFG_BOOT_KB_WR    = 0x10,
    HOGPD_CFG_BOOT_MOUSE_WR = 0x20,
};

/// Report Char. Configuration Flag Values
enum
{
    HOGPD_CFG_REPORT_IN     = 0x01,
    HOGPD_CFG_REPORT_OUT    = 0x02,
    //HOGPD_CFG_REPORT_FEAT can be used as a mask to check Report type
    HOGPD_CFG_REPORT_FEAT   = 0x03,
    HOGPD_CFG_REPORT_WR     = 0x10,
};

/// Pointer to the connection clean-up function
#define HOGPD_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// HIDS Features structure
struct hogpd_features
{
    /// Service Features
    uint8_t svc_features;
    /// Number of Report Char. instances to add in the database
    uint8_t report_nb;
    /// Report Char. Configuration
    uint8_t report_char_cfg[HOGPD_NB_REPORT_INST_MAX];
    /// Report Char. id
    uint8_t report_id[HOGPD_NB_REPORT_INST_MAX];
};

/// HID Over GATT Profile HID Device Role Environment variable
struct hogpd_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;
    /// Notified handle
    uint16_t ntf_handle;
    /// HIDS Start Handles
    uint16_t shdl[HOGPD_NB_HIDS_INST_MAX];
    /// Supported Features
    struct hogpd_features features[HOGPD_NB_HIDS_INST_MAX];

    ///Attribute Table
    uint8_t att_tbl[HOGPD_NB_HIDS_INST_MAX][HOGPD_CHAR_MAX];

    /// Current Protocol Mode
    uint8_t proto_mode[HOGPD_NB_HIDS_INST_MAX];

    /// Number of HIDS added in the database
    uint8_t hids_nb;
};

/// Database Creation Service Instance Configuration structure
struct hogpd_hids_cfg
{
    /// HIDS Features
    struct hogpd_features features;

    /// HID Information Char. Values
    struct hids_hid_info hid_info;
    /// External Report Reference - Included Service
    struct att_incl_desc ext_rep_ref;
    /// External Report Reference - Characteristic UUID
    uint16_t ext_rep_ref_uuid;
};

/// Notification Configurations for an HIDS Instance
struct hogpd_hids_ntf_cfg
{
    /// Boot Keyboard Input Report Ntf Cfg
    uint16_t boot_kb_in_report_ntf_en;
    /// Boot Mouse input Report Ntf Cfg
    uint16_t boot_mouse_in_report_ntf_en;

    /// Report
    uint16_t report_ntf_en[HOGPD_NB_REPORT_INST_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Full BAS Database Description
extern const struct attm_desc hids_att_db[HOGPD_IDX_NB];

/// HID Service
extern const att_svc_desc_t hid_svc;

/// HID Service Characteristics
extern const struct att_char_desc hids_proto_mode_char;
extern const struct att_char_desc hids_report_char;
extern const struct att_char_desc hids_report_map_char;
extern const struct att_char_desc hids_boot_kb_in_report_char;
extern const struct att_char_desc hids_boot_kb_out_report_char;
extern const struct att_char_desc hids_boot_mouse_in_report_char;
extern const struct att_char_desc hids_hid_info_char;
extern const struct att_char_desc hids_hid_ctnl_pt_char;

/// HOGPD Environment
extern struct hogpd_env_tag hogpd_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HOGPD module.
 * This function performs all the initializations of the HOGPD module.
 ****************************************************************************************
 */
void hogpd_init(void);

/**
 ****************************************************************************************
 * @brief Initialize a Client Characteristic Configuration Descriptor value in the database
 ****************************************************************************************
 */
void hogpd_init_ntf_cfg(uint16_t ntf_cfg, uint16_t handle, uint8_t con_type,
                        uint8_t *flag, uint8_t mask);

/**
 ****************************************************************************************
 * @brief Check if a report value can be notified to the peer central.
 ****************************************************************************************
 */
uint8_t hogpd_ntf_send(uint8_t hids_nb, uint8_t char_code, uint8_t report_nb,
                       uint16_t report_len, uint8_t *p_report);

/**
 ****************************************************************************************
 * @brief Inform APP if a notification has been sent or not.
 * @param status Error status to send to requester task
 ****************************************************************************************
 */
void hogpd_ntf_cfm_send(uint8_t status, uint8_t char_code, uint8_t hids_nb, uint8_t report_nb);

/**
 ****************************************************************************************
 * @brief Send a HOGPD_NTF_CFG_IND message to the application
 *
 * @param ntf_cfg       Client Characteristic Configuration Descriptor value
 * @param handle        Descriptor handle
 * @param cfg_code      Descriptor code
 * @param hids_nb       HID Service instance
 * @param report_nb     Report Characteristic instance
 ****************************************************************************************
 */
uint8_t hogpd_ntf_cfg_ind_send(uint16_t ntf_cfg, uint16_t handle, uint8_t cfg_code,
                               uint8_t hids_nb, uint8_t report_nb);

/**
 ****************************************************************************************
 * @brief Retrieve all the attribute information using the handle of this attribute
 *
 * @param[in]     handle       Attribute Handle
 * @param[in|out] char_code    Characteristic Code
 * @param[in|out] hids_nb      HID Service Instance
 * @param[in|out] report_nb    Report Characteristic Instance
 *
 * @return  PRF_ERR_OK if the attribute has been found in the database
 *          PRF_APP_ERROR if the attribute has not been found in the database
 ****************************************************************************************
 */
uint8_t hogpd_get_att(uint16_t handle, uint8_t *char_code, uint8_t *hids_nb, uint8_t *report_nb);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 *
 * @param[in]     conhdl       Connection Handle
 ****************************************************************************************
 */
void hogpd_disable(uint16_t conhdl); 

#endif /* #if (BLE_HID_DEVICE) */

/// @} HOGPD

#endif /* _HOGPD_H_ */
