/**
 ****************************************************************************************
 *
 * @file user_sleepmode_task.h
 *
 * @brief Sleep mode project header file.
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

#ifndef _USER_SLEEPMODE_TASK_H_
#define _USER_SLEEPMODE_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
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
 
#include "custs1_task.h"
#include "ke_msg.h"
#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

enum
{
    CUSTS1_BTN_STATE_RELEASED = 0,
    CUSTS1_BTN_STATE_PRESSED,
};

enum
{
    CUSTS1_CP_CMD_PWM_DISABLE = 0,
    CUSTS1_CP_CMD_PWM_ENABLE,
    CUSTS1_CP_CMD_ADC_VAL_2_DISABLE,
    CUSTS1_CP_CMD_ADC_VAL_2_ENABLE,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct app_proj_env_tag
{
    uint8_t custs1_pwm_enabled;
    uint8_t custs1_adcval2_enabled;
    uint8_t custs1_btn_state;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Control point write indication handler.
 * @param[in] msgid    Id of the message received.
 * @param[in] param    Pointer to the parameters of the message.
 * @param[in] dest_id  ID of the receiving task instance.
 * @param[in] src_id   ID of the sending task instance.
 * @return void
 ****************************************************************************************
 */
void user_custs1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                     struct custs1_val_write_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Enable peripherals used by application.
 * @return void
 ****************************************************************************************
 */
void user_app_enable_periphs(void);


/**
 ****************************************************************************************
 * @brief Disable peripherals used by application.
 * @return void
 ****************************************************************************************
 */
void user_app_disable_periphs(void);

/// @} APP

#endif // _USER_SLEEPMODE_TASK_H_

