/**
 ****************************************************************************************
 *
 * @file lld_util.h
 *
 * @brief Link layer driver utilities definitions
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */
#ifndef LLD_UTIL_H_
#define LLD_UTIL_H_
/**
 ****************************************************************************************
 * @addtogroup LLDUTIL
 * @ingroup LLD
 * @brief Link layer driver utilities definitions
 *
 * full description
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "co_bt.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief  Get the BD address
 *
 * @param[out]    Pointer to BD address buffer
 ****************************************************************************************
 */
void lld_util_get_bd_address(struct bd_addr *bd_addr);

/**
 ****************************************************************************************
 * @brief  Set the BD address
 *
 * @param[in]    bd_addr    Pointer to BD address buffer
 * @param[in]    type       Address type (0:public, 1:random)
 ****************************************************************************************
 */
void lld_util_set_bd_address(struct bd_addr *bd_addr, uint8_t type);

/// @} LLDUTIL
#endif // LLD_UTIL_H_
