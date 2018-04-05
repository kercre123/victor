/**
 ****************************************************************************************
 *
 * @file attc.h
 *
 * @brief Header file - ATTC.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTC_H_
#define ATTC_H_

/**
 ****************************************************************************************
 * @addtogroup ATTC Attribute Client
 * @ingroup ATT
 * @brief Attribute Protocol Client
 *
 * The ATTC module is responsible for handling messages intended for the attribute
 * profile client. It has defined interfaces with @ref ATTM "ATTM".
 *
 * @{
 *
 ****************************************************************************************
 */
//#include "rwip_config.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ke_task.h"
#include "attm.h"
#if (BLE_ATTC)
/*
 * DEFINES
 ****************************************************************************************
 */
/// maximum number of ATTC instances
#define ATTC_IDX_MAX                                    BLE_CONNECTION_MAX

/*
 * DATA STRUCTURES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ATTC module.
 * This function performs all the initializations of the ATTC module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void attc_init(bool reset);

/**
 ****************************************************************************************
 * @brief Initialization of the Attribute client connection
 *
 * @param[in] conidx            connection index
 *
 ****************************************************************************************
 */
void attc_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Created link connection parameters (from bond data) has been set, connection
 * ready to be used.
 *
 * @param[in] conidx     Connection Index
 *
 ****************************************************************************************
 */
void attc_con_enable(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Clean-up of the ATTC module.
 * This function performs all the clean-up of the ATTC module.
 *
 * @param[in] conidx            connection index
 *
 ****************************************************************************************
 */
void attc_cleanup(uint8_t conidx);
#endif /* (BLE_ATTC) */
/// @} ATTC
#endif // ATT_H_
