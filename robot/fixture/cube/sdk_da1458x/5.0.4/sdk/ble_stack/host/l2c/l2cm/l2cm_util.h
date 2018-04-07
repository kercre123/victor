/**
 ****************************************************************************************
 *
 * @file l2cm_util.h
 *
 * @brief Header file - L2CMUTIL.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef L2CM_UTIL_H_
#define L2CM_UTIL_H_

/**
 ****************************************************************************************
 * @addtogroup L2CMUTIL Utility
 * @ingroup L2CM
 * @brief Contains utility functions for L2CAP Manager.
 *
 * The L2CMUTIL is responsible for providing utility functions for
 * @ref L2CM "L2CM", in order to accomplish defined L2CM functionalities.
 *
 * @{
 ****************************************************************************************
 */
#include "rwip_config.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "l2cm.h"
#include "l2cm_task.h"

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/*
 * DEFINES
 ****************************************************************************************
 */
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Sets link layer buffer size capacity
 *
 * @param[in] pkt_len           length of the packet
 * @param[in] nb_acl            number of acl
 *
 ****************************************************************************************
 */
void l2cm_set_link_layer_buff_size(uint16_t pkt_len, uint16_t nb_acl);
#endif /* #if (BLE_CENTRAL || BLE_PERIPHERAL) */
/// @} L2CMUTIL
#endif // L2CM_UTIL_H_
