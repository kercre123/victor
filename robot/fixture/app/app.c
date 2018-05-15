#include "hal/board.h"
#include "hal/display.h"
#include "hal/flash.h"
#include "hal/monitor.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "hal/testport.h"
#include "hal/uart.h"
#include "hal/console.h"
#include "hal/cube.h"
#include "hal/espressif.h"
#include "hal/random.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app/app.h"
#include "app/fixture.h"
#include "app/tests.h"
#include "nvReset.h"

u8 g_fixtureReleaseVersion = 104;
u8 g_fixtureReleaseRev = 8; //'ver.rev' e.g. 104.2
#define BUILD_INFO "MP v1.5"

//Set this flag to modify display info - indicates a debug/test build
#define NOT_FOR_FACTORY 0

#ifdef FCC
  const bool fcc = true;
#else
  const bool fcc = false;
#endif

#ifdef JRL
  const bool jrl = true;
#else
  const bool jrl = false;
#endif

//other global dat
app_reset_dat_t g_app_reset;

BOOL g_isDevicePresent = 0;
const char* FIXTYPES[NUM_FIXTYPES] = FIXTURE_TYPES;
FixtureType g_fixtureType = FIXTURE_NONE;
board_rev_t g_fixtureRev = (board_rev_t)0;

char g_lotCode[15] = {0};
u32 g_time = 0;
u32 g_dateCode = 0;

static TestFunction* m_functions = 0;
static u8 m_functionCount = 0;

BOOL ToggleContacts(void);
static BOOL TryToRunTests(void);

// This sets up a log entry in the Device flash - showing a test was started
void WritePreTestData(void)
{
}
// This logs an error code in the Device flash - showing a test completed (maybe successfully)
void WriteFactoryBlockErrorCode(error_t errorCode)
{
}

// This generates a unique ID per cycle of the test fixture
// This was meant to help the "big data" team see if the fixture was ever run but the log was lost (gaps in sequences)
int GetSequence(void)
{
  u32 sequence;
  u8 bit;
  
  {
    sequence = 0;
    u8* serialbase = (u8*)FLASH_SERIAL_BITS;
    while (serialbase[(sequence >> 3)] == 0)
    {
      sequence += 8;
      if (sequence > 0x7ffff)
      {
        ConsolePrintf("fixtureSequence,-1\r\n");
        throw ERROR_OUT_OF_SERIALS;
      }
    }
    
    u8 bitMask = serialbase[(sequence >> 3)];
    
    // Find which bit we're on
    bit = 0;
    while (!(bitMask & (1 << bit)))
    {
      bit++;
    }
    sequence += bit;
  }
  
  // Reserve this test sequence
  FLASH_Unlock();
  FLASH_ProgramByte(FLASH_SERIAL_BITS + (sequence >> 3), ~(1 << bit));
  FLASH_Lock();
  
  ConsolePrintf("fixtureSequence,%i,%i\r\n", FIXTURE_SERIAL, sequence);
  SlowPrintf("Allocated serial: %x\n", sequence);
  
  return sequence;
}

// Get a serial number for a device in the normal 12.20 fixture.sequence format
u32 GetSerial()
{
  if (FIXTURE_SERIAL > 0xfff) //12-bit limit
    throw ERROR_SERIAL_INVALID;
  return (FIXTURE_SERIAL << 20) | (GetSequence() & 0x0Fffff);
}

