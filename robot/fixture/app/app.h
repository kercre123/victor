#ifndef APP_H
#define APP_H

#include "hal/board.h"
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

//Fixture Hardware revisions
typedef enum {
  FIX_HW_REV_1_0_REV1  = 0,
  FIX_HW_REV_1_0_REV2  = 1,
  FIX_HW_REV_1_0_REV3  = 2,
  FIX_HW_REV_1_5_0     = 3,
} fixture_hw_rev_t;

//globals located in app.c
extern u8               g_fixtureReleaseVersion;
extern app_reset_dat_t  g_app_reset;
extern BOOL             g_isDevicePresent;
extern const char*      FIXTYPES[];
extern FixtureType      g_fixtureType;
extern board_rev_t      g_fixtureRev;
extern char             g_lotCode[15];
extern u32              g_time;
extern u32              g_dateCode;

//debugTest.c
extern TestFunction* GetDebugTestFunctions(void);
extern bool DebugTestDetectDevice(void);

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif

#endif

