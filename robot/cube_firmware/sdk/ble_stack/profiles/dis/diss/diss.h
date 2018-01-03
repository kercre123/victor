/**
 ****************************************************************************************
 *
 * @file diss.h
 *
 * @brief Header file - Device Information Service Server.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef DISS_H_
#define DISS_H_

/**
 ****************************************************************************************
 * @addtogroup DISS Device Information Service Server
 * @ingroup DIS
 * @brief Device Information Service Server
 * @{
 ****************************************************************************************
 */
 
/// Device Information Service Server Role
#define BLE_DIS_SERVER          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_DIS_SERVER)
//#include "atts.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal length for Characteristic values - 18
#define DIS_VAL_MAX_LEN                         (0x12)

///System ID string length
#define DIS_SYS_ID_LEN                          (0x08)
///IEEE Certif length (min 6 bytes)
#define DIS_IEEE_CERTIF_MIN_LEN                 (0x06)
///PnP ID length
#define DIS_PNP_ID_LEN                          (0x07)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Attributes State Machine
enum
{
    DIS_IDX_SVC,

    DIS_IDX_MANUFACTURER_NAME_CHAR,
    DIS_IDX_MANUFACTURER_NAME_VAL,

    DIS_IDX_MODEL_NB_STR_CHAR,
    DIS_IDX_MODEL_NB_STR_VAL,

    DIS_IDX_SERIAL_NB_STR_CHAR,
    DIS_IDX_SERIAL_NB_STR_VAL,

    DIS_IDX_HARD_REV_STR_CHAR,
    DIS_IDX_HARD_REV_STR_VAL,

    DIS_IDX_FIRM_REV_STR_CHAR,
    DIS_IDX_FIRM_REV_STR_VAL,

    DIS_IDX_SW_REV_STR_CHAR,
    DIS_IDX_SW_REV_STR_VAL,

    DIS_IDX_SYSTEM_ID_CHAR,
    DIS_IDX_SYSTEM_ID_VAL,

    DIS_IDX_IEEE_CHAR,
    DIS_IDX_IEEE_VAL,

    DIS_IDX_PNP_ID_CHAR,
    DIS_IDX_PNP_ID_VAL,

    DIS_IDX_NB,
};

///Attribute Table Indexes
enum
{
    DIS_MANUFACTURER_NAME_CHAR,
    DIS_MODEL_NB_STR_CHAR,
    DIS_SERIAL_NB_STR_CHAR,
    DIS_HARD_REV_STR_CHAR,
    DIS_FIRM_REV_STR_CHAR,
    DIS_SW_REV_STR_CHAR,
    DIS_SYSTEM_ID_CHAR,
    DIS_IEEE_CHAR,
    DIS_PNP_ID_CHAR,

    DIS_CHAR_MAX,
};

///Database Configuration Flags
enum
{
    ///Indicate if Manufacturer Name String Char. is supported
    DIS_MANUFACTURER_NAME_CHAR_SUP       = 0x0001,
    ///Indicate if Model Number String Char. is supported
    DIS_MODEL_NB_STR_CHAR_SUP            = 0x0002,
    ///Indicate if Serial Number String Char. is supported
    DIS_SERIAL_NB_STR_CHAR_SUP           = 0x0004,
    ///Indicate if Hardware Revision String Char. supports indications
    DIS_HARD_REV_STR_CHAR_SUP            = 0x0008,
    ///Indicate if Firmware Revision String Char. is writable
    DIS_FIRM_REV_STR_CHAR_SUP            = 0x0010,
    ///Indicate if Software Revision String Char. is writable
    DIS_SW_REV_STR_CHAR_SUP              = 0x0020,
    ///Indicate if System ID Char. is writable
    DIS_SYSTEM_ID_CHAR_SUP               = 0x0040,
    ///Indicate if IEEE Char. is writable
    DIS_IEEE_CHAR_SUP                    = 0x0080,
    ///Indicate if PnP ID Char. is writable
    DIS_PNP_ID_CHAR_SUP                  = 0x0100,

    ///All features are supported
    DIS_ALL_FEAT_SUP                     = 0x01FF,
};

/// Pointer to the connection clean-up function
#define DISS_CLEANUP_FNCT        (NULL)

///Device Information Service Server Environment Variable
struct diss_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// Service Start HandleVAL
    uint16_t shdl;
    /// Attribute Table
    uint8_t att_tbl[DIS_CHAR_MAX];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc diss_att_db[DIS_IDX_NB];

extern const att_svc_desc_t diss_dis_svc;

extern const struct att_char_desc diss_manufacturer_name_char;
extern const struct att_char_desc diss_model_nb_str_char;
extern const struct att_char_desc diss_serial_nb_str_char;
extern const struct att_char_desc diss_hard_rev_str_char;
extern const struct att_char_desc diss_firm_rev_str_char;
extern const struct att_char_desc diss_sw_rev_str_char;
extern const struct att_char_desc diss_system_id_char;
extern const struct att_char_desc diss_ieee_char;
extern const struct att_char_desc diss_pnp_id_char;

extern struct diss_env_tag diss_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the DISS module.
 * This function performs all the initializations of the DISS module.
 ****************************************************************************************
 */
void diss_init(void);

/**
 ****************************************************************************************
 * @brief Check if an attribute shall be added or not in the database
 * @param features DIS features
 ****************************************************************************************
 */
uint32_t diss_compute_cfg_flag(uint16_t features);

/**
 ****************************************************************************************
 * @brief Check if the provided value length matches characteristic requirements
 * @param char_code Characteristic Code
 * @param val_len   Length of the Characteristic value
 ****************************************************************************************
 */
uint8_t diss_check_val_len(uint8_t char_code, uint8_t val_len);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void diss_disable(uint16_t conhdl); 

#endif //BLE_DIS_SERVER

/// @} DISS

#endif // DISS_H_
