/**
 ****************************************************************************************
 *
 * @file app_callback.h
 *
 * @brief Application callbacks definitions. 
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

#ifndef _APP_CALLBACK_H_
#define _APP_CALLBACK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "gapm.h"
#include "attm.h"
#include "gapc_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct app_callbacks
{
    void (*app_on_connection)(const uint8_t, struct gapc_connection_req_ind const *);
    void (*app_on_disconnect)(struct gapc_disconnect_ind const *);
    void (*app_on_update_params_rejected)(const uint8_t);
    void (*app_on_update_params_complete)(void);
    void (*app_on_set_dev_config_complete)(void);
    void (*app_on_adv_nonconn_complete)(const uint8_t);
    void (*app_on_adv_undirect_complete)(const uint8_t);
    void (*app_on_adv_direct_complete)(const uint8_t);
    void (*app_on_db_init_complete)(void);
    void (*app_on_scanning_completed)(const uint8_t);
    void (*app_on_adv_report_ind)(struct gapm_adv_report_ind const *);
    void (*app_on_connect_failed)(void);
#if (BLE_APP_SEC)
    void (*app_on_pairing_request)(const uint8_t, struct gapc_bond_req_ind const *);
    void (*app_on_tk_exch_nomitm)(const uint8_t, struct gapc_bond_req_ind const *);
    void (*app_on_irk_exch)(struct gapc_bond_req_ind const *);
    void (*app_on_csrk_exch)(const uint8_t, struct gapc_bond_req_ind const *);
    void (*app_on_ltk_exch)(const uint8_t, struct gapc_bond_req_ind const *);
    void (*app_on_pairing_succeded)(void);
    void (*app_on_encrypt_ind)(const uint8_t);
    void (*app_on_mitm_passcode_req)(const uint8_t);
    void (*app_on_encrypt_req_ind)(const uint8_t, struct gapc_encrypt_req_ind const *);
    void (*app_on_security_req_ind)(const uint8_t);
#endif
};

struct profile_callbacks
{
    void (*on_batt_level_upd_cfm)(uint8_t connection_idx, uint8_t status);
    void (*on_batt_level_ntf_cfg_ind)(uint8_t connection_idx, uint16_t ntf_cfg, uint8_t bas_instance);
    void (*on_findt_alert_ind)(uint8_t connection_idx, uint8_t alert_lvl);
    void (*on_proxr_level_upd_ind)(uint8_t connection_idx, uint8_t alert_lvl, uint8_t char_code);
    void (*on_proxr_lls_alert_ind)(uint8_t connection_idx, uint8_t alert_lvl);
    void (*on_spotar_status_change)(uint8_t status);
};

/*
 * DEFINES
 ****************************************************************************************
 */

#define EXECUTE_PROFILE_CALLBACK_VOID(func)     {if (user_profile_callbacks.func!=NULL)\
                    user_profile_callbacks.func();\
    }

#define EXECUTE_PROFILE_CALLBACK_PARAM(func,param)     {if (user_profile_callbacks.func!=NULL)\
                    user_profile_callbacks.func(param);\
    }
#define EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2(func,param1,param2)     {if (user_profile_callbacks.func!=NULL)\
                    user_profile_callbacks.func(param1,param2);\
    }

#define EXECUTE_PROFILE_CALLBACK_PARAM1_PARAM2_PARAM3(func,param1,param2,param3)     {if (user_profile_callbacks.func!=NULL)\
                    user_profile_callbacks.func(param1,param2,param3);\
    }
  
#define EXECUTE_CALLBACK_VOID(fnc)  {if (user_app_callbacks.fnc!=NULL)\
                    user_app_callbacks.fnc();\
    }     
    
#define EXECUTE_CALLBACK_PARAM(func,param)     {if (user_app_callbacks.func!=NULL)\
                    user_app_callbacks.func(param);\
    }

#define EXECUTE_CALLBACK_PARAM1_PARAM2(func,param1,param2)     {if (user_app_callbacks.func!=NULL)\
                    user_app_callbacks.func(param1,param2);\
    }

#endif // _APP_CALLBACK_H_
