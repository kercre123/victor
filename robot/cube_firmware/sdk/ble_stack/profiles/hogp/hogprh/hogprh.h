/**
 ****************************************************************************************
 *
 * @file hogprh.h
 *
 * @brief Header file - HID Over GATT Profile - Report Host Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HOGPRH_H_
#define _HOGPRH_H_


/**
 ****************************************************************************************
 * @addtogroup HOGPRH HID Over GATT Profile Report Host
 * @ingroup HOGP
 * @brief HID Over GATT Profile Report Host
 * @{
 ****************************************************************************************
 */

/// HOGPRH HID Over GATT Profile Report Host role
#define BLE_HID_REPORT_HOST           1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HID_REPORT_HOST)

#include "hogp_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal number of hids instances that can be handled
#define HOGPRH_NB_HIDS_INST_MAX              (2)
/// Maximal number of Report Char. that can be added in the DB for one HIDS - Up to 11
#define HOGPRH_NB_REPORT_INST_MAX            (5)

/// Maximal length of Report Map Char. Value
#define HOGPRH_REPORT_MAP_MAX_LEN            (512)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Report Map
    HOGPRH_CHAR_REPORT_MAP,
    /// HID Information
    HOGPRH_CHAR_HID_INFO,
    /// HID Control Point
    HOGPRH_CHAR_HID_CTNL_PT,
    /// Protocol Mode
    HOGPRH_CHAR_PROTOCOL_MODE,
    /// Report
    HOGPRH_CHAR_REPORT,

    HOGPRH_CHAR_MAX = HOGPRH_CHAR_REPORT + HOGPRH_NB_REPORT_INST_MAX,
};


/// Characteristic descriptors
enum
{
    /// Report Map Char. External Report Reference Descriptor
    HOGPRH_DESC_REPORT_MAP_EXT_REP_REF,
    /// Report Char. Report Reference
    HOGPRH_DESC_REPORT_REF,
    /// Report Client Config
    HOGPRH_DESC_REPORT_CFG = HOGPRH_DESC_REPORT_REF + HOGPRH_NB_REPORT_INST_MAX,

    HOGPRH_DESC_MAX = HOGPRH_DESC_REPORT_CFG + HOGPRH_NB_REPORT_INST_MAX,

    HOGPRH_DESC_MASK = 0x10,
};

/// Internal codes for reading / writing a HIDS characteristic
enum
{
    /// Read HIDS Report Map
    HOGPRH_RD_HIDS_REPORT_MAP             = HOGPRH_CHAR_REPORT_MAP,
    /// Read HIDS HID Information
    HOGPRH_RD_HIDS_HID_INFO               = HOGPRH_CHAR_HID_INFO,
    /// Read/Write HIDS Protocol Mode
    HOGPRH_RD_WR_HIDS_PROTOCOL_MODE       = HOGPRH_CHAR_PROTOCOL_MODE,
    /// Write the HID Control Point
    HOGPRH_RD_WR_HID_CTNL_PT              = HOGPRH_CHAR_HID_CTNL_PT,
    /// Read/Write HIDS Report
    HOGPRH_RD_WR_HIDS_REPORT              = HOGPRH_CHAR_REPORT,

    /// Read Report Map Char. External Report Reference Descriptor
    HOGPRH_RD_HIDS_REPORT_MAP_EXT_REF_REF = (HOGPRH_DESC_MASK | HOGPRH_DESC_REPORT_MAP_EXT_REP_REF),
    /// Read Report Char. Report Reference
    HOGPRH_RD_HIDS_REPORT_REF             = (HOGPRH_DESC_MASK | HOGPRH_DESC_REPORT_REF),
    /// Read/Write Report Client Cfg. Desc
    HOGPRH_RD_WR_HIDS_REPORT_CFG          = (HOGPRH_DESC_MASK | HOGPRH_DESC_REPORT_CFG),
};

/// Indication types codes

enum
{
    /// Notification
    HOGPRH_IND_NTF                    = 0x00,
    /// Read Response
    HOGPRH_IND_RD_RSP,
};

/// Pointer to the connection clean-up function
#define HOGPRH_CLEANUP_FNCT        (NULL)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct hogprh_hids_content
{
    /// Service info
    struct prf_svc svc;

    /// Included service info
    struct prf_incl_svc incl_svc;

    /// Characteristic info:
    struct prf_char_inf chars[HOGPRH_CHAR_MAX];

    /// Descriptor handles:
    struct prf_char_desc_inf descs[HOGPRH_DESC_MAX];

    /// Number of Report Char. that have been found
    uint8_t report_nb;
};

/// HID Over Gatt Profile Report Host environment variable
struct hogprh_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID
    uint16_t last_uuid_req;
    ///Last instance of service for which something was requested
    uint8_t last_svc_inst_req;
    ///Last instance of report char. for which something was requested
    uint8_t last_report_char_inst_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    ///Number of HIDS instances found
    uint8_t hids_nb;
    ///HIDS characteristics
    struct hogprh_hids_content hids[HOGPRH_NB_HIDS_INST_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct hogprh_env_tag **hogprh_envs;

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
void hogprh_init(void);

/**
 ****************************************************************************************
 * @brief Send HID ATT DB discovery results to HOGPBH host.
 ****************************************************************************************
 */
void hogprh_enable_cfm_send(struct hogprh_env_tag *hogprh_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void hogprh_char_req_rsp_send(struct hogprh_env_tag *hogprh_env, ke_msg_id_t msg_id, uint8_t status,
                              uint8_t att_code);

#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRH

#endif /* _HOGPRH_H_ */
