/**
 ****************************************************************************************
 *
 * @file attm_cfg.h
 *
 * @brief Header file - ATTMCFG.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTM_CFG_H_
#define ATTM_CFG_H_

/**
 ****************************************************************************************
 * @addtogroup ATTMCFG Settings
 * @ingroup ATTM
 * @brief ATTM Configuration file
 *
 * The ATTMCFG is the attribute configuration holder for @ref ATTM "ATTM".
 *
 * @{
 ****************************************************************************************
 */
 
/*
 * INCLUDES
 ****************************************************************************************
 */
 
#include "rwip_config.h"

/*
 * DEFINES
 ****************************************************************************************
 */
 
/// Maximum Transmission Unit
#define ATT_DEFAULT_MTU                                 (23)
/// 30 seconds transaction timer
#define ATT_TRANS_RTX                                   (0x0BB8)
/// Acceptable encryption key size - strict access
#define ATT_SEC_ENC_KEY_SIZE                            (0x10)

/// @} ATTMCFG

#endif // ATTM_CFG_H_
