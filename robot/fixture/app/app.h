#ifndef APP_H
#define APP_H

#include "hal/flash.h"
#include "hal/portable.h"
#include "binaries.h"
#include "app/fixture.h"

//data preserved through a soft-reset
typedef struct {
  u8 valid;
  struct{
    u8 isInConsoleMode;
  } console;
} app_reset_dat_t;

//globals located in app.c
extern u8               g_fixtureReleaseVersion;
extern app_reset_dat_t  g_app_reset;
extern BOOL             g_isDevicePresent;
extern const char*      FIXTYPES[];
extern FixtureType      g_fixtureType;

extern char             g_lotCode[15];
extern u32              g_time;
extern u32              g_dateCode;

//debugTest.c
extern TestFunction* GetDebugTestFunctions(void);
extern bool DebugTestDetectDevice(void);

#endif
