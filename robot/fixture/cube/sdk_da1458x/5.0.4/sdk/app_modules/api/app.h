/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef _APP_H_
#define _APP_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration
#include "rwble_hl_config.h"
#include "ke_msg.h"
#include "app_msg_utils.h"
#include "app_easy_msg_utils.h"
#include "app_easy_timer.h"

#if (BLE_APP_PRESENT)

#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>           // Common BT Definitions
#include "arch.h"            // Platform Definitions
#include "gapc.h"
#include "app_easy_gap.h"

#if (BLE_PROX_REPORTER)
#include "app_proxr.h"
#include "app_proxr_task.h"
#endif

#if (BLE_DIS_SERVER)
#include "app_diss.h"
#include "app_diss_task.h"
#endif

#if (BLE_BAS_SERVER)
#include "app_bass.h"
#include "app_bass_task.h"
#endif     

#if (BLE_SPOTA_RECEIVER)
#include "app_spotar.h"
#include "app_spotar_task.h"
#endif     

/*
 * DEFINES
 ****************************************************************************************
 */

/// Advertising data maximal length
#define APP_ADV_DATA_MAX_SIZE               (ADV_DATA_LEN - 3)

/// Scan Response data maximal length
#define APP_SCAN_RESP_DATA_MAX_SIZE         (SCAN_RSP_DATA_LEN)

/// Max connections supported by application task
#define APP_EASY_MAX_ACTIVE_CONNECTION      (1)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// APP Task messages
enum APP_MSG
{
    APP_MODULE_INIT_CMP_EVT = KE_FIRST_MSG(TASK_APP),
    
#if BLE_PROX_REPORTER
    APP_PROXR_TIMER,
#endif //BLE_PROX_REPORTER

#if BLE_BAS_SERVER
    APP_BASS_TIMER,
    APP_BASS_ALERT_TIMER,
#endif //BLE_BAS_SERVER

#if HAS_MULTI_BOND
    APP_ALT_PAIR_TIMER,
#endif //HAS_MULTI_BOND

    APP_CREATE_TIMER,
    APP_CANCEL_TIMER,
    APP_MODIFY_TIMER,
    //Do not alter the order of the next messages 
    //they are considered a range
    APP_TIMER_API_MES0,
    APP_TIMER_API_MES1=APP_TIMER_API_MES0+1,
    APP_TIMER_API_MES2=APP_TIMER_API_MES0+2,
    APP_TIMER_API_MES3=APP_TIMER_API_MES0+3,
    APP_TIMER_API_MES4=APP_TIMER_API_MES0+4,
    APP_TIMER_API_MES5=APP_TIMER_API_MES0+5,
    APP_TIMER_API_MES6=APP_TIMER_API_MES0+6,
    APP_TIMER_API_MES7=APP_TIMER_API_MES0+7,
    APP_TIMER_API_MES8=APP_TIMER_API_MES0+8,
    APP_TIMER_API_MES9=APP_TIMER_API_MES0+9,
    APP_TIMER_API_LAST_MES=APP_TIMER_API_MES9,
    
    APP_MSG_UTIL_API_MES0,
    APP_MSG_UTIL_API_MES1=APP_MSG_UTIL_API_MES0+1,
    APP_MSG_UTIL_API_MES2=APP_MSG_UTIL_API_MES0+2,
    APP_MSG_UTIL_API_MES3=APP_MSG_UTIL_API_MES0+3,
    APP_MSG_UTIL_API_MES4=APP_MSG_UTIL_API_MES0+4,
    APP_MSG_UTIL_API_LAST_MES=APP_MSG_UTIL_API_MES4

};

/// Application environment structure
struct app_env_tag
{
    /// Connection handle
    uint16_t conhdl;
    
    /// Connection Id
    uint8_t conidx; // Should be used only with KE_BUILD_ID()
    
    /// Connection active flag
    bool connection_active;
    
    /// Last initialized profile
    //uint8_t next_prf_init;

    /// Security enable flag
    bool sec_en;
    
    // Last paired peer address type 
    uint8_t peer_addr_type;
    
    // Last paired peer address 
    struct bd_addr peer_addr;

};

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env[APP_EASY_MAX_ACTIVE_CONNECTION];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the BLE application.
 * @return void
 ****************************************************************************************
 */
void app_init(void);

/**
 ****************************************************************************************
 * @brief Start placing services in the database.
 * @return true if succeeded, else false
 ****************************************************************************************
 */
bool app_db_init_start(void);

/**
 ****************************************************************************************
 * @brief Database initialization. Add a required service in the database.
 * @return true if succeeded, else false
 ****************************************************************************************
 */
bool app_db_init(void);

/**
 ****************************************************************************************
 * @brief Start a kernel timer.
 * @param[in] timer_id Timer Id
 * @param[in] task_id Task Id
 * @param[in] delay Timer delay
 * @return void
 ****************************************************************************************
 */
void app_timer_set(ke_msg_id_t const timer_id, ke_task_id_t const task_id, uint16_t delay);



/**
 ****************************************************************************************
 * @brief Calls all the enable function of the profile registered in prf_func, 
 *        custs_prf_func and user_prf_func.
 * @param[in] conhdl The connection handle
 * @return void
 ****************************************************************************************
 */
void app_prf_enable (uint16_t conhdl);

#endif //(BLE_APP_PRESENT)

/// @} APP

#endif // _APP_H_