extern int g_canary;
// Show the name of the fixture and version information
void SetFixtureText(void)
{
  DisplayClear();
  DisplayBigCenteredText(FIXTYPES[g_fixtureType]);
  
  //reset display sizing/defaults
  DisplayTextHeightMultiplier(1);
  DisplayTextWidthMultiplier(1);
  DisplayInvert(fcc); //invert the display colors for fcc build (easy to idenfity)
  
  //Dev builds show compile date-time across the top
#if NOT_FOR_FACTORY
  DisplayMoveCursor(1,2);
  DisplayPutString("DEV-NOT FOR FACTORY!!");
  DisplayMoveCursor(10,2);
  DisplayPutString(__DATE__);
  DisplayPutString(" ");
  DisplayPutString(__TIME__);
#endif
  
  //add verision #s and other useful info
#ifdef FCC
  DisplayMoveCursor(45, 2);
  DisplayPutString("CERT/TEST ONLY");
#else
  if( g_fixtureType == FIXTURE_PLAYPEN_TEST ) {
    DisplayMoveCursor(45, 25);
    DisplayPutString("SSID: Afix");
    DisplayPutChar('0' + ((FIXTURE_SERIAL&63) / 10)); //param sent to cozmo in playpen / robotTest.c
    DisplayPutChar('0' + ((FIXTURE_SERIAL&63) % 10)); //"
  }
#endif
  DisplayMoveCursor(55, 2);
  DisplayPutString(BUILD_INFO);
  
#if NOT_FOR_FACTORY > 0
  DisplayMoveCursor(55, fcc ? 108+7 : 105+7 );
  DisplayPutChar('N');
  DisplayPutChar('A');
  volatile bool junk = jrl; //i hate compiler warnings
#else
  u8 rev_space = (g_fixtureReleaseRev > 0 ? 14 : 0); //leave space for ".#" revision
  DisplayMoveCursor(55, fcc ? 108-rev_space : 105-rev_space );
  DisplayPutChar(fcc ? 'c' : (jrl ? 'j' : 'v') );
  DisplayPutChar('0' + ((g_fixtureReleaseVersion / 100)));
  DisplayPutChar('0' + ((g_fixtureReleaseVersion / 10) % 10));
  DisplayPutChar('0' + (g_fixtureReleaseVersion % 10));
  if( g_fixtureReleaseRev > 0) {
    DisplayPutChar('.');
    DisplayPutChar('0' + (g_fixtureReleaseRev % 10));
  }
#endif
  
  DisplayFlip();
}

// Clear the display and print (index / count)
void SetTestCounterText(u32 current, u32 count)
{
  DisplayClear();
  DisplayBigCenteredText("%02d/%02d", current, count);
  DisplayFlip();
  
//  SlowPrintf("Test %i/%i\r\n", current, count);
}

void SetErrorText(u16 error)
{
  STM_EVAL_LEDOn(LEDRED);  // Red
  
  DisplayClear();
  DisplayInvert(1);  
  DisplayBigCenteredText("%3i", error % 1000);
  DisplayFlip();
  
  MicroWait(200000);      // So nobody misses the error
}

void SetOKText(void)
{
  STM_EVAL_LEDOn(LEDGREEN);  // Green
  
  DisplayClear();
  DisplayBigCenteredText("OK");
  DisplayFlip();
}

// Return true if a device is detected (on the contacts)
bool DetectDevice(void)
{
  switch (g_fixtureType)
  {
    case FIXTURE_CHARGER_TEST:
    case FIXTURE_CUBE1_TEST:
    case FIXTURE_CUBE2_TEST:
    case FIXTURE_CUBE3_TEST:
    case FIXTURE_CUBEX_TEST:
      return CubeDetect();
    case FIXTURE_HEAD1_TEST:
    case FIXTURE_HEAD2_TEST:
      return HeadDetect();
    case FIXTURE_BODY1_TEST:
    case FIXTURE_BODY2_TEST:
    case FIXTURE_BODY3_TEST:
      return BodyDetect();
    case FIXTURE_INFO_TEST:
    case FIXTURE_ROBOT1_TEST:
    case FIXTURE_ROBOT2_TEST:
    case FIXTURE_ROBOT3_TEST:
    case FIXTURE_ROBOT3_CE_TEST:
    case FIXTURE_COZ187_TEST:
    case FIXTURE_PACKOUT_TEST:
    case FIXTURE_PACKOUT_CE_TEST:
    case FIXTURE_LIFETEST_TEST:
    case FIXTURE_RECHARGE_TEST:
    case FIXTURE_RECHARGE2_TEST:
    case FIXTURE_PLAYPEN_TEST:
    case FIXTURE_SOUND_TEST:
    case FIXTURE_EMROBOT_TEST:
      return RobotDetect();
    case FIXTURE_MOTOR1L_TEST:
    case FIXTURE_MOTOR1H_TEST:
    case FIXTURE_MOTOR2L_TEST:
    case FIXTURE_MOTOR2H_TEST:
    case FIXTURE_MOTOR3L_TEST:
    case FIXTURE_MOTOR3H_TEST:
      return MotorDetect();
    case FIXTURE_FINISHC_TEST:
    case FIXTURE_FINISH1_TEST:
    case FIXTURE_FINISH2_TEST:
    case FIXTURE_FINISH3_TEST:
    case FIXTURE_FINISHX_TEST:
    case FIXTURE_EMCUBE_TEST:
      return FinishDetect();
    case FIXTURE_DEBUG:
      return DebugTestDetectDevice();
  }

  // If we don't know what kind of device to look for, it's not there!
  return false;
}

