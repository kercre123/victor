/**
 ****************************************************************************************
 *
 * @file app_easy_security.h
 *
 * @brief Application security helper functions header file.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_EASY_SECURITY_H_
    #define _APP_EASY_SECURITY_H_

/**
 ****************************************************************************************
 * @addtogroup APP_SECURITY
 * @ingroup
 *
 * @brief Application security helper functions header file.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "co_bt.h"
#include "gap.h"
#include "gapm.h"
#include "gapc_task.h"
#include "app_security.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Get confirm requested bond information message in pairing procedure.
 * @param[in] connection_idx Connection Id index
 * @return gapc_bond_cfm     Pointer to GAPC_BOND_CFM message
 ****************************************************************************************
*/
struct gapc_bond_cfm* app_easy_security_pairing_rsp_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get confirm requested bond information message using term key exchange.
 * @param[in] connection_idx Connection Id index
 * @return gapc_bond_cfm     Pointer to GAPC_BOND_CFM message
 ****************************************************************************************
*/
struct gapc_bond_cfm* app_easy_security_tk_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get confirm requested bond information message using connection signature resolving key exchange.
 * @param[in] connection_idx Connection Id index
 * @return gapc_bond_cfm     Pointer to GAPC_BOND_CFM message
 ****************************************************************************************
*/
struct gapc_bond_cfm* app_easy_security_csrk_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get confirm requested bond information message using long term key exchange.
 * @param[in] connection_idx Connection Id index
 * @return gapc_bond_cfm     Pointer to GAPC_BOND_CFM message
 ****************************************************************************************
*/
struct gapc_bond_cfm* app_easy_security_ltk_exch_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get confirm requested encryption information message.
 * @param[in] connection_idx Connection Id index
 * @return gapc_encrypt_cfm  Pointer to GAPC_ENCRYPT_CFM message
 ****************************************************************************************
*/
struct gapc_encrypt_cfm* app_easy_security_encrypt_cfm_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Set long term key loaded from security environment.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_set_ltk_exch_from_sec_env(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Set long term key.
 * @param[in] connection_idx         Connection Id index
 * @param[in] long_term_key          Pointer to long term key
 * @param[in] encryption_key_size    Encryption key size
 * @param[in] keylen                 Length of the key
 * @param[in] random_number          Pointer to random number
 * @param[in] encryption_diversifier Encryption diversifier
 * @return void
 ****************************************************************************************
*/
void app_easy_security_set_ltk_exch(uint8_t connection_idx, uint8_t* long_term_key, uint8_t encryption_key_size, uint8_t* random_number, uint16_t encryption_diversifier);

/**
 ****************************************************************************************
 * @brief Indicate that a long term key has been found for the peer device
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_set_encrypt_req_valid(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Indicate that a long term key has not been found for the peer device
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_set_encrypt_req_invalid(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Validate encryption request information against security environment variable.
 * @param[in] connection_idx       Connection Id index
 * @param[in] gapc_encrypt_req_ind Pointer to GAPC_ENCRYPT_REQ_IND message
 * @return true if succeeded, else false
 ****************************************************************************************
*/
bool app_easy_security_validate_encrypt_req_against_env(uint8_t connection_idx, struct gapc_encrypt_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Send confirm requested bond information in pairing procedure.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_send_pairing_rsp(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Send bond confirmation message using TK exchange.
 * @param[in] connection_idx Connection Id index
 * @param[in] key            Pointer to the key that will be sent over TK exchange message.
 * @param[in] length         Length of the pass key in octets
 * @return void
 * @note                     The #key can be either a 6-digit (4 octets) pass key or an
 *                           OOB provided key. The max size of the OOB key is 128-bit
 *                          (16 octets).
 ****************************************************************************************
*/
void app_easy_security_tk_exch(uint8_t connection_idx, uint8_t *key, uint8_t length);

/**
 ****************************************************************************************
 * @brief Send confirm requested bond information using connection signature resolving key exchange.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_csrk_exch(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Send confirm requested bond information using long term key exchange.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_ltk_exch(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Send encryption confirmation message.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_encrypt_cfm(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get the active security request message.
 * @param[in] connection_idx Connection Id index
 * @return gapc_security_cmd  pointer to the active security request message
 ****************************************************************************************
*/
struct gapc_security_cmd* app_easy_security_request_get_active(uint8_t connection_idx);
/**
 ****************************************************************************************
 * @brief Send encryption request message.
 * @param[in] connection_idx Connection Id index
 * @return void
 ****************************************************************************************
*/
void app_easy_security_request(uint8_t connection_idx);

/// @} APP_SECURITY

#endif // _APP_EASY_SECURITY_H_
