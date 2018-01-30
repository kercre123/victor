/**
 *****************************************************************************************
 *
 * @file app_bond_db.h
 *
 * @brief Bond database header file.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 *****************************************************************************************
 */

#ifndef _APP_BOND_DB_H_
#define _APP_BOND_DB_H_

/**
 ****************************************************************************************
 * @addtogroup APP_BOND_DB
 *
 * @brief
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gapc_task.h"
#include "gap.h"
#include "co_bt.h"
#include "user_periph_setup.h"
#include "user_config.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// SPI FLASH and I2C EEPROM data offset
#if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    #define APP_BOND_DB_DATA_OFFSET     (0x1E000)
#elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
    #define APP_BOND_DB_DATA_OFFSET     (0x8000)
#endif

// Max number of bonded peers
#define APP_BOND_DB_MAX_BONDED_PEERS    (5)

// Database version
#define BOND_DB_VERSION                 (0x0001)

// Start and end header used to mark the bond data in memory
#define BOND_DB_HEADER_START            ((0x1234) + BOND_DB_VERSION)
#define BOND_DB_HEADER_END              ((0x4321) + BOND_DB_VERSION)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
enum bond_db_entry_flags
{
    BOND_DB_ENTRY_IRK_PRESENT       = (1 << 0),
    BOND_DB_ENTRY_LCSRK_PRESENT     = (1 << 1),
    BOND_DB_ENTRY_RCSRK_PRESENT     = (1 << 2),
};

struct bond_db_data
{
    uint8_t valid;
    uint8_t flags;

    struct gapc_ltk ltk;      // LTK
    struct gapc_irk irk;      // IRK
    struct gap_sec_key lcsrk; // Local CSRK
    struct gap_sec_key rcsrk; // Remote CSRK

    struct gap_bdaddr bdaddr; // Remote address

    // authentication level
    uint8_t auth;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Do initial fetch of stored bond data.
 * @return void
 ****************************************************************************************
 */
void bond_db_init(void);

/**
 ****************************************************************************************
 * @brief Store bond data.
 * @param[in] data    Pointer to the data to be stored.
 * @return void
 ****************************************************************************************
 */
void bond_db_store(struct bond_db_data *data);

/**
 ****************************************************************************************
 * @brief Look up bond data by EDIV, RAND_NB. This is used when security is requested
 *        in order to retrieve the LTK.
 * @param[in] rand_nb    RAND_NB
 * @param[in] ediv       EDIV
 * @return Pointer to the bond data if they were found. Otherwise null.
 ****************************************************************************************
 */
const struct bond_db_data* bond_db_lookup_by_ediv(const struct rand_nb *rand_nb,
                                                  uint16_t ediv);

/**
 ****************************************************************************************
 * @brief Clear bond data.
 * @return void
 ****************************************************************************************
 */
void bond_db_clear(void);

/// @} APP_BOND_DB

#endif // _APP_BOND_DB_H_