// Wait until the Device has been pulled off the fixture
void WaitForDeviceOff(bool error)
{
  // In debug mode, keep device powered up so we can continue talking to it
  if (g_fixtureType == FIXTURE_DEBUG)
  {
    //while (g_isDevicePresent)
    {
      // Note: We used to send DMC_ACK commands continuously here to prevent auto-power-off
      ConsoleUpdate();
      DisplayUpdate();
    }
    // ENBAT off
    DisableVEXT();
    DisableBAT();

  // In normal mode, just debounce the connection
  } else {
    if( g_fixtureType == FIXTURE_HEAD1_TEST || g_fixtureType == FIXTURE_HEAD2_TEST ) //head tests leave this on!
      DisableVEXT();
    
    // ENBAT off
    DisableBAT();
    
    u32 debounce = 0;
    u8 buz = 0;
    while (g_isDevicePresent)
    {
      // Blink annoying red LED
      static u8 annoy;
      if (error && (annoy++ & 0x80))
        STM_EVAL_LEDOn(LEDRED);
      else
        STM_EVAL_LEDOff(LEDRED);
      
      // Beep an even more annoying buzzer
      BuzzerOnStatic( error && (buz++ & 0x80) );
      
      if (!DetectDevice())
      {
        // 500 checks * 1ms = 500ms delay showing error post removal
        if (++debounce >= 500)
          g_isDevicePresent = 0;
      }
      
      ConsoleUpdate();  // No need to freeze up the console while waiting
      DisplayUpdate();  // While we wait, let screen saver kick in
    }
    BuzzerOnStatic(0);
  }
  
  // When device is removed, restore fixture text
  SetFixtureText();
}

// Walk through tests one by one - logging to the PC and to the Device flash
int g_stepNumber;
static void RunTests()
{
  ConsoleWrite("[TEST:START]\r\n");
  
  ConsolePrintf("fixtureSerial,%i\r\n", FIXTURE_SERIAL);
  ConsolePrintf("fixtureVersion,%i\r\n", FIXTURE_VERSION);
  ConsolePrintf("fixtureRev,%s,v%d.%d,%s\r\n", GetBoardRevStr(), g_fixtureReleaseVersion, g_fixtureReleaseRev, NOT_FOR_FACTORY > 0 ? "debug" : "release");
  
  error_t error = ERROR_OK;
  try
  {
    // Write pre-test data to flash and update factory block
    WritePreTestData();
    
    for (g_stepNumber = 0; g_stepNumber < m_functionCount; g_stepNumber++)
    {      
      SetTestCounterText(g_stepNumber + 1, m_functionCount);
      m_functions[g_stepNumber]();
    }
    
    WriteFactoryBlockErrorCode(ERROR_OK);
  }
  catch (error_t e)
  {
    error = e;
  }
  
  try
  {
    // Attempt to log to the factory block...
    if (error != ERROR_OK && !IS_INTERNAL_ERROR(error))
      WriteFactoryBlockErrorCode(error);
  }
  catch (error_t e)
  {
    // ...
  }

  ConsolePrintf("[RESULT:%03i]\r\n[TEST:END]\r\n", error);
//  SlowPrintf("Test finished with error %03d\n", error);
  
  if (error != ERROR_OK)
  {
    SetErrorText(error);
  } else {
    SetOKText();
  }
  
  WaitForDeviceOff(error != ERROR_OK);
}

// This checks for a Device (even asleep) that is in contact with the fixture
static BOOL IsDevicePresent(void)
{
  g_isDevicePresent = 0;
  
  static u32 s_debounce = 0;
  
  if (DetectDevice())
  {
    // 300 checks * 1ms = 300ms to be sure the board is reliably in contact
    if (++s_debounce >= 300)
    {
      s_debounce = 0;
      return TRUE;
    }
  } else {
    s_debounce  = 0;
  }
  
  return FALSE;
}

