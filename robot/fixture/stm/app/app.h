#ifndef APP_H
#define APP_H
#include <stdint.h>
#include <string.h>
#include "binaries.h"
#include "board.h"
#include "fixture.h"
#include "flash.h"
#include "portable.h"

//data preserved through a soft-reset
typedef struct {
  u8 valid;
  struct{
    u8 isInConsoleMode;
  } console;
} app_reset_dat_t;

//globals located in app.c
extern const bool       g_isReleaseBuild;
extern u8               g_fixtureReleaseVersion;
extern app_reset_dat_t  g_app_reset;
extern bool             g_isDevicePresent;
extern bool             g_forceStart;
extern char             g_lotCode[15];
extern u32              g_time;
extern u32              g_dateCode;

#define APP_GLOBAL_BUF_SIZE   (1024 * 16)
extern uint8_t app_global_buffer[APP_GLOBAL_BUF_SIZE];

char* snformat(char *s, size_t n, const char *format, ...);

#endif //APP_H
