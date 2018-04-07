/**
 ****************************************************************************************
 *
 * @file gap_task.h
 *
 * @brief Header file - GAP_TASK.
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
 
#ifndef GAP_TASK_H_
#define GAP_TASK_H_
/**
 ****************************************************************************************
 * @addtogroup GAPTASK Task
 * @ingroup GAP
 * @brief Handles ALL messages to/from GAP block.
 *
 * The GAPTASK is responsible for managing the messages coming from
 * the link layer, application layer and host-level layers. The task
 * handling includes device discovery, bonding, connection establishment,
 * link termination and name discovery.
 *
 * Messages can originate from @ref L2C "L2C", @ref ATT "ATT", @ref SMP "SMP",
 * @ref LLM "LLM" and @ref LLC "LLC"
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

//#include <stdbool.h>
#include "gapc_task.h"
#include "gapm_task.h"

#define TASK_APP    TASK_GTL
/// @} GAPTASK

#endif // GAP_TASK_H_
