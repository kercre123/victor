/**
 ****************************************************************************************
 *
 * @file app_api.h
 *
 * @brief app - project api header file.
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

#ifndef _APP_API_H_
#define _APP_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#include "gapc_task.h"
#include "gapm_task.h"
#include "gattm_task.h"
#include "gattc_task.h"
#include "smpc_task.h"

#include "app.h"
#include "app_task.h"
#include "app_user_config.h"
#include "app_entry_point.h"
#include "app_default_handlers.h"
#include "app_callback.h"
#include "app_easy_gap.h"
#include "app_easy_msg_utils.h"
#include "app_easy_security.h"
#include "app_easy_timer.h"
#include "app_mid.h"
#include "app_msg_utils.h"

#include "arch_api.h"
#include "arch_wdg.h"

#endif // _APP_API_H_
