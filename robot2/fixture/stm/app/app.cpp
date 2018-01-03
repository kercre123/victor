#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "cube.h"
#include "display.h"
#include "fixture.h"
#include "flash.h"
#include "meter.h"
#include "nvReset.h"
#include "portable.h"
#include "random.h"
#include "tests.h"
#include "timer.h"
#include "uart.h"

u8 g_fixtureReleaseVersion = 2;
#define BUILD_INFO "Victor"

//Set this flag to modify display info - indicates a debug/test build
#define NOT_FOR_FACTORY 1

#define APP_CMD_OPTS    ((CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN) | CMD_OPTS_DBG_PRINT_RSP_TIME)
#define LCD_CMD_TIMEOUT 500 /*ms*/

#define USE_START_BTN 0

//other global dat
app_reset_dat_t g_app_reset;
bool g_isDevicePresent = 0;
bool g_forceStart = 0;

char g_lotCode[15] = {0};
u32 g_time = 0;
u32 g_dateCode = 0;

static TestFunction* m_functions = 0;
static int m_functionCount = 0;

bool ToggleContacts(void);
static bool TryToRunTests(void);

__align(4) uint8_t app_global_buffer[APP_GLOBAL_BUF_SIZE];

char* snformat(char *s, size_t n, const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  int len = vsnprintf(s, n, format, argptr);
  va_end(argptr);
  return s;
}

//XXX: define some helper lcd specs (these are placeholders)
//const int HELPER_LCD_NUM_LINES = 10;
//const int HELPER_LCD_CHARS_PER_LINE = 25;
//const int HELPER_LCD_CHARS_BIG_TEXT = 7;

void HelperLcdShow(bool solo, bool invert, char color_rgbw, const char* center_text)
{
  char b[50]; int bz = sizeof(b);
  if( color_rgbw != 'r' && color_rgbw != 'g' && color_rgbw != 'b' && color_rgbw != 'w' )
    color_rgbw = 'w';
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdshow %u %s%c %s", solo, invert?"i":"", color_rgbw, center_text), LCD_CMD_TIMEOUT, APP_CMD_OPTS);
}

void HelperLcdSetLine(int n, const char* line)
{
  char b[50]; int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdset %u %s", n, line), LCD_CMD_TIMEOUT, APP_CMD_OPTS);
}

void HelperLcdClear(void)
{
  HelperLcdSetLine(0,""); //clears all lines
  HelperLcdShow(0,0,'w',""); //clear center text
}

// Show the name of the fixture and version information
void SetFixtureText(void)
{
  const bool fcc = false;
  char b[50]; int bz = sizeof(b);
  static bool inited = 0;
  
  DisplayClear();
  if( !inited )
    HelperLcdClear();
  
  DisplayBigCenteredText( fixtureName() );
  
  //reset display sizing/defaults
  DisplayTextHeightMultiplier(1);
  DisplayTextWidthMultiplier(1);
  DisplayInvert(fcc); //invert the display colors for fcc build (easy to idenfity)
  
  //Dev builds show compile date-time across the top
#if NOT_FOR_FACTORY
  DisplayMoveCursor(1,2);
  DisplayPutString("DEV-NOT FOR FACTORY!!");
  if( !inited )
    HelperLcdSetLine(1, "DEV-NOT FOR FACTORY!");
  DisplayMoveCursor(10,2);
  DisplayPutString( (__DATE__ " " __TIME__) );
  if( !inited )
    HelperLcdSetLine(2, (__DATE__ " " __TIME__) );
#else
  //XXX: for now, show date even on 'release' builds (until we actually release something)
  DisplayMoveCursor(1,2);
  DisplayPutString( (__DATE__ " " __TIME__) );
  if( !inited )
    HelperLcdSetLine(2, (__DATE__ " " __TIME__) );
#endif
  
  //add verision #s and other useful info
#ifdef FCC
  DisplayMoveCursor(45, 2);
  DisplayPutString("CERT/TEST ONLY");
  if( !inited )
    HelperLcdSetLine(7, "CERT/TEST ONLY");
#else
  if( g_fixmode == FIXMODE_PLAYPEN ) {
    DisplayMoveCursor(45, 25);
    DisplayPutString("SSID: Afix");
    DisplayPutChar('0' + ((FIXTURE_SERIAL&63) / 10)); //param sent to cozmo in playpen / robotTest.c
    DisplayPutChar('0' + ((FIXTURE_SERIAL&63) % 10)); //"
    if( !inited )
      HelperLcdSetLine(7, snformat(b,bz,"   SSID: Afix %02u", FIXTURE_SERIAL&63) ); //XXX: this is probably going away
  }
#endif
  DisplayMoveCursor(55, 2);
  DisplayPutString(BUILD_INFO);
  DisplayMoveCursor(55, fcc ? 108 : 105 );
  DisplayPutChar(fcc ? 'c' : 'v');
  DisplayPutChar(NOT_FOR_FACTORY ? '-' : '0' + ((g_fixtureReleaseVersion / 100)));
  DisplayPutChar(NOT_FOR_FACTORY ? '-' : '0' + ((g_fixtureReleaseVersion / 10) % 10));
  DisplayPutChar(NOT_FOR_FACTORY ? '-' : '0' + (g_fixtureReleaseVersion % 10));
  DisplayFlip();
  
  if( !inited )
    HelperLcdSetLine(8, snformat(b,bz,"%-15s v%03u", BUILD_INFO, g_fixtureReleaseVersion) );
  
  //DisplayBigCenteredText( fixtureName() );
  HelperLcdShow(0,0,'b', (char*)fixtureName());
  
  inited = 1;
}

