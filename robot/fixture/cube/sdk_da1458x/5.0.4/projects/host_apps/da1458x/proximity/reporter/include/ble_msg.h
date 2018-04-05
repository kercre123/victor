/**
 ****************************************************************************************
 *
 * @file ble_msg.h
 *
 * @brief Header file for BLE messages functions.
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

#ifndef BLE_MSG_H_
#define BLE_MSG_H_

#include "rwble_config.h"

typedef struct {
  unsigned short bType;
  unsigned short bDstid;
  unsigned short bSrcid;
  unsigned short bLength;
} ble_hdr;


typedef struct {
  unsigned short bType;
  unsigned short bDstid;
  unsigned short bSrcid;
  unsigned short bLength;
  unsigned char  bData[1];
} ble_msg;


void *BleMsgAlloc(unsigned short id, unsigned short dest_id,
                   unsigned short src_id, unsigned short param_len);

void BleSendMsg(void *msg);

void HandleBleMsg(ble_msg *msg);

void BleReceiveMsg(void);

void BleFreeMsg(void *msg);


#endif //BLE_MSG_H_
