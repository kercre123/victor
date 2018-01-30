/**
 ****************************************************************************************
 *
 * @file accel.h
 *
 * @brief Accelerometer profile attribute declarations
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ACCEL_H_
#define ACCEL_H_

/**
 ****************************************************************************************
 * @addtogroup ACCEL Accelerator
 * @ingroup PROFILE
 * @brief Accelerometer Profile
 *
 * The ACCEL module is the responsible block for implementing the Accelerator Profile
 * functionalities in the BLE Host.
 *
 *****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup ACCEL_DEV Accelerator Device
 * @ingroup ACCEL
 * @brief Definition of Accelerometer attributes
 * @{
 ****************************************************************************************
 */

/// Accelerometer Device Server Role
#define BLE_ACCEL          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "prf_types.h"

#if (BLE_ACCEL)
/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum length of display lines
#define ACCEL_LINE_LEN_MAX      17


/// Get database attribute handle
#define ACCEL_HANDLE(idx) \
    (accel_env.shdl + (idx))

/// Get database attribute index
#define ACCEL_IDX(hdl) \
    ((hdl) - accel_env.shdl)

#define ACCEL_DIR_EN_HANDLE(dir) \
    ACCEL_HANDLE(ACCEL_IDX_ACCEL_X_EN + 4 * ((dir)))

#define ACCEL_DIR_VAL_HANDLE(dir) \
    ACCEL_HANDLE(ACCEL_IDX_ACCEL_X_VAL + 4 * ((dir)))


enum
{
    ACCEL_X = 0,
    ACCEL_Y,
    ACCEL_Z,
    ACCEL_MAX
};


/// Pointer to the connection clean-up function
#define ACCEL_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
/// Accelerometer environment structure definition
struct accel_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;
    /// Start DB Handle
    uint16_t shdl;
};


/*
 * MACROS
 ****************************************************************************************
 */
// Proprietary UUIDs
enum
{
    ACCEL_SERVICE_UUID = 0xFFA0,
    ACCEL_ENABLE_UUID,
    ACCEL_RANGE_UUID,
    ACCEL_X_UUID,
    ACCEL_Y_UUID,
    ACCEL_Z_UUID,
    ACCEL_DISPLAY1_UUID,
    ACCEL_DISPLAY2_UUID
};

#define ACCEL_ENABLE_DESC        "Enable accel"
#define ACCEL_ENABLE_DESC_LEN    12
#define ACCEL_RANGE_DESC         "Accel range"
#define ACCEL_RANGE_DESC_LEN     11
#define ACCEL_X_DESC             "X-coordinate"
#define ACCEL_X_DESC_LEN         12
#define ACCEL_Y_DESC             "Y-coordinate"
#define ACCEL_Y_DESC_LEN         12
#define ACCEL_Z_DESC             "Z-coordinate"
#define ACCEL_Z_DESC_LEN         12
#define ACCEL_DISPLAY1_DESC      "Display Line 1"
#define ACCEL_DISPLAY1_DESC_LEN  14
#define ACCEL_DISPLAY2_DESC      "Display Line 2"
#define ACCEL_DISPLAY2_DESC_LEN  14



/* Accelerometer service */
enum
{
    ACCEL_IDX_PRIM_SVC,
    ACCEL_IDX_ENABLE_CHAR,
    ACCEL_IDX_ENABLE_VAL,
    ACCEL_IDX_ENABLE_DESC,
    ACCEL_IDX_RANGE_CHAR,
    ACCEL_IDX_RANGE_VAL,
    ACCEL_IDX_RANGE_DESC,
    ACCEL_IDX_ACCEL_X_CHAR,
    ACCEL_IDX_ACCEL_X_VAL,
    ACCEL_IDX_ACCEL_X_EN,
    ACCEL_IDX_ACCEL_X_DESC,
    ACCEL_IDX_ACCEL_Y_CHAR,
    ACCEL_IDX_ACCEL_Y_VAL,
    ACCEL_IDX_ACCEL_Y_EN,
    ACCEL_IDX_ACCEL_Y_DESC,
    ACCEL_IDX_ACCEL_Z_CHAR,
    ACCEL_IDX_ACCEL_Z_VAL,
    ACCEL_IDX_ACCEL_Z_EN,
    ACCEL_IDX_ACCEL_Z_DESC,
    ACCEL_IDX_ACCEL_DISPLAY1_CHAR,
    ACCEL_IDX_ACCEL_DISPLAY1_VAL,
    ACCEL_IDX_ACCEL_DISPLAY1_DESC,
    ACCEL_IDX_ACCEL_DISPLAY2_CHAR,
    ACCEL_IDX_ACCEL_DISPLAY2_VAL,
    ACCEL_IDX_ACCEL_DISPLAY2_DESC,
    ACCEL_IDX_NB
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct accel_env_tag accel_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize the accelerometer environment
 ****************************************************************************************
 */
void accel_init(void);

#endif /* BLE_ACCEL */

/// @} ACCEL_DEV

#endif // ACCEL_H_
