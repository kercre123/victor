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
#include "app/fixture.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app/bodyTest.h"
#include "app/cubeTest.h"
#include "app/headTest.h"

u8 g_fixtureReleaseVersion = 1;

BOOL g_isRobotPresent = 0;
FixtureType g_fixtureType = FIXTURE_HEAD_TEST;
FlashParams g_flashParams;

char g_lotCode[15] = {0};
u32 g_time = 0;
u32 g_dateCode = 0;

static TestFunction* m_functions = 0;
static u8 m_functionCount = 0;

static BOOL IsContactOnFixture(void);
BOOL ToggleContacts(void);
static BOOL TryToRunTests(void);

// This sets up a log entry in the robot flash - showing a test was started
void WritePreTestData(void)
{
}
// This logs an error code in the robot flash - showing a test completed (maybe successfully)
void WriteFactoryBlockErrorCode(error_t errorCode)
{
}

// Not even sure why..
TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    NULL
  };
  return m_debugFunctions;
}

// This generates a unique ID per cycle of the test fixture
// This was meant to help the "big data" team see if the fixture was ever run but the log was lost (gaps in sequences)
void GetSequence(void)
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
        return;
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
}

// Show the name of the fixture and version information
void SetFixtureText(void)
{
  DisplayClear();
  
  if (g_fixtureType == FIXTURE_CHARGER_TEST) {    
    DisplayBigCenteredText("CHARGE");
  } else if (g_fixtureType == FIXTURE_CUBE_TEST) {    
    DisplayBigCenteredText("CUBE");
  } else if (g_fixtureType == FIXTURE_HEAD_TEST) {    
    DisplayBigCenteredText("HEAD");  
  } else if (g_fixtureType == FIXTURE_BODY_TEST) {    
    DisplayBigCenteredText("BODY");
  } else if (g_fixtureType == FIXTURE_NONE) {    
    DisplayBigCenteredText("NO ID");
  } else if (g_fixtureType == FIXTURE_DEBUG) {    
    DisplayBigCenteredText("DEBUG");
  }
  
  // Show the version number in the corner
  DisplayTextHeightMultiplier(1);
  DisplayTextWidthMultiplier(1);
  DisplayMoveCursor(7, 0);
  DisplayPutChar('v');
  DisplayPutChar('0' + ((g_fixtureReleaseVersion / 10) % 10));
  DisplayPutChar('0' + (g_fixtureReleaseVersion % 10));
  
  DisplayFlip();
}

// Clear the display and print (index / count)
void SetTestCounterText(u32 current, u32 count)
{
  DisplayClear();
  DisplayBigCenteredText("%02d/%02d", current, count);
  DisplayFlip();
  
  SlowPrintf("Test %i/%i\r\n", current, count);
}

void SetErrorText(u16 error)
{
  STM_EVAL_LEDOn(LEDRED);  // Red
  
  DisplayClear();
  DisplayInvert(1);  
  DisplayBigCenteredText("%3i", error % 1000);
  DisplayFlip();
  
  // We want to force the red light to be seen for at least a second
  MicroWait(1000000);
}

void SetOKText(void)
{
  STM_EVAL_LEDOn(LEDGREEN);  // Green
  
  DisplayClear();
  DisplayBigCenteredText("OK");
  DisplayFlip();
}

// Wait until the Robot has been pulled off the fixture
void WaitForRobotOff(void)
{
  if (g_fixtureType & FIXTURE_DEBUG)
  {
    while (g_isRobotPresent)
    {
      try
      {
        Put8(DMC_ACK);
        SendCommand(NULL, 0);
      }
      catch (error_t error)
      {
        break;
      }
      
      ConsoleUpdate();
    }
    // ENBAT off
    DisableBAT();
    g_isRobotPresent = 0;
  } else {
    // ENBAT off
    DisableBAT();
    
    u32 debounce = 0;
    while (g_isRobotPresent)
    {
      TestEnableRx();
      TestDisable();
      PIN_IN(GPIOC, 10);
      PIN_IN(GPIOC, 12);
      PIN_PULL_UP(GPIOC, 10);
      MicroWait(10000);
    
      if ((GPIO_READ(GPIOC) & GPIO_Pin_10))
      {
        if (++debounce >= 10)
          g_isRobotPresent = 0;
      }
    }
  }
}

// Try to get a robot sitting on the charge contacts to enter debug mode
// XXX: This code will need to be reworked for Cozmo
void TryToEnterDiagnosticMode(void)
{
  u32 i;
  int value;
  u8* buffer = GetGlobalBuffer();

  TestEnableTx();
  TestPutChar(0xFF);
  for (i = 0; i < 4; i++)
    TestPutChar(DMC_ENTER);
  
  TestEnableRx();
  value = TestGetCharWait(100);
  if (value == DMC_ACK)
  {
    SlowPrintf("In diag mode..\r\n");
    return;
  }
  
  throw ERROR_ACK1;
}