// Wake up the board and try to talk to it
static BOOL TryToRunTests(void)
{
  // PCB fixtures are a special case (no diagnostic mode)
  // If/when we add testport support - use ToggleContacts and then repeatedly call TryToEnterDiagnosticMode
  g_isDevicePresent = 1;
  RunTests();
  return TRUE;
}

// Repeatedly scan for a device, then run through the tests when it appears
static void MainExecution()
{
  switch (g_fixtureType)
  {
    case FIXTURE_CHARGER_TEST:
    case FIXTURE_CUBE1_TEST:
    case FIXTURE_CUBE2_TEST:
    case FIXTURE_CUBE3_TEST:
    case FIXTURE_CUBEX_TEST:
      m_functions = GetCubeTestFunctions();
      break;
    case FIXTURE_HEAD1_TEST:
      m_functions = GetHeadTestFunctions();
      break;    
    case FIXTURE_HEAD2_TEST:
      m_functions = GetHead2TestFunctions();
      break;    
    case FIXTURE_BODY1_TEST:
      m_functions = GetBody1TestFunctions();
      break;
    case FIXTURE_BODY2_TEST:
      m_functions = GetBody2TestFunctions();
      break;
    case FIXTURE_BODY3_TEST:
      m_functions = GetBody3TestFunctions();
      break;
    case FIXTURE_INFO_TEST:
      m_functions = GetInfoTestFunctions();
      break;
    case FIXTURE_ROBOT1_TEST:
    case FIXTURE_ROBOT2_TEST:
    case FIXTURE_ROBOT3_TEST:
    case FIXTURE_ROBOT3_CE_TEST:
      m_functions = GetRobotTestFunctions();
      break;
    case FIXTURE_EMROBOT_TEST:
      m_functions = GetEMRobotTestFunctions();
      break;
    case FIXTURE_COZ187_TEST:
      m_functions = GetFacRevertTestFunctions();
      break;
    case FIXTURE_PACKOUT_TEST:
    case FIXTURE_PACKOUT_CE_TEST:
      m_functions = GetPackoutTestFunctions();
      break; 
    case FIXTURE_LIFETEST_TEST:
      m_functions = GetLifetestTestFunctions();
      break; 
    case FIXTURE_RECHARGE_TEST:
    case FIXTURE_RECHARGE2_TEST:
      m_functions = GetRechargeTestFunctions();
      break; 
    case FIXTURE_PLAYPEN_TEST:
      m_functions = GetPlaypenTestFunctions();
      break;
    case FIXTURE_SOUND_TEST:
      m_functions = GetSoundTestFunctions();
      break;      
    case FIXTURE_MOTOR1L_TEST:
      m_functions = GetMotor1LTestFunctions();
      break;
    case FIXTURE_MOTOR1H_TEST:
      m_functions = GetMotor1HTestFunctions();
      break;
    case FIXTURE_MOTOR2L_TEST:      
      m_functions = GetMotor2LTestFunctions();
      break;
    case FIXTURE_MOTOR2H_TEST:      
      m_functions = GetMotor2HTestFunctions();
      break;
    case FIXTURE_MOTOR3L_TEST:
      m_functions = GetMotor3LTestFunctions();
      break;
    case FIXTURE_MOTOR3H_TEST:
      m_functions = GetMotor3HTestFunctions();
      break;
    case FIXTURE_FINISHC_TEST:
    case FIXTURE_FINISH1_TEST:
    case FIXTURE_FINISH2_TEST:
    case FIXTURE_FINISH3_TEST:
    case FIXTURE_FINISHX_TEST:
    case FIXTURE_EMCUBE_TEST:
      m_functions = GetFinishTestFunctions();
      break;
    case FIXTURE_DEBUG:
      m_functions = GetDebugTestFunctions();
      break;
  }
  
  // Count the number of functions to test
  TestFunction* fn = m_functions;
  m_functionCount = 0;
  while (*fn++)
    m_functionCount++;

  STM_EVAL_LEDOff(LEDRED);
  STM_EVAL_LEDOff(LEDGREEN);
  
  ConsoleUpdate();
  
  u32 startTime = getMicroCounter();
  
  if (IsDevicePresent())
  {
    SetTestCounterText(0, m_functionCount);
    
    STM_EVAL_LEDOff(LEDRED);
    STM_EVAL_LEDOff(LEDGREEN);
    
    TryToRunTests();
  }
}

