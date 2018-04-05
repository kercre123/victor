/**
 ****************************************************************************************
 *
 * @file prf_utils_128.h
 *
 * @brief  Header file - Profile Utilities for attributes with 128-bit UUIDs
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */


#ifndef _PRF_UTILS_128_H_
#define _PRF_UTILS_128_H_

/**
 ****************************************************************************************
 * @addtogroup PRF_UTILS
 * @ingroup PROFILE
 *
 * @brief Definitions of shared profiles functions that can be used by several profiles
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_CLIENT_PRF)
#include "ke_msg.h"
#include "prf_types.h"
#include "gattc_task.h"
#include "gapc.h"
#include "gapc_task.h"
#include "attm_db.h"
#endif /* (BLE_CLIENT_PRF) */

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
 

#if (BLE_CLIENT_PRF)

/// Characteristic definition
struct prf_char_def_128
{
    /// Characteristic UUID
    uint8_t uuid[ATT_UUID_128_LEN];
    /// Requirement Attribute Flag
    uint8_t req_flag;
    /// Mandatory Properties
    uint8_t prop_mand;
};

/**
 ****************************************************************************************
 * @brief Request service discovery with 128-bit UUID on peer device.
 *
 * This request will be used to retrieve start and end handles of the service.
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param uuid_128 128-bit service UUID
 ****************************************************************************************
 */
void prf_disc_svc_send_128(struct prf_con_info* con_info, uint8_t *uuid_128);

/**
 ****************************************************************************************
 * @brief Request service characteristic discovery on peer device.
 *
 * This request service to provide all its service characteristic information.
 *
 * @param con_info Pointer to connection information (connection handle, app task id,
 *                 profile task id)
 *
 * @param uuid_128 128-bit characteristic UUID
 ****************************************************************************************
 */
void prf_disc_char_by_uuid_128_send(struct prf_con_info* con_info, uint8_t *uuid_128);

/**
 ****************************************************************************************
 * @brief Check validity for service characteristic with 128-bit UUID
 *
 * For each characteristic in service it verifies handles.
 *
 * If some handles are not present, it checks if they shall be present or they are optional.
 *
 * @param nb_chars   Number of Characteristics in the service
 * @param chars      Characteristics values (char handles, val handles, properties)
 * @param chars_req  Characteristics requirements.
 *
 * @return 0x1 if service is valid, 0x00 else.
 ****************************************************************************************
 */																		
uint8_t prf_check_svc_char_validity_128(uint8_t nb_chars,
                                    const struct prf_char_inf* chars,
                                    const struct prf_char_def_128* chars_req);

/**
 ****************************************************************************************
 * @brief Look for requested characteristics in the list provided by the
 * gattc_disc_char_ind message.
 *
 * @param svc_ehdl         End handle of the service for which characteristics are discovered
 * @param nb_chars         Length of provided arrays (chars and chars_req)
 * @param chars            Characteristics
 * @param chars_req        Characteristics requirements
 * @param param            List of found characteristics
 * @param last_found_char  Characteristics code of the last characteristic that has been found.
 ****************************************************************************************
 */
void prf_search_chars_128(uint16_t svc_ehdl, uint8_t nb_chars,
                      struct prf_char_inf* chars, const struct prf_char_def_128* chars_req,
                      const struct gattc_disc_char_ind* param,
                      uint8_t* last_found_char);

#endif //(BLE_CLIENT_PRF)


/// @} prf_utils

#endif /* _PRF_UTILS_128_H_ */