// Walk through tests one by one - logging to the PC and to the robot flash
static void RunTests()
{
  int i;
  
  ConsoleWrite("[TEST:START]\r\n");
  
  ConsolePrintf("fixtureSerial,%i\r\n", FIXTURE_SERIAL);
  ConsolePrintf("fixtureVersion,%i\r\n", FIXTURE_VERSION);
  
  error_t error = ERROR_OK;
  try
  {
    // Write pre-test data to flash and update factory block
    WritePreTestData();
    
    for (i = 0; i < m_functionCount; i++)
    {      
      SetTestCounterText(i + 1, m_functionCount);
      m_functions[i]();
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

  if (error != ERROR_OK)
  {
    SetErrorText(error);
  } else {
    SetOKText();
  }
  
  WaitForRobotOff();
}

// This checks for a robot (even asleep) that is in contact with the fixture
static BOOL IsContactOnFixture(void)
{
  TestDisable();
  PIN_IN(GPIOC, 10);
  PIN_IN(GPIOC, 12);
  PIN_PULL_UP(GPIOC, 10);
  MicroWait(100);
  
  g_isRobotPresent = 0;
  
  static u32 s_debounce = 0;
  if (!(GPIO_READ(GPIOC) & GPIO_Pin_10))
  {
    if (++s_debounce >= 12500)
    {
      s_debounce = 0;
      return TRUE;
    }
  } else {
    s_debounce  = 0;
  }
  
  return FALSE;
}

// This function is meant to wake up a robot that is placed on a charger once it is detected
BOOL ToggleContacts(void)
{
  TestEnable();
  TestEnableTx();
  MicroWait(100000);  // 100ms
  TestEnableRx();
  MicroWait(200000);  // 200ms
  return TRUE;
  
  /*
  u32 i;
  BOOL sawPowerOn = FALSE;
  
  PIN_OD(GPIOC, 10);
  
  const u32 maxCycles = 5000;
  for (i = 0; i < maxCycles; i++)
  {
    GPIO_SET(GPIOC, 10);
    PIN_OUT(GPIOC, 10);
    GPIO_SET(GPIOC, 12);
    MicroWait(10);
    GPIO_RESET(GPIOC, 12);
    GPIO_RESET(GPIOC, 10);
    MicroWait(5);
    PIN_IN(GPIOC, 10);
    MicroWait(5);
    if (GPIO_READ(GPIOC) & (1 << 10))
    {
      sawPowerOn = TRUE;
      break;
    }
  }
  
  PIN_PULL_NONE(GPIOC, 10);
  PIN_OUT(GPIOC, 10);
  PIN_PP(GPIOC, 10);
  GPIO_RESET(GPIOC, 10);
  GPIO_SET(GPIOC, 12);
  PIN_OUT(GPIOC, 12);
  
  return sawPowerOn;*/
}

static BOOL TryToRunTests(void)
{
  u32 i;
  
  // Revert PC10 and PC12 to outputs
  PIN_PULL_NONE(GPIOC, 10);
  PIN_OUT(GPIOC, 10);
  PIN_OUT(GPIOC, 12);
  
  // PCB fixtures are a special case (no diagnostic mode)
  if (g_fixtureType == FIXTURE_BODY_TEST)
  {
    TestEnable();
    TestEnableRx();
    TestEnableTx();
    g_isRobotPresent = 1;
    
    RunTests();
    return TRUE;
  } else {
    ToggleContacts();
    
    TestEnable();
    TestEnableRx();
    g_isRobotPresent = 1;
    BOOL isInDiagnosticMode = FALSE;
    
    // One day, the factory sent us a message saying they were receiving
    // lots of 002 power-on errors. This was quite possibly caused by the
    // bootloader timing matching up perfectly when it sends an ACK command
    // for flashing through there.  This delay should be sufficient enough
    // to not hit this problem as often. This should let the bootloader
    // finish booting into the main program.
    MicroWait(200000);
    
    // The count here was also increased for the factory, just in case.
    for (i = 0; i < 5000; i++)
    {
      try
      {
        TryToEnterDiagnosticMode();
        isInDiagnosticMode = TRUE;
        break;
      }
      catch (error_t e)
      {
        // ...
      }
    }
    
    if (isInDiagnosticMode)
    {
      RunTests();
      return TRUE;
    }
  }
  
  return FALSE;
}

static void MainExecution()
{
  int i;
    
  switch (g_fixtureType)
  {
    case FIXTURE_CHARGER_TEST:
    case FIXTURE_CUBE_TEST:
      m_functions = GetCubeTestFunctions();
      break;
    case FIXTURE_BODY_TEST:
      m_functions = GetBodyTestFunctions();
      break;
    case FIXTURE_HEAD_TEST:
      m_functions = GetHeadTestFunctions();
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
  
  if (IsContactOnFixture())
  {
    TestEnableRx();
    SetTestCounterText(0, m_functionCount);
    
    STM_EVAL_LEDOff(LEDRED);
    STM_EVAL_LEDOff(LEDGREEN);
    
    const int maxTries = 5;
    for (i = 0; i < maxTries; i++)
    {
      if (TryToRunTests())
        break;
    }
    
    if (i == maxTries)
    {
      error_t error = ERROR_OK;
      if (error != ERROR_OK)
      {
        SetErrorText(error);
        WaitForRobotOff();
      }
    }
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

#if 0
extern u8 g_fixbootbin[], g_fixbootend[];

// Check if bootloader is outdated and update it with the latest
// This is stupidly risky and could easily brick a board - but it replaces an old bootloader that bricks boards
// Fight bricking with bricking!
void UpdateBootLoader(void)
{
  // Save the serial number, in case we have to restore it
  u32 serial = FIXTURE_SERIAL;
  
  // Spend a little while checking - no point in giving up on our first try, since the board will die if we do
  for (int i = 0; i < 100; i++)
  {
    // Byte by byte comparison
    bool matches = true;
    for (int j = 0; j < (g_fixbootend - g_fixbootbin); j++)
      if (g_fixbootbin[j] != ((u8*)FLASH_BOOTLOADER)[j])
        matches = false;
      
    // Bail out if all is good
    if (matches)
      return;
    
    SlowPutString("Mismatch!\r\n");
    
    // If not so good, check a few more times, leaving time for voltage to stabilize
    if (i > 50)
    {
      SlowPutString("Flashing...");
      
      // Gulp.. it's bricking time
      FLASH_Unlock();
      
      // Erase and reflash the boot code
      FLASH_EraseSector(FLASH_BLOCK_BOOT, VoltageRange_1);
      for (int j = 0; j < (g_fixbootend - g_fixbootbin); j++)
        FLASH_ProgramByte(FLASH_BOOTLOADER + j, g_fixbootbin[j]);
      
      // Recover the serial number
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL, serial & 255);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+1, serial >> 8);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+2, serial >> 16);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+3, serial >> 24);
      FLASH_Lock();
      
      SlowPutString("Done!\r\n");
    }
  }
  
  // If we get here, we are DEAD!
}
#endif

int main(void)
{
  __IO uint32_t i = 0;
 
  InitTimers();
  InitUART();
  FetchParams();
  InitConsole();
  
  SlowPutString("STARTUP!\r\n");
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  /* Initialize LEDs */
  STM_EVAL_LEDInit(LEDRED);
  STM_EVAL_LEDInit(LEDGREEN);

  STM_EVAL_LEDOff(LEDRED);
  STM_EVAL_LEDOff(LEDGREEN);
  
  // This belongs in future board.c
  // Always enable charger/ENCHG (PA15), despite switch being off
  // On EP1 fixtures, R5 must be removed for this to work!
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_SetBits(GPIOA, GPIO_Pin_15);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // Initialize PB13-PB15 as the ID inputs with pullups
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // Wait for lines to initialize and not float
  // 2 us was not long enough, just being safe...
  MicroWait(100);

  // Read the pins with pull-down resistors on GPIOB[14:12]
  g_fixtureType = (FixtureType)((GPIO_READ(GPIOB) >> 12) & 7);
  
#ifdef CHARGE_BUILD
  g_fixtureType = FIXTURE_CHARGE_TEST;
#endif
  
  SlowPrintf("fixture: %i\r\n", g_fixtureType);
  
  InitBAT();
  
  SlowPutString("Initializing Display...\r\n");
  
  // XXX: EP1 test fixtures unfortunately share display lines with DUT lines, so cube must be in reset for OLED to work!
  InitCube();  
  InitDisplay();

  SetFixtureText();
  
  SlowPutString("Initializing Test Port...\r\n");
  InitTestPort(0);

  SlowPutString("Initializing Monitor...\r\n");
  InitMonitor();
  
  SlowPutString("Ready...\r\n");

  STM_EVAL_LEDOn(LEDRED);

  ProgramCube();

  while (1)
  {  
    MainExecution();
    DisplayUpdate();
  }
}
