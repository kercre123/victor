/**
 ****************************************************************************************
 *
 * @file atts.h
 *
 * @brief Header file - ATTS.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTS_H_
#define ATTS_H_

/**
 ****************************************************************************************
 * @addtogroup ATTS Attribute Server
 * @ingroup ATT
 * @brief Attribute Protocol Server
 *
 * The ATTS module is responsible for handling messages intended for the attribute
 * profile server. It has defined interfaces with @ref ATTM "ATTM".
 *
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_ATTS)
#include <stdio.h>
#include <string.h>
#include "ke_task.h"
#include "atts_task.h"
#include "att.h"
#include "l2cc_pdu.h"
/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Prepare write data part structure
struct atts_prep_data
{
    /// List header for chaining
    struct co_list_hdr hdr;
    /// attribute handle
    uint16_t attr_hdl;
    /// data offset
    uint16_t offset;
    /// length of the partial data
    uint16_t len;
    /// data value part of the prepare write
    uint8_t data[__ARRAY_EMPTY];
};


/// Attribute server environment variables
struct atts_env_tag
{
    /** This is used to merge save all the prepare write request received ,
     *  before receiving the execute or cancel or disconnection. */
    struct co_list atts_prep_wr_req_list;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the ATTS module.
 * This function performs all the initializations of the ATTS module.
 *
 * @param[in] reset  true if it's requested by a reset; false if it's boot initialization
 *
 ****************************************************************************************
 */
void atts_init(bool reset);

/**
 ****************************************************************************************
 * @brief prepare attribute server environment for created connection
 *
 * @param[in] conidx        connection index
 *
 ****************************************************************************************
 */
void atts_create(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Created link connection parameters (from bond data) has been set, connection
 * ready to be used.
 *
 * @param[in] conidx     Connection Index
 *
 ****************************************************************************************
 */
void atts_con_enable(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Clean up connection data.
 *
 * @param[in] conidx        connection index
 *
 ****************************************************************************************
 */
void atts_cleanup(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Update the value of the specified attribute.
 *
 * After updating the value, the function sends an indication to the upper layer profile
 * or protocol task registered for the specified attribute.
 *
 * @param[in] conidx   connection index
 * @param[in] value    Pointer to the data to be written
 * @param[in] length   The length to be written
 * @param[in] offset   The offset at which the data has to be written
 * @param[in] handle   The handle of the attribute that has to be written
 * @param[in] response Task shall send back a write response command if true.
 * @param[in] last     Informs that it's last request of a multiple prepare write request.
 *
 *
 ****************************************************************************************
 */
void atts_update_elmt(uint8_t conidx, uint8_t *value, uint16_t length, uint16_t offset,
                     uint16_t handle, bool response, bool last);


/**
 ****************************************************************************************
 * @brief Store temporary data.
 *
 * @param[in] conidx            connection index
 * @param[in] buf               pointer to buffer data
 * @param[in] len               length of the buffer
 * @param[in] offset            offset value
 * @param[in] handle            connection handle
 *
 * @return If the data can be stored or not.
 ****************************************************************************************
 */
uint8_t atts_store_prep_data(uint8_t conidx, uint8_t *buf, uint16_t len, uint16_t offset, uint16_t handle);

/**
 * @brief  Clear allocated prepare write temporary data.
 *
 * @param[in] conidx            connection index
 */
void atts_clear_prep_data(uint8_t conidx);

/// @} ATTS
#endif /* (BLE_ATTS) */
#endif // ATTS_H_
