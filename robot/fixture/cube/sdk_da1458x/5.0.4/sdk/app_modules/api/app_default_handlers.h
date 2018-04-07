/**
 ****************************************************************************************
 *
 * @file app_default_handlers.h
 *
 * @brief Application default handlers header file.
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

#ifndef _APP_DEFAULT_HANDLERS_H_
#define _APP_DEFAULT_HANDLERS_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup
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

#include <stdio.h>
#include "app_callback.h"
#include "gapc_task.h"

/**
 ****************************************************************************************
 * @brief Possible advertise scenarios.
 ****************************************************************************************
 */
enum default_advertise_scenario{
    DEF_ADV_FOREVER,
    DEF_ADV_WITH_TIMEOUT};

/**
 ****************************************************************************************
 * @brief Possible security request scenarios.
 ****************************************************************************************
 */    
enum default_security_request_scenario{
    DEF_SEC_REQ_NEVER,
    DEF_SEC_REQ_ON_CONNECT};

/**
 ****************************************************************************************
 * @brief Configuration options for the default_handlers.
 ****************************************************************************************
 */    
struct default_handlers_configuration
{
	//Configure the advertise operation used by the default handlers
	enum default_advertise_scenario adv_scenario;

	//Configure the advertise period in case of DEF_ADV_WITH_TIMEOUT.
	//It is measured in timer units (10ms). Use MS_TO_TIMERUNITS macro to convert
	//from milliseconds (ms) to timer units.
	int16_t advertise_period;

	//Configure the security start operation of the default handlers
	//if the security is enabled (CFG_APP_SECURITY)
	enum default_security_request_scenario security_request_scenario;
};

    
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
    
/**
 ****************************************************************************************
 * @brief Default function called on initialization event.
 * @return void
 ****************************************************************************************
 */
void default_app_on_init(void);

/**
 ****************************************************************************************
 * @brief Default function called on connection event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_CONNECTION_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_connection(uint8_t connection_idx, struct gapc_connection_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on disconnection event.
 * @param[in] param          Pointer to GAPC_DISCONNECT_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_disconnect(struct gapc_disconnect_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on device configuration completion event.
 * @return void
 ****************************************************************************************
 */
void default_app_on_set_dev_config_complete(void);

/**
 ****************************************************************************************
 * @brief Default function called on database initialization completion event.
 * @return void
 ****************************************************************************************
 */
void default_app_on_db_init_complete(void);

/**
 ****************************************************************************************
 * @brief Default function called on pairing request event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_BOND_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_pairing_request(uint8_t connection_idx, struct gapc_bond_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on no-man-in-the-middle TK exchange event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_BOND_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_tk_exch_nomitm(uint8_t connection_idx, struct gapc_bond_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on CSRK exchange event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_BOND_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_csrk_exch(uint8_t connection_idx, struct gapc_bond_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on long term key exchange event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_BOND_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_ltk_exch(uint8_t connection_idx, struct gapc_bond_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on encryption request event.
 * @param[in] connection_idx Connection Id number
 * @param[in] param          Pointer to GAPC_ENCRYPT_REQ_IND message
 * @return void
 ****************************************************************************************
 */
void default_app_on_encrypt_req_ind(uint8_t connection_idx, struct gapc_encrypt_req_ind const *param);

/**
 ****************************************************************************************
 * @brief Default function called on advertising operation.
 * @return void
 ****************************************************************************************
 */
void default_advertise_operation(void);

/**
 ****************************************************************************************
 * @brief Structure containing the operations used by the default handlers.
 * @return void
 ****************************************************************************************
 */
struct default_app_operations {
    void (*default_operation_adv)(void);
};

/**
 ****************************************************************************************
 * @brief Macro used to call the default operation.
 * @param[in] the void operation to execute
 ****************************************************************************************
 */
#define EXECUTE_DEFAULT_OPERATION_VOID(func)     {if (user_default_app_operations.func!=NULL)\
                    user_default_app_operations.func();\
    }

/// @} APP

#endif // _APP_DEFAULT_HANDLERS_H_