// Clear the display and print (index / count)
void SetTestCounterText(u32 current, u32 count)
{
  char b[10]; int bz = sizeof(b);
  DisplayClear();
  DisplayBigCenteredText("%02d/%02d", current, count);
  DisplayFlip();
  HelperLcdShow(1,0,'b', snformat(b,bz,"%02d/%02d", current, count) );
}

void SetErrorText(u16 error)
{
  char b[10]; int bz = sizeof(b);
  Board::ledOn(Board::LED_RED);  // Red
  
  DisplayClear();
  DisplayInvert(1);  
  DisplayBigCenteredText("%3i", error % 1000);
  DisplayFlip();
  HelperLcdShow(1,1,'r', snformat(b,bz,"%03i", error % 1000) );
  
  Timer::wait(200000);      // So nobody misses the error
}

void SetOKText(void)
{
  Board::ledOn(Board::LED_GREEN);  // Green
  DisplayClear();
  DisplayBigCenteredText("OK");
  DisplayFlip();
  HelperLcdShow(1,1,'g', "OK");
}

// Wait until the Device has been pulled off the fixture
void WaitForDeviceOff(bool error, int debounce_ms = 500);
void WaitForDeviceOff(bool error, int debounce_ms)
{
  Board::disableVEXT();
  Board::disableVBAT();
  
  u32 debounce = 0;
  u8 buz = 0, annoy = 0;
  while (g_isDevicePresent)
  {
    // Blink annoying red LED
    if (error && (annoy++ & 0x80))
      Board::ledOn(Board::LED_RED);
    else
      Board::ledOff(Board::LED_RED);
    
    // Beep an even more annoying buzzer
    if( error && (buz++ & 0x80) )
      Board::buzzerOn();
    else
      Board::buzzerOff();
    
    if (!fixtureDetect(1000)) {
      if (++debounce >= debounce_ms)  // 500 checks * 1ms = 500ms delay showing error post removal
        g_isDevicePresent = 0;
    } else
      debounce = 0;
    
    ConsoleUpdate();  // No need to freeze up the console while waiting
    DisplayUpdate();  // While we wait, let screen saver kick in
    
    if( g_forceStart ) { //force=1 exits this infuriating loop
      g_forceStart = 0;
      g_isDevicePresent = 0;
    }
  }
  
  Board::ledOff(Board::LED_RED);
  Board::buzzerOff();
  
  // When device is removed, restore fixture text
  SetFixtureText();
}

