/**
 ****************************************************************************************
 *
 * @file bms_common.h
 *
 * @brief Header file - Bond Management Service common types.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BMS_COMMON_H_
#define _BMS_COMMON_H_

/**
 ****************************************************************************************
 * @addtogroup BMS Bond Management Service
 * @ingroup BMS
 * @brief Bond Management Service
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BM_CLIENT || BLE_BM_SERVER)

#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// BMS v1.0.0 - OpCode definitions for BM Control Point Characteristic
#define BMS_OPCODE_DEL_BOND                         (0x03)
#define BMS_OPCODE_DEL_ALL_BOND                     (0x06)
#define BMS_OPCODE_DEL_ALL_BOND_BUT_PEER            (0x09)

/// BMS v1.0.0 - BM Feature Bit Description
#define BMS_FEAT_DEL_BOND                           (0x00000010)
#define BMS_FEAT_DEL_BOND_AUTHZ                     (0x00000020)
#define BMS_FEAT_DEL_ALL_BOND                       (0x00000400)
#define BMS_FEAT_DEL_ALL_BOND_AUTHZ                 (0x00000800)
#define BMS_FEAT_DEL_ALL_BOND_BUT_PEER              (0x00010000)
#define BMS_FEAT_DEL_ALL_BOND_BUT_PEER_AUTHZ        (0x00020000)

/// BMS v1.0.0 - Attribute Protocol Error codes
#define BMS_ATT_OPCODE_NOT_SUPPORTED                (0x80)
#define BMS_ATT_OPERATION_FAILED                    (0x81)

#define BMS_FEAT_DEL_BOND_SUPPORTED                 (BMS_FEAT_DEL_BOND |\
                                                    BMS_FEAT_DEL_BOND_AUTHZ)

#define BMS_FEAT_DEL_ALL_BOND_SUPPORTED             (BMS_FEAT_DEL_ALL_BOND |\
                                                    BMS_FEAT_DEL_ALL_BOND_AUTHZ)

#define BMS_FEAT_DEL_ALL_BOND_BUT_PEER_SUPPORTED    (BMS_FEAT_DEL_ALL_BOND_BUT_PEER |\
                                                    BMS_FEAT_DEL_ALL_BOND_BUT_PEER_AUTHZ)

/*
 * STRUCTURES DEFINITIONS
 ****************************************************************************************
 */

struct bms_ctrl_point_op
{
    /// Operation code
    uint8_t op_code;

    /// Operand length
    uint16_t operand_length;

    /// Operand
    uint8_t operand[__ARRAY_EMPTY];
};

#endif /* #if (BLE_BM_CLIENT || BLE_BM_SERVER) */

/// @} BMS_common

#endif /* _BMS_COMMON_H_ */
