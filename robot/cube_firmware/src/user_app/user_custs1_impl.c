/**
 ****************************************************************************************
 *
 * @file user_custs1_impl.c
 *
 * @brief Peripheral project Custom1 Server implementation source code.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_api.h"
#include "app.h"
#include "user_custs_config.h"
#include "user_custs1_impl.h"
#include "user_peripheral.h"
#include "user_periph_setup.h"

#include "application_api.h"

static bool app_valid = false;
static bool app_running = false;
static timer_hnd app_timer = EASY_TIMER_INVALID_TIMER;
static const int app_location = 0x20005800;
static ApplicationMap* const app_current = (ApplicationMap*) app_location;
static uint8_t* app_write_address = (uint8_t*) app_location;
static const uint8_t xxtea_key[16] = {0x88, 0xdb, 0x37, 0x96, 0x76, 0x8f, 0x86, 0x91, 0x42, 0x24, 0xf4, 0x35, 0xc1, 0xfd, 0x7f, 0xd3};
static const uint8_t xxtea_nonce[16] = {0x3d, 0x70, 0x7a, 0x86, 0x10, 0x3c, 0xef, 0x4f, 0x83, 0x2f, 0xa8, 0xa6, 0x64, 0x20, 0xba, 0xdf};
static int write_offset = 0;

void app_send_version() {
  int length = app_valid ? sizeof(app_current->version) : 0;

  // Send the current loaded application version
  struct custs1_val_ntf_req* req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                    TASK_CUSTS1,
                                                    TASK_APP,
                                                    custs1_val_ntf_req,
                                                    length);

  req->conhdl = app_env->conhdl;
  req->handle = CUST1_IDX_LOADED_APP_VAL;
  req->length = length;

  if (app_valid) memcpy(req->value, app_current->version, sizeof(app_current->version));

  ke_msg_send(req);
}

void app_send_target(uint8_t length, const void* value) {
  // Send the current loaded application version
  struct custs1_val_ntf_req* req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                    TASK_CUSTS1,
                                                    TASK_APP,
                                                    custs1_val_ntf_req,
                                                    length);

  req->conhdl = app_env->conhdl;
  req->handle = CUST1_IDX_APP_READ_VAL;
  req->length = length;
  memcpy(req->value, value, length);

  ke_msg_send(req);
}

void app_tick() {
  if (!app_running) return ;
  app_current->AppTick();
  app_timer = app_easy_timer(1, app_tick);
}

void app_start() {
  if (!app_valid || app_running) return ;

  app_running = true;
  app_current->BLE_Send = app_send_target;
  app_current->AppInit();
  app_timer = app_easy_timer(1, app_tick);
}

void app_stop() {
  if (!app_running) return ;

  app_running = false;
  app_current->AppDeInit();
  app_easy_timer_cancel(app_timer);
  app_timer = EASY_TIMER_INVALID_TIMER;
}

void app_recv_target(uint8_t length, const void* data) {
  if (!app_running) return ;

  app_current->BLE_Recv(length, data);
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void user_custs1_connected() {
  app_send_version();
  app_start();
}

void user_custs1_disconnected() {
  app_stop();
}

#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))

void xxtea(uint32_t *v, int n, const uint32_t* key) {
  uint32_t y, z, sum;
  unsigned p, rounds, e;

  rounds = 6 + 52/n;
  sum = rounds*DELTA;
  y = v[0];
  do {
    e = (sum >> 2) & 3;
    for (p=n-1; p>0; p--) {
      z = v[p-1];
      y = v[p] -= MX;
    }
    z = v[n-1];
    y = v[0] -= MX;
    sum -= DELTA;
  } while (--rounds);
}

void user_custs1_ota_target_wr_ind_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
  int length = param->length & ~3;

  if (app_valid) {
    write_offset = 0;
    app_stop();
    app_valid = false;
    app_send_version();
  }

  if (write_offset + length > sizeof(ApplicationMap)) {
    write_offset = 0;
    return ;
  }

  memcpy(&app_write_address[write_offset], param->value, length);
  write_offset += length;

  if (param->length & 3) {
    // Erase upper memory (anti-hack)
    memset(&app_write_address[write_offset], 0, sizeof(ApplicationMap) - write_offset);
    xxtea((uint32_t*)app_location, write_offset / sizeof(uint32_t), (const uint32_t*)&xxtea_key);

    if (memcmp(xxtea_nonce, app_current->nonce, sizeof(xxtea_nonce)) != 0) {
      write_offset = 0;
      return ;
    }

    app_valid = true;
    app_send_version();
    app_start();

    return ;
  }
}

void user_custs1_app_write_wr_ind_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_write_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    app_recv_target(param->length, param->value);
}
