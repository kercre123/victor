/**
 ****************************************************************************************
 *
 * @file app_prf_perm_types.h
 *
 * @brief app - Service access permission rights api.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_PRF_PERM_TYPES_H_
#define _APP_PRF_PERM_TYPES_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "attm.h"                       // Service access permissions 

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Service access rights
typedef enum app_prf_srv_perm
{
    /// Disable access
    SRV_PERM_DISABLE  = PERM(SVC, DISABLE),
    /// Enable access
    SRV_PERM_ENABLE   = PERM(SVC, ENABLE),
    /// Access Requires Unauthenticated link
    SRV_PERM_UNAUTH   = PERM(SVC, UNAUTH),
    /// Access Requires Authenticated link
    SRV_PERM_AUTH     = PERM(SVC, AUTH),
    /// Access Requires authorization
    SRV_PERM_AUTHZ    = PERM(SVC, AUTHZ),
    
}app_prf_srv_perm_t;

typedef struct app_prf_srv_sec
{
    enum KE_TASK_TYPE   task_id;
    app_prf_srv_perm_t  perm;
    
}app_prf_srv_sec_t;

extern app_prf_srv_perm_t *app_srv_perm;

/*
 * DEFINES
 ****************************************************************************************
 */
// Maximum number for profiles (TASK_IDs) that can be included in a user application
#define PRFS_TASK_ID_MAX    (10)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Returns the Service permission set by user. If user has not set any service
 * permission, the default "ENABLE" is used.
 * @param[in] task_id         Task ID of the profile service
 * @return app_prf_srv_perm_t Service access permission rights
 ****************************************************************************************
*/
app_prf_srv_perm_t get_user_prf_srv_perm(enum KE_TASK_TYPE task_id);

/*
 ****************************************************************************************
 * @brief Sets the service permission access rights for a profile.
 * @param[in] task_id  Task ID of the profile service
 * @param[in] srv_perm Service permission access rights
 * @return void
 ****************************************************************************************
 */
void app_set_prf_srv_perm(enum KE_TASK_TYPE task_id, app_prf_srv_perm_t srv_perm);

/*
 ****************************************************************************************
 * @brief Initialises service permissions to "ENABLE". Should be called once upon app initialization
 * @return void
 ****************************************************************************************
 */
void prf_init_srv_perm(void);

/// @} APP

#endif // _APP_PRF_TYPES_H_
