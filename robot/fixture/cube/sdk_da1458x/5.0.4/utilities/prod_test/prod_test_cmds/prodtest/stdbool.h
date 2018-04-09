/**
 ****************************************************************************************
 *
 * @file stdbool.h
 *
 * @brief stdbool.h implementation for C compilers that do not support this header.
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

#ifndef STDBOOL_H_
#define STDBOOL_H_


#ifndef __cplusplus
    #define bool char
    #define true 1
    #define false 0
#endif


#endif // STDBOOL_H_

