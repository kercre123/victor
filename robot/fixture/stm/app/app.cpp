#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "fixture.h"
#include "flash.h"
#include "flexflow.h"
#include "meter.h"
#include "nvReset.h"
#include "portable.h"
#include "random.h"
#include "robotcom.h"
#include "tests.h"
#include "timer.h"

//Set this flag to modify display info - indicates a debug/test build
#include "app_build_flags.h"
const bool g_isReleaseBuild = !NOT_FOR_FACTORY;

#include "app_release_ver.h"
u8 g_fixtureReleaseVersion = (NOT_FOR_FACTORY) ? 0 : (APP_RELEASE_VERSION);
#define BUILD_INFO "Victor DVT3"

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
static int m_last_time_ms = 0;
static error_t m_last_error = ERROR_OK;

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

// Show the name of the fixture and version information
extern int HelperTempC;
void SetFixtureText(bool reinit=0);
void SetFixtureText(bool reinit)
{
  //const bool fcc = false;
  char b[50]; int bz = sizeof(b);
  static bool inited = 0;
  if( reinit )
    inited = 0;
  
  if( !inited )
    helperLcdClear();
  
  //different colors for debug builds
  char color = 'b';
  if( !g_isReleaseBuild )
    color = m_last_error == ERROR_OK ? 'g' : 'r';
  
  //for head programming fixtures, show last written ESN on the display
  if( g_fixmode >= FIXMODE_HEAD1 && g_fixmode <= FIXMODE_HELPER1 )
    helperLcdSetLine(1, snformat(b,bz,"prev esn: 0x%08x", TestHeadGetPrevESN()) );
  else if( !inited && !g_isReleaseBuild )
    helperLcdSetLine(1, "DEV-NOT FOR FACTORY!");
  
  //debug builds show last error code and test time (color coded)
  if( !g_isReleaseBuild )
    helperLcdSetLine(2, snformat(b,bz,"DEBUG e%03i %u.%03us", m_last_error, m_last_time_ms/1000, m_last_time_ms%1000 ) );
  
  //Dev builds show compile date-time
  if( !inited && !g_isReleaseBuild )
    helperLcdSetLine(3, (__DATE__ " " __TIME__) );
  
  //show helper head internal temperature  
  int tempC = HelperTempC;
  /*tempC = tempC < -9 ? -9 : (tempC > 99 ? 99 : tempC); //bounded for 2-char display space
  if( tempC < 0 ) { //read error
    helperLcdSetLine(4, snformat(b,bz,"                   -") );
    helperLcdSetLine(5, snformat(b,bz,"                   %u", (-1)*tempC) );
  } else {
    helperLcdSetLine(4, snformat(b,bz,"                   %u", tempC/10) );
    helperLcdSetLine(5, snformat(b,bz,"                   %u", tempC%10) );
  }//-*/
  int temp_line = (g_fixmode >= FIXMODE_HEAD1 && g_fixmode <= FIXMODE_HELPER1) ? 6 : 7; //shift up for head modes (make space for os version info)
  helperLcdSetLine(temp_line, snformat(b,bz,"%iC", tempC) );
  
  //mode-specific info
  if( !inited && (g_fixmode == FIXMODE_HEAD1 || g_fixmode == FIXMODE_HELPER1) )
    helperLcdSetLine(7, snformat(b,bz,"os-ver %s", helperGetEmmcdlVersion()) );
  
  //show build info and version
  if( !inited )
    helperLcdSetLine(8, snformat(b,bz,"%-15s v%03u", BUILD_INFO, g_fixtureReleaseVersion) );
  
  helperLcdShow(0,0, color, (char*)fixtureName());
  inited = 1;
}

// Clear the display and print (index / count)
void SetTestCounterText(u32 current, u32 count)
{
  char b[10]; int bz = sizeof(b);
  helperLcdShow(1,0,'b', snformat(b,bz,"%02d/%02d", current, count) );
}

void SetErrorText(u16 error)
{
  char b[10]; int bz = sizeof(b);
  Board::ledOn(Board::LED_RED);  // Red
  helperLcdShow(1,1,'r', snformat(b,bz,"%03i", error % 1000) );
  Timer::wait(200000);      // So nobody misses the error
}

void SetOKText(void)
{
  Board::ledOn(Board::LED_GREEN);  // Green
  helperLcdShow(1,1,'g', "OK");
}