// Walk through tests one by one - logging to the PC and to the Device flash
int g_stepNumber;
static void RunTests()
{
  //char b[10]; int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, "logstart [args]", CMD_DEFAULT_TIMEOUT, APP_CMD_OPTS );
  
  ConsolePrintf("[TEST:START]\n");
  ConsolePrintf("fixtureSerial,%i\n", FIXTURE_SERIAL);
  ConsolePrintf("fixtureVersion,%i\n", FIXTURE_VERSION);
  ConsolePrintf("fixtureRev,%s,v%d,%s\n", Board::revString(), g_fixtureReleaseVersion, NOT_FOR_FACTORY > 0 ? "debug" : "release");
  
  error_t error = ERROR_OK;
  try {
    for (g_stepNumber = 0; g_stepNumber < m_functionCount; g_stepNumber++) {      
      ConsolePrintf("[RUN:%u/%u]\n", g_stepNumber+1, m_functionCount);
      SetTestCounterText(g_stepNumber + 1, m_functionCount);
      m_functions[g_stepNumber]();
    }
  } catch (error_t e) {
    error = e == ERROR_OK ? ERROR_BAD_ARG : e; //don't allow test to throw 'OK'
  }
  ConsolePrintf("[RESULT:%03i]\n", error);
  
  //test-specific driver and resource cleanup
  try {
    fixtureCleanup();
  } catch(error_t e) {
    ConsolePrintf("[CLEANUP-ERROR:%03i]\n", e);
  }
  
  ConsolePrintf("[TEST:END]\n", error);
  cmdSend(CMD_IO_HELPER, "logstop [args]", CMD_DEFAULT_TIMEOUT, APP_CMD_OPTS );
  
  if (error != ERROR_OK)
    SetErrorText(error); //turns on RED led
  else
    SetOKText(); //turns on GRN led
  
  WaitForDeviceOff(error != ERROR_OK);
  
  //flush console. lots of stuff can back up while we've been busy running a test
  ConsoleFlushLine();
  while( ConsoleReadChar() > -1 );
}

// This checks for a Device (even asleep) that is in contact with the fixture
static bool IsDevicePresent(void)
{
  g_isDevicePresent = 0;
  
  static u32 s_debounce = 0;
  if (fixtureDetect(1000)) {
    if (++s_debounce >= 300) {  // 300 checks * 1ms = 300ms to be sure the board is reliably in contact
      s_debounce = 0;
      return TRUE;
    }
  } else
    s_debounce  = 0;
  
  return FALSE;
}

// Wake up the board and try to talk to it
static bool TryToRunTests(void)
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
  m_functions = fixtureGetTests();
  m_functionCount = fixtureGetTestCount();
  
  Board::ledOff(Board::LED_RED);
  Board::ledOff(Board::LED_GREEN);
  
  ConsoleUpdate();
  u32 startTime = Timer::get();
  
  bool start = 0;
  bool isPresent = IsDevicePresent(); //give some juice to detect() methods
  
  #if USE_START_BTN > 0
    const int start_window_us = 1*1000*1000;
    static uint32_t Tstart = 0;
    
    //poll for start button press
    if( !Tstart ) {
      if( Board::btnEdgeDetect(Board::BTN_1, 1000, 75) > 0 ) {
        //Board::btnEdgeDetect(Board::BTN_1, -1, -1); //reset state machine
        ConsolePrintf("start btn pressed\n");
        Tstart = Timer::get();
      }
    }
    else //DUT detect to start
    {
      if( Timer::elapsedUs(Tstart) < start_window_us ) {
        start = isPresent;
      } else { //timeout
        ConsolePrintf("timeout waiting for DUT detect\n");
        Tstart = 0;
        SetErrorText(ERROR_DEVICE_NOT_DETECTED); //turns on RED led
        g_isDevicePresent = 1;
        WaitForDeviceOff(1);
      }
    }
    
    //overide detect mechanism and start the test
    if( g_forceStart ) {
      start = 1;
      Tstart = 0;
      Board::btnEdgeDetect(Board::BTN_1, -1, -1); //reset state machine
    }
  
  #else
    //legacy DUT connect starts the test
    if( isPresent || g_forceStart )
      start = 1;
  
  #endif
  
  if( start ) {
    Board::ledOff(Board::LED_RED);
    Board::ledOff(Board::LED_GREEN);
    SetTestCounterText(0, m_functionCount);
    TryToRunTests();
    g_forceStart = 0;
  }
  
  //Debug: monitor unused buttons
  int x = USE_START_BTN > 0 ? Board::BTN_2 : Board::BTN_1; //skip start btn if used for testing
  for( ; x < Board::BTN_1+Board::BTN_NUM; x++) {
    int edge = Board::btnEdgeDetect((Board::btn_e)x, 1000, 50);
    if( edge != 0 )
      ConsolePrintf("btn %u %s\n", x, edge > 0 ? "pressed" : "released");
  }//-*/
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

