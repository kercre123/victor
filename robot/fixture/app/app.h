#ifndef APP_H
#define APP_H

#include "hal/portable.h"

//data preserved through a soft-reset
typedef struct {
  u8 valid;
  struct{
    u8 isInConsoleMode;
  } console;
} app_reset_dat_t;

//globals located in app.c
extern app_reset_dat_t g_app_reset;

#endif