// Fetch flash parameters - done once on boot up
void FetchParams(void)
{
  memcpy(&g_flashParams, (u8*)FLASH_PARAMS, sizeof(FlashParams));
}

// Store flash parameters
void StoreParams(void)
{
  FLASH_Unlock();
  FLASH_EraseSector(FLASH_BLOCK_PARAMS, VoltageRange_1);
  for (int i = 0; i < sizeof(FlashParams); i++)
    FLASH_ProgramByte(FLASH_PARAMS + i, ((u8*)(&g_flashParams))[i]);
  FLASH_Lock();
}
int main(void)
{
  __IO uint32_t i = 0;
  
  //Check for nvReset data
  memset( &g_app_reset, 0, sizeof(g_app_reset) );
  g_app_reset.valid = sizeof(g_app_reset) == nvResetGet( (u8*)&g_app_reset, sizeof(g_app_reset) );
  
  InitTimers();
  InitUART();
  FetchParams(); //g_flashParams = flash backup (saved via 'setmode' console cmd)
  InitConsole();
  InitRandom();
 
  SlowPutString("STARTUP!\r\n");
  InitBoard();
  g_fixtureRev = GetBoardRev();
  
  //Check the fixture's mode (need a freakin' flow chart for this)
  //1) if we don't have a full upload, always set to version 0, mode NONE
  //2)   if fixture is 1.0, read ID set by external resistors
  //3)   if no valid mode detected so far, and one is saved in flash, use that.
  g_fixtureType = FIXTURE_NONE;
  if (g_canary != 0xcab00d1e)
    g_fixtureReleaseVersion = 0;
  else
  {
    if( g_fixtureRev < BOARD_REV_1_5_1 )
      g_fixtureType = GetBoardID();
    
    if (g_fixtureType == FIXTURE_NONE 
          && g_flashParams.fixtureTypeOverride > FIXTURE_NONE 
          && g_flashParams.fixtureTypeOverride < FIXTURE_DEBUG)
      g_fixtureType = g_flashParams.fixtureTypeOverride;
  }
  
  SlowPutString("Initializing Display...\r\n");
  
  InitCube();
  InitDisplay();

  SetFixtureText();
  
  SlowPutString("Initializing Test Port...\r\n");
  InitTestPort(0);

  SlowPutString("Initializing Monitor...\r\n");
  InitMonitor();
  
  SlowPutString("Ready...\r\n");

  InitEspressif();

  STM_EVAL_LEDOn(LEDRED);
  
  ConsolePrintf("\r\n----- Cozmo Test Fixture: %s%s v%d.%d -----\r\n", BUILD_INFO, (NOT_FOR_FACTORY > 0 ? " NOT-FOR-FACTORY" : ""), g_fixtureReleaseVersion, g_fixtureReleaseRev );
  ConsolePrintf("Build date-time: %s %s\r\n", __DATE__, __TIME__);
  ConsolePrintf("FIXTURE_SERIAL: %d (0x%04x)\r\n", FIXTURE_SERIAL, FIXTURE_SERIAL);
  ConsolePrintf("ConsoleMode=%u\r\n", g_app_reset.valid && g_app_reset.console.isInConsoleMode );
  ConsolePrintf("Fixure Rev: %s\r\n", GetBoardRevStr() );
  ConsolePrintf("Mode: %s\r\n", FIXTYPES[g_fixtureType]);
  
  //lockout on bad hw (or old FW on newer hw rev)
  if( g_fixtureRev == BOARD_REV_UNKNOWN ) {
    SetErrorText( ERROR_INCOMPATIBLE_FIX_REV );
    u32 start = getMicroCounter();
    while(1) {
      if( getMicroCounter() - start > 250*1000 ) {
        start = getMicroCounter();
        STM_EVAL_LEDToggle(LEDRED);
      }
      ConsoleUpdate();  // Keep the comm chanel open so we can bootload out of this corner
      DisplayUpdate();  // While we wait, let screen saver kick in
    }
  }
  
  while (1)
  {  
    MainExecution();
    DisplayUpdate();
  }
}