void MainErrorLoop(void)
{
  u32 start = Timer::get();
  
  while(1) {
    if( Timer::get() - start > 250*1000 ) {
      start = Timer::get();
      Board::ledToggle(Board::LED_RED);
    }
    ConsoleUpdate();  // Keep the comm chanel open so we can bootload out of this corner
    DisplayUpdate();  // While we wait, let screen saver kick in
  }
}

int main(void)
{
  __IO uint32_t i = 0;
  
  //Check for nvReset data
  memset( &g_app_reset, 0, sizeof(g_app_reset) );
  g_app_reset.valid = sizeof(g_app_reset) == nvResetGet( (u8*)&g_app_reset, sizeof(g_app_reset) );
  
  Timer::init();
  InitUART();
  FetchParams(); //g_flashParams = flash backup (saved via 'setmode' console cmd)
  InitConsole();
  InitRandom();
  Board::init();
  
  //Try to restore saved mode
  g_fixmode = FIXMODE_NONE;
  if ( g_flashParams.fixtureTypeOverride > FIXMODE_NONE && g_flashParams.fixtureTypeOverride < g_num_fixmodes )
    g_fixmode = g_flashParams.fixtureTypeOverride;
  
  //TODO: ^^move board init/rev stuff into fixture init
  fixtureInit();
  InitDisplay();
  Meter::init();

  Board::ledOn(Board::LED_RED);
  
  ConsolePrintf("\n----- Victor Test Fixture: %s v%d -----\n", BUILD_INFO, ((NOT_FOR_FACTORY > 0) ? 0 : g_fixtureReleaseVersion) );
  ConsolePrintf("Build date-time: %s %s\n", __DATE__, __TIME__);
  ConsolePrintf("Serial: %d (0x%04x)\n", FIXTURE_SERIAL, FIXTURE_SERIAL);
  ConsolePrintf("ConsoleMode=%u\n", g_app_reset.valid && g_app_reset.console.isInConsoleMode );
  ConsolePrintf("Fixure Rev: %s\n", Board::revString() );
  ConsolePrintf("Mode: %s (%i)\n", fixtureName(), g_fixmode );
  
  //DEBUG:
  SystemCoreClockUpdate();
  ConsolePrintf("SystemCoreClock: %uHz (%uMHz)\n", SystemCoreClock, SystemCoreClock/1000000);
  
  SetFixtureText();
  
  //DEBUG: runtime validation of the fixmode array
  if( !fixtureValidateFixmodeInfo() ) {
    ConsolePrintf("\nFixmode Info failed validation:\n");
    fixtureValidateFixmodeInfo(true); //print the info array to console, hilighting invalid entries
    ConsolePrintf("\n\n");
    MainErrorLoop(); //process uart/display so we can bootload back to safetey
  }
  
  /*/DEBUG: show all lcd screens
  MainExecution();
  void dbgCycleDisplayScreens(void);
  dbgCycleDisplayScreens();
  //-*/
  
  //DEBUG:
  //cmdDbgParseTestbench();
  
  //lockout on bad hw
  if( Board::revision() <= BOARD_REV_UNKNOWN ) {
    SetErrorText( ERROR_INCOMPATIBLE_FIX_REV );
    MainErrorLoop(); //process uart/display so we can bootload back to safetey
  }
  
  //prevent test from running if device is connected at POR (require re-insert)
  #if USE_START_BTN < 1
  g_forceStart = 0;
  g_isDevicePresent = 1;
  Board::ledOn(Board::LED_GREEN);
  WaitForDeviceOff(0);
  #endif
  
  while (1)
  {  
    MainExecution();
    DisplayUpdate();
  }
}

