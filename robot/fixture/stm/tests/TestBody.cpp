#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "swd.h"
#include "systest.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

#define DBG_VERBOSE(x)  x
//#define DBG_VERBOSE(x)  {}

#define BODY_CMD_OPTS   (CMD_OPTS_DEFAULT)
//#define BODY_CMD_OPTS   (CMD_OPTS_LOG_ERRORS | CMD_OPTS_REQUIRE_STATUS_CODE) /*disable exceptions*/

#define FLASH_ADDR_TEST_APP       0x08000000
#define FLASH_ADDR_SYSBOOT        0x08000000
#define FLASH_ADDR_SYSCON         0x08002000
#define FLASH_SIZE_SYSBOOT        0x00002000
#define FLASH_SIZE_SYSCON         ??

//syscon has a 4-word header that is mangled by the normal build signing process
#define SYSCON_HEADER_SIZE        16
#define SYSCON_EVIL               0x4F4D3243 /*first word of syscon header, indicates valid app*/

//-----------------------------------------------------------------------------
//                  STM Load
//-----------------------------------------------------------------------------

static void mcu_power_up_(void) {
  if( g_fixmode < FIXMODE_BODY3 ) {
    Board::enableVBAT();
  }
  DUT_RESET::write(0); //hold in reset
  DUT_RESET::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  Contacts::powerOn(); //must power through VEXT to force mcu on
}

static void mcu_power_down_(void) {
  Contacts::powerOff();
  Board::disableVBAT();
  Timer::delayMs(100);
  
  swd_stm32_deinit();
}

void stm32f030_program_(bool erase, uint32_t flash_addr, const uint8_t *bin, const uint8_t *binEnd, bool verify, const char* name)
{
  swd_stm32_init(); //safe from re-init
  
  if( erase )
    swd_stm32_erase();
  
  int size = binEnd - bin;
  if( size > 0 ) {
    ConsolePrintf("load %s: %u kB\n", name?name:"program", CEILDIV(size,1024));
    swd_stm32_flash(flash_addr, bin, binEnd, verify);
  }
  
  //if( verify )
  //  swd_stm32_verify(flash_addr, bin, binEnd);
}

//overload: non-const ptrs
void stm32f030_program_(bool erase, uint32_t flash_addr, uint8_t *bin, uint8_t *binEnd, bool verify, const char* name) {
  stm32f030_program_(erase, flash_addr, (const uint8_t*)bin, (const uint8_t*)binEnd, verify, name);
}

//-----------------------------------------------------------------------------
//                  Body Tests
//-----------------------------------------------------------------------------

bool TestBodyDetect(void)
{
  // Make sure power is not applied, as it messes up the detection code below
  Board::disableVBAT();
  Contacts::powerOff();
  
  //weakly pulled-up - it will detect as grounded when the board is attached
  DUT_TRX::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  bool detected = !DUT_TRX::read(); //true if TRX is pulled down by body board
  DUT_TRX::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things to mcu (phantom power?)
  
  return detected;
}

void TestBodyCleanup(void)
{
  mcu_power_down_();
}

static void ShortCircuitTest(void)
{
  ConsolePrintf("short circuit tests\n");
  
  //BODY3 has battery attached
  if( g_fixmode < FIXMODE_BODY3 )
  {
    const int ima_limit_VBAT = 450;
    TestCommon::powerShortVBAT( 100, ima_limit_VBAT, 2);
    Board::disableVBAT();
    Timer::delayMs(250);
  }
  
  int ima_limit_VEXT = g_fixmode == FIXMODE_BODY3 ? 1500 : 350; //BODY3 may pull charging current
  TestCommon::powerShortVEXT( 100, ima_limit_VEXT, 2);
  Contacts::powerOff();
}

static void BodyLoadTestFirmware(void)
{
  //erase and power cycle resets any strange internal states from previous fw
  mcu_power_up_();
  swd_stm32_init();
  swd_stm32_erase();
  mcu_power_down_();
  
  //Erase and flash
  mcu_power_up_();
  stm32f030_program_(0, FLASH_ADDR_TEST_APP, g_BodyTest, g_BodyTestEnd, 1, "test firmware");
  mcu_power_down_();
  
  //Power up and test comms
  Contacts::powerOn(0); //must provide VEXT to wake up the mcu
  if( g_fixmode < FIXMODE_BODY3 )
    Board::enableVBAT();
  Timer::delayMs(150); //wait for systest to boot into main and enable battery power
  Contacts::setModeTx(); //switch to comms mode
  
  //Run some tests
  cmdSend(CMD_IO_CONTACTS, "getvers", CMD_DEFAULT_TIMEOUT, BODY_CMD_OPTS);
  
  //DEBUG: console bridge, manual testing
  cmdSend(CMD_IO_CONTACTS, "echo on", CMD_DEFAULT_TIMEOUT, BODY_CMD_OPTS);
  TestCommon::consoleBridge(TO_CONTACTS,1000);
  //-*/
  
  Contacts::powerOff();
  Board::disableVBAT();
}

STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= STM32F030XX_PAGESIZE , body_buffer_size_check );

static void BodyLoadProductionFirmware(void)
{
  //erase and power cycle resets any strange internal states from previous fw
  mcu_power_up_();
  swd_stm32_init();
  swd_stm32_erase();
  mcu_power_down_();
  
  //Erase and flash boot/app
  mcu_power_up_();
  stm32f030_program_(0, FLASH_ADDR_SYSBOOT, g_BodyBoot, g_BodyBootEnd, 1, "bootloader");
  stm32f030_program_(0, FLASH_ADDR_SYSCON,  g_Body,     g_BodyEnd,     1, "application");
  swd_stm32_verify( FLASH_ADDR_SYSBOOT, g_BodyBoot, g_BodyBootEnd );
  //swd_stm32_verify( FLASH_ADDR_SYSCON,  g_Body,     g_BodyEnd     );
  mcu_power_down_();
  
  //XXX: wouldn't it be nice if we had some way to check if the app runs?
  
  //Power up and test comms
  Contacts::powerOn(0); //must provide VEXT to wake up the mcu
  if( g_fixmode < FIXMODE_BODY3 )
    Board::enableVBAT();
  Timer::delayMs(150); //wait for mcu to boot into main and enable battery power
  
  ConsolePrintf("listening for syscon boot message. press a key to skip\n");
  while( ConsoleReadChar() > -1 );
  uint32_t Tstart = Timer::get();
  while(1)
  {
    if( Timer::elapsedUs(Tstart) > 5*1000*1000 ) {
      ConsolePrintf("timeout waiting for syscon boot\n");
      //throw ERROR_TESTPORT_TIMEOUT; //XXX: exception after we get this working
      break;
    }
    
    //Debug: shortcut outta here
    if( ConsoleReadChar() > -1 )
      break;
    
    char *line = Contacts::getline();
    if( line ) {
      ConsolePrintf("%s @ %ums\n", line, Timer::elapsedUs(Tstart)/1000 );
      if( !strcmp(line, "booted") )
        break;
    }
  }
  //-*/
}

TestFunction* TestBody1GetTests(void)
{
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    BodyLoadTestFirmware,
    BodyLoadProductionFirmware,
    NULL,
  };
  return m_tests;
}

TestFunction* TestBody2GetTests(void)
{
  static TestFunction m_tests[] = {
    NULL,
  };
  return m_tests;
}

TestFunction* TestBody3GetTests(void)
{
  static TestFunction m_tests[] = {
    ShortCircuitTest,
    BodyLoadProductionFirmware,
    NULL,
  };
  return m_tests;
}

