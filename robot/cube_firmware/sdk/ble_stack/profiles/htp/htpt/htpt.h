/**
 ****************************************************************************************
 *
 * @file htpt.h
 *
 * @brief Header file - Health Thermometer Profile Thermometer.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HTPT_H_
#define HTPT_H_

/**
 ****************************************************************************************
 * @addtogroup HTPT Health Thermometer Profile Thermometer
 * @ingroup HTP
 * @brief Health Thermometer Profile Thermometer
 *
 * An actual thermometer device does not exist on current platform, so measurement values
 * that would come from a driver are replaced by simple counters sent at certain intervals
 * following by the profile attributes configuration.
 * When a measurement interval
 * has been set to a non-zero value in a configuration connection, once reconnected,
 * TH will send regular measurement INDs if Temp Meas Char Cfg is set to indicate and
 * using the Meas Intv Value. The INDs will continue until meas interval is set to 0
 * or connection gets disconnected by C. Measurements should be stored even so, until
 * profile is disabled.(?)
 *
 * If the measurement interval has been set to 0, then if Intermediate Temp is set to be
 * notified and Temp Meas to be indicated, then a timer of fixed length simulates
 * sending several NTF before and indication of a "stable" value. This fake behavior
 * should be replaced once a real driver exists. If Intermediary Temp cannot be notified,
 * just send the indication, if neither can be sent (the configuration connection should
 * never leave this like this) then disconnect.
 *
 *
 * @{
 ****************************************************************************************
 */

/// Health Thermometer Profile Thermometer Role
#define BLE_HT_THERMOM          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_HT_THERMOM)
#include <stdint.h>
#include <stdbool.h>
#include "htp_common.h"
#include "prf_types.h"
#include "gap.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HTPT_OUT_OF_RANGE_ERR_CODE    0x80

///Valid range for measurement interval values (s)
#define HTPT_MEAS_INTV_DFLT_MIN           (0x0001)
#define HTPT_MEAS_INTV_DFLT_MAX           (0x000A)

#define HTPT_TEMP_MEAS_MAX_LEN            (13)
#define HTPT_TEMP_TYPE_MAX_LEN            (1)
#define HTPT_MEAS_INTV_MAX_LEN            (2)

#define HTPT_TEMP_MEAS_MASK                (0x000F)
#define HTPT_TEMP_TYPE_MASK                (0x0030)
#define HTPT_INTM_TEMP_MASK                (0x01C0)
#define HTPT_MEAS_INTV_MASK                (0x0600)
#define HTPT_MEAS_INTV_CCC_MASK            (0x0800)
#define HTPT_MEAS_INTV_VALID_RGE_MASK      (0x1000)

#define HTPT_CFG_NTFIND_MASK            (0xE0)

///Attributes State Machine
enum
{
    HTS_IDX_SVC,

    HTS_IDX_TEMP_MEAS_CHAR,
    HTS_IDX_TEMP_MEAS_VAL,
    HTS_IDX_TEMP_MEAS_IND_CFG,

    HTS_IDX_TEMP_TYPE_CHAR,
    HTS_IDX_TEMP_TYPE_VAL,

    HTS_IDX_INTERM_TEMP_CHAR,
    HTS_IDX_INTERM_TEMP_VAL,
    HTS_IDX_INTERM_TEMP_CFG,

    HTS_IDX_MEAS_INTV_CHAR,
    HTS_IDX_MEAS_INTV_VAL,
    HTS_IDX_MEAS_INTV_CFG,
    HTS_IDX_MEAS_INTV_VAL_RANGE,

    HTS_IDX_NB,
};

///Attribute Table Indexes
enum
{
    HTPT_TEMP_MEAS_CHAR,
    HTPT_TEMP_TYPE_CHAR,
    HTPT_INTERM_TEMP_CHAR,
    HTPT_MEAS_INTV_CHAR,

    HTPT_CHAR_MAX,
};

///Database Configuration Flags
enum
{
    ///Indicate if Temperature Type Char. is supported
    HTPT_TEMP_TYPE_CHAR_SUP        = 0x01,
    ///Indicate if Intermediate Temperature Char. is supported
    HTPT_INTERM_TEMP_CHAR_SUP      = 0x02,
    ///Indicate if Measurement Interval Char. is supported
    HTPT_MEAS_INTV_CHAR_SUP        = 0x04,
    ///Indicate if Measurement Interval Char. supports indications
    HTPT_MEAS_INTV_IND_SUP         = 0x08,
    ///Indicate if Measurement Interval Char. is writable
    HTPT_MEAS_INTV_WR_SUP          = 0x10,

    /// All Features supported
    HTPT_ALL_FEAT_SUP              = 0x1F,
};

enum
{
    HTPT_MASK_TEMP_MEAS_CFG        = 0x20,
    HTPT_MASK_INTM_MEAS_CFG        = 0x40,
    HTPT_MASK_MEAS_INTV_CFG        = 0x80,
};

/// Pointer to the connection clean-up function
#define HTPT_CLEANUP_FNCT        (NULL)


///Health Thermometer Profile Thermometer Environment Variable
struct htpt_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    ///Service Start Handle
    uint16_t shdl;
    ///Attribute Table
    uint8_t att_tbl[HTPT_CHAR_MAX];
    ///Database configuration
    uint8_t features;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Array containing attribute to add into the database
extern const struct attm_desc htpt_att_db[HTS_IDX_NB];

/// Health Thermometer Service
extern const att_svc_desc_t htpt_hlth_therm_svc;

/// Temperature Measurement Char.
extern const struct att_char_desc htpt_temp_meas_char;
/// Temperature Type Char.
extern const struct att_char_desc htpt_temp_type_char;
/// Intermediate Temperature Char.
extern const struct att_char_desc htpt_interm_temp_char;
/// Measurement Interval Char.
extern const struct att_char_desc htpt_meas_intv_char;

extern struct htpt_env_tag htpt_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HTPT module.
 * This function performs all the initializations of the HTPT module.
 ****************************************************************************************
 */
void htpt_init(void);

/**
 ****************************************************************************************
 * @brief Compute the offset of the valid range descriptor.
 * The Measurement Interval characteristic has two optional descriptors. In the database,
 * the Client Characteristic Configuration descriptor will always be placed just after the
 * characteristic value. Thus, this function checks if the CCC descriptor has been added.
 * @return     0 if Measurement Interval Char. is not writable (no Valid Range descriptor)
 *             1 if Measurement Interval Char. doesn't support indications (no CCC descriptor)
 *             2 otherwise
 ****************************************************************************************
 */
uint8_t htpt_get_valid_rge_offset(void);

/**
 ****************************************************************************************
 * @brief Pack temperature value from several components
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t htpt_pack_temp_value(uint8_t *packed_temp, struct htp_temp_meas temp_meas);

/**
 ****************************************************************************************
 * @brief Compute a flag allowing to know attributes to add into the database
 *
 * @return a 16-bit flag whose each bit matches an attribute. If the bit is set to 1, the
 * attribute will be added into the database.
 ****************************************************************************************
 */
uint16_t htpt_compute_att_table(uint8_t features);

/**
 ****************************************************************************************
 * @brief Send a HTPT_TEMP_SEND_CFM message to the application
 ****************************************************************************************
 */
void htpt_temp_send_cfm_send(uint8_t status, uint8_t cfm_type);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void htpt_disable(uint16_t conhdl); 

#endif //BLE_HT_THERMOM

/// @} HTPT

#endif // HTPT_H_