//-----------------------------------------------------------------------------
//                  Debug
//-----------------------------------------------------------------------------
/*
void dbgSetFixtureText(bool fcc, bool not_for_factory);
void dbgSetTestCounterText(u32 current, u32 count);
void dbgSetErrorText(u16 error);
void dbgSetOKText(void);

void dbgDelayMs(int ms) {
  uint32_t start = Timer::get();
  while( Timer::elapsedUs(start) < ms*1000 )
    ConsoleUpdate();
}

void dbgCycleDisplayScreens(void)
{
  while(1)
  {
    dbgSetFixtureText(0,0); dbgDelayMs(10000);
    dbgSetFixtureText(0,1); dbgDelayMs(10000);
    dbgSetFixtureText(1,0); dbgDelayMs(10000);
    dbgSetFixtureText(0,1); dbgDelayMs(10000);
    
    const int testCount = 5;
    for(int x=1; x<=testCount; x++) {
      dbgSetTestCounterText(x,testCount);
      dbgDelayMs(2000);
    }
    
    dbgSetErrorText( Timer::get()&0x1FF ); //random?
    dbgDelayMs(10000);
    
    dbgSetOKText();
    dbgDelayMs(10000);
  }
}

void dbgSetFixtureText(bool fcc, bool not_for_factory)
{
  const int dummyReleaseVers = 47;
  
  DisplayClear();
  DisplayBigCenteredText( fixtureName() );
  
  //reset display sizing/defaults
  DisplayTextHeightMultiplier(1);
  DisplayTextWidthMultiplier(1);
  DisplayInvert(fcc); //invert the display colors for fcc build (easy to idenfity)
  
  //Dev builds show compile date-time across the top
  if(not_for_factory) {
    DisplayMoveCursor(1,2);
    DisplayPutString("DEV-NOT FOR FACTORY!!");
    DisplayMoveCursor(10,2);
    DisplayPutString(__DATE__);
    DisplayPutString(" ");
    DisplayPutString(__TIME__);
  }
  else {
    //during development, always show build date/time
    //DisplayMoveCursor(1,2);
    //DisplayPutString(__DATE__);
    //DisplayPutString(" ");
    //DisplayPutString(__TIME__);
  }
  
  //add verision #s and other useful info
  if(fcc) {
    DisplayMoveCursor(45, 2);
    DisplayPutString("CERT/TEST ONLY");
  } else {
    if( g_fixmode == FIXMODE_PLAYPEN ) {
      DisplayMoveCursor(45, 25);
      DisplayPutString("SSID: Afix");
      DisplayPutChar('0' + ((FIXTURE_SERIAL&63) / 10)); //param sent to cozmo in playpen / robotTest.c
      DisplayPutChar('0' + ((FIXTURE_SERIAL&63) % 10)); //"
    }
  }
  
  DisplayMoveCursor(55, 2);
  DisplayPutString(BUILD_INFO);
  DisplayMoveCursor(55, fcc ? 108 : 105 );
  DisplayPutChar(fcc ? 'c' : 'v');
  DisplayPutChar(not_for_factory ? '-' : '0' + ((dummyReleaseVers / 100)));
  DisplayPutChar(not_for_factory ? '-' : '0' + ((dummyReleaseVers / 10) % 10));
  DisplayPutChar(not_for_factory ? '-' : '0' + (dummyReleaseVers % 10));
  DisplayFlip();
}

void dbgSetTestCounterText(u32 current, u32 count)
{
  DisplayClear();
  DisplayBigCenteredText("%02d/%02d", current, count);
  DisplayFlip();
}

void dbgSetErrorText(u16 error)
{
  //Board::ledOn(Board::LED_RED);  // Red
  
  DisplayClear();
  DisplayInvert(1);  
  DisplayBigCenteredText("%3i", error % 1000);
  DisplayFlip();
  
  //Timer::wait(200000);      // So nobody misses the error
}

void dbgSetOKText(void)
{
  //Board::ledOn(Board::LED_GREEN);  // Green
  DisplayClear();
  DisplayBigCenteredText("OK");
  DisplayFlip();
}
//-*/