// Wait until the Device has been pulled off the fixture
void WaitForDeviceOff(bool error, int debounce_ms = 500);
void WaitForDeviceOff(bool error, int debounce_ms)
{
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  Board::powerOff(PWR_CUBEBAT,100);
  Board::powerOff(PWR_DUTPROG);
  Board::powerOff(PWR_DUTVDD);
  Board::powerOff(PWR_UAMP);
  
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

static void printFixtureInfo() {
  ConsolePrintf("fixture,hw,%i,%s,serial,%i,%04x\n", Board::revision(), Board::revString(), FIXTURE_SERIAL, FIXTURE_SERIAL);
  ConsolePrintf("fixture,build,%s,%s %s\n", BUILD_INFO, __DATE__, __TIME__);
  ConsolePrintf("fixture,fw,%03d,%s,mode,%i,%s\n", g_fixtureReleaseVersion, (NOT_FOR_FACTORY > 0 ? "debug" : "release"), g_fixmode, fixtureName() );
}

// Walk through tests one by one - logging to the PC and to the Device flash
int g_stepNumber;
static void RunTests()
{
  Board::ledOn(Board::LED_YLW); //test in-progress
  
  //char b[10]; int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, "logstart", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN );
  
  ConsolePrintf("[TEST:START]\n");
  printFixtureInfo();
  
  uint32_t Tstart = Timer::get();
  error_t error = ERROR_OK;
  try {
    for (g_stepNumber = 0; g_stepNumber < m_functionCount; g_stepNumber++) {      
      ConsolePrintf("[RUN:%u/%u]\n", g_stepNumber+1, m_functionCount);
      SetTestCounterText(g_stepNumber + 1, m_functionCount);
      m_functions[g_stepNumber]();
    }
  } catch (error_t e) {
    error = e == ERROR_OK ? ERROR_THROW_0 : e; //don't allow test to throw 'OK'
  }
  ConsolePrintf("[RESULT:%03i]\n", error);
  
  //test-specific driver and resource cleanup
  try {
    fixtureCleanup();
  } catch(error_t e) {
    ConsolePrintf("[CLEANUP-ERROR:%03i]\n", e);
  }
  
  m_last_error = error; //save the error code
  m_last_time_ms = Timer::elapsedUs(Tstart) / 1000; //Note: may be less than actual time, if test isn't using the Timer (requires polling for accuracy)
  ConsolePrintf("[TEST:END]\n", error);
  cmdSend(CMD_IO_HELPER, "logstop", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN );
  
  Board::ledOff(Board::LED_YLW); //test ended
  
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

inline void dbgBtnHandler(void)
{
  //monitor unused buttons
  int x = USE_START_BTN > 0 ? Board::BTN_2 : Board::BTN_1; //skip start btn if used for testing
  for( ; x <= Board::BTN_NUM; x++)
  {
    int edge = Board::btnEdgeDetect((Board::btn_e)x, 1000, 50);
    if( edge != 0 )
      ConsolePrintf("btn %u %s\n", x, edge > 0 ? "pressed" : "released");
    
    //XXX: debug backback test. Either kill it, or find a good home (not here)
    if( g_fixmode == FIXMODE_BACKPACK1 && edge > 0 && x == Board::BTN_4 )
      g_forceStart = 1;
    
    //XXX: hack to manually start head programming
    if( (g_fixmode >= FIXMODE_HEAD1 && g_fixmode <= FIXMODE_HELPER1) && edge > 0 && x == Board::BTN_4 )
      g_forceStart = 1;
    
    //XXX: run debug tests
    if( g_fixmode == FIXMODE_DEBUG && edge > 0 && x == Board::BTN_4 )
      g_forceStart = 1;
    
    /*/DEBUG: the ol' btn toggles an LED trick
    if( edge > 0 ) { 
      switch( (Board::btn_e)x ) {
        case Board::BTN_1:  Board::ledToggle(Board::LED_GREEN); break;
        case Board::BTN_2:  Board::ledToggle(Board::LED_YLW); break;
        case Board::BTN_3:  Board::ledToggle(Board::LED_RED); break;
        case Board::BTN_4:
          for(int b = 0; b < (int)Board::LED_NUM; b++)
            Board::ledOff((Board::led_e)b);
          break;
      }
    }
    //-*/
  }
}

static uint32_t TtempUpdate = 0;
int HelperTempC = INT_MIN;
void helper_temp_manage(bool force_update)
{
  static bool last_update_successful = 1;
  int update_interval = last_update_successful ? 10*1000*1000 : 60*1000*1000; //throttle down for debug if not connected to helper head
  bool update = force_update || Timer::elapsedUs(TtempUpdate) > update_interval;
  bool changed = 0;
  
  if( update ) {
    int tempC = helperGetTempC(); //read new temp
    last_update_successful = tempC >= 0;
    changed = tempC != HelperTempC;
    HelperTempC = tempC; //still process failures to display error code
    TtempUpdate = Timer::get();
  }
  
  if( changed )
    SetFixtureText();
}

// Repeatedly scan for a device, then run through the tests when it appears
static void MainExecution()
{
  m_functions = fixtureGetTests();
  m_functionCount = fixtureGetTestCount();
  
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
    if( g_forceStart )
      g_forceStart = 0, start = 1;
    
    //reset button state & timing
    if( start )
      Tstart = 0, Board::btnEdgeDetect(Board::BTN_1, -1, -1); //reset state machine
  
  #else
    //legacy DUT connect starts the test
    if( isPresent || g_forceStart )
      g_forceStart = 0, start = 1;
  
  #endif
  
  if( start ) {
    SetTestCounterText(0, m_functionCount);
    TryToRunTests();
    Board::ledOff(Board::LED_RED);
    Board::ledOff(Board::LED_GREEN);
    Board::ledOff(Board::LED_YLW);
  }
  helper_temp_manage(start); //force immediate temp read after test completion
  
  //DEBUG
  dbgBtnHandler();
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
  }
}

int main(void)
{
  __IO uint32_t i = 0;
  
  //Check for nvReset data
  memset( &g_app_reset, 0, sizeof(g_app_reset) );
  g_app_reset.valid = sizeof(g_app_reset) == nvResetGet( (u8*)&g_app_reset, sizeof(g_app_reset) );
  
  Timer::init();
  FLEXFLOW::init();
  FetchParams(); //g_flashParams = flash backup (saved via 'setmode' console cmd)
  InitConsole();
  InitRandom();
  Board::init();
  
  //Try to restore saved mode
  g_fixmode = FIXMODE_NONE;
  if ( g_flashParams.fixtureTypeOverride > FIXMODE_NONE && g_flashParams.fixtureTypeOverride < g_num_fixmodes ) {
    if( g_fixmode_info[g_fixmode].name != NULL ) { //Prevent invalid modes
      g_fixmode = g_flashParams.fixtureTypeOverride;
    }
  }
  
  //TODO: ^^move board init/rev stuff into fixture init
  fixtureInit();
  Meter::init();

  Board::ledOn(Board::LED_RED);
  
  ConsolePrintf("\n----- Victor Test Fixture: -----\n");
  printFixtureInfo();
  helper_temp_manage(1);
  SetFixtureText();
  
  //DEBUG: runtime validation of the fixmode array
  if( !fixtureValidateFixmodeInfo() ) {
    ConsolePrintf("\nFixmode Info failed validation:\n");
    fixtureValidateFixmodeInfo(true); //print the info array to console, hilighting invalid entries
    ConsolePrintf("\n\n");
    MainErrorLoop(); //process console so we can bootload back to safetey
  }
  
  //lockout on bad hw
  if( Board::revision() <= BOARD_REV_INVALID ) {
    SetErrorText( ERROR_INCOMPATIBLE_FIX_REV );
    MainErrorLoop(); //process console so we can bootload back to safetey
  }
  
  //validate cube bin
  int cubeErr = ERROR_OK;
  try { cubebootSignature(0); /*supress dbg print*/ } catch(int e) { cubeErr=e; }
  if( cubeErr != ERROR_OK ) {
    try { cubebootSignature(1); /*print error detail*/ } catch(int e) {}
    SetErrorText( cubeErr );
    MainErrorLoop(); //process console so we can bootload back to safetey
  }
  
  //prevent test from running if device is connected at POR (require re-insert)
  #if USE_START_BTN < 1
  g_forceStart = 0;
  g_isDevicePresent = 1;
  Board::ledOn(Board::LED_GREEN);
  WaitForDeviceOff(0);
  #endif
  
  Board::ledOff(Board::LED_RED);
  Board::ledOff(Board::LED_GREEN);
  Board::ledOff(Board::LED_YLW);
  
  try { fixtureCleanup(); } catch(error_t e) {}
  
  while (1)
  {  
    MainExecution();
  }
}

