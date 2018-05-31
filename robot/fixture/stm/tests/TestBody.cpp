#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "binaries.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "fixture.h"
#include "flexflow.h"
#include "hwid.h"
#include "meter.h"
#include "portable.h"
#include "swd.h"
#include "systest.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

#define TESTBODY_DEBUG 1

#if TESTBODY_DEBUG > 0
#define DBG_VERBOSE(x)  x
#else
#define DBG_VERBOSE(x)  {}
#endif

#define FLASH_ADDR_TEST_APP       0x08000000
#define FLASH_ADDR_SYSBOOT        0x08000000
#define FLASH_ADDR_SYSCON         0x08002000
#define FLASH_SIZE_SYSBOOT        0x00002000
#define FLASH_SIZE_SYSCON         ??

//syscon has a 4-word header that is mangled by the normal build signing process
#define SYSCON_HEADER_SIZE        16
#define SYSCON_EVIL               0x4F4D3243 /*first word of syscon header, indicates valid app*/

static const int CURRENT_BODY_HW_REV = BODYID_HWREV_PVT;

//-----------------------------------------------------------------------------
//                  STM Load
//-----------------------------------------------------------------------------

static void mcu_power_up_(void) {
  if( g_fixmode < FIXMODE_BODY3 ) {
    Board::powerOn(PWR_VBAT, 0);
  }
  DUT_RESET::write(0); //hold in reset
  DUT_RESET::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  Board::powerOn(PWR_VEXT); //must power through VEXT to force mcu on
}

static void mcu_power_down_(void) {
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  Timer::delayMs(100); //extra delay for power discharge
  
  swd_stm32_deinit();
}

static void mcu_swd_init_(void) {
  try { swd_stm32_init(); /*safe from re-init*/ }
  catch(int e) {
    throw e;
  }
}

static void mcu_flash_erase_(void) {
  try { swd_stm32_erase(); }
  catch(int e) {
    throw e;
  }
}

static void mcu_flash_program_(uint32_t flash_addr, const uint8_t *bin, const uint8_t *binEnd, bool verify, const char* name)
{
  mcu_swd_init_();
  
  int size = binEnd - bin;
  if( size < 1 ) {
    ConsolePrintf("BAD_ARG: mcu_flash_program_() size=%i\n", size );
    throw ERROR_BAD_ARG;
  }
  
  ConsolePrintf("load %s: %u kB\n", name?name:"program", CEILDIV(size,1024));
  try { swd_stm32_flash(flash_addr, bin, binEnd, verify); }
  catch(int e) {
    throw e;
  }
}

static void mcu_flash_verify_(uint32_t flash_addr, const uint8_t* bin_start, const uint8_t* bin_end) {
  try { swd_stm32_verify(flash_addr, bin_start, bin_end); }
  catch(int e) {
    throw e;
  }
}

static void mcu_jtag_lock_(void) {
  try { swd_stm32_lock_jtag(); }
  catch(int e) {
    throw e;
  }
}

//-----------------------------------------------------------------------------
//                  Body Tests
//-----------------------------------------------------------------------------

static bodyid_t bodyid;

bool TestBodyDetect(void)
{
  bodyid.hwrev = BODYID_HWREV_EMPTY;
  bodyid.model = BODYID_MODEL_EMPTY;
  bodyid.esn = BODYID_ESN_EMPTY; //reset serial
  
  // Make sure power is not applied, as it messes up the detection code below
  Board::powerOff(PWR_VBAT);
  Board::powerOff(PWR_VEXT);
  
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

static void BodyShortCircuitTest(void)
{
  ConsolePrintf("short circuit tests\n");
  
  //BODY3 has battery attached
  if( g_fixmode < FIXMODE_BODY3 )
  {
    const int ima_limit_VBAT = 450;
    Board::powerOff(PWR_VBAT);
    TestCommon::powerOnProtected(PWR_VBAT, 100, ima_limit_VBAT, 2);
    Board::powerOff(PWR_VBAT);
    Timer::delayMs(250);
  }
  
  int ima_limit_VEXT = g_fixmode == FIXMODE_BODY3 ? 1200 : 350; //BODY3 may pull charging current
  Board::powerOff(PWR_VEXT);
  TestCommon::powerOnProtected(PWR_VEXT, 100, ima_limit_VEXT, 2);
  Board::powerOff(PWR_VEXT);
  Timer::delayMs(250);
}

static void BodyTryReadSerial(void)
{
  mcu_power_up_();
  mcu_swd_init_();
  bodyid.hwrev = swd_read32(FLASH_ADDR_SYSBOOT+16);
  bodyid.model = swd_read32(FLASH_ADDR_SYSBOOT+20);
  bodyid.esn    = swd_read32(FLASH_ADDR_SYSBOOT+24); /*ein0*/
  uint32_t ein1 = swd_read32(FLASH_ADDR_SYSBOOT+28);
  uint32_t ein2 = swd_read32(FLASH_ADDR_SYSBOOT+32);
  uint32_t ein3 = swd_read32(FLASH_ADDR_SYSBOOT+36);
  mcu_power_down_();
  
  ConsolePrintf("read ESN %08x %08x %08x %08x\n", bodyid.esn, ein1, ein2, ein3);
  ConsolePrintf("read hwrev %u model %u\n", bodyid.hwrev, bodyid.model);
  
  //invalidate ESN if there is garbage in the other fields - generate a new one
  if( !BODYID_HWREV_IS_VALID(bodyid.hwrev) || !BODYID_MODEL_IS_VALID(bodyid.model) || 
      ein1 != BODYID_ESN_EMPTY || ein2 != BODYID_ESN_EMPTY || ein3 != BODYID_ESN_EMPTY )
  {
    bodyid.hwrev = BODYID_HWREV_EMPTY;
    bodyid.model = BODYID_MODEL_EMPTY;
    bodyid.esn = BODYID_ESN_EMPTY;
  }
}

static void BodyLoadTestFirmware(void)
{
  //erase and power cycle resets any strange internal states from previous fw
  mcu_power_up_();
  mcu_swd_init_();
  mcu_flash_erase_();
  mcu_power_down_();
  
  //Erase and flash
  mcu_power_up_();
  mcu_flash_program_(FLASH_ADDR_TEST_APP, g_BodyTest, g_BodyTestEnd, 1, "test firmware");
  mcu_power_down_();
  
  //Power up and test comms
  Board::powerOn(PWR_VEXT,0); //must provide VEXT to wake up the mcu
  if( g_fixmode < FIXMODE_BODY3 )
    Board::powerOn(PWR_VBAT, 0);
  Timer::delayMs(150); //wait for systest to boot into main and enable battery power
  Contacts::powerOff(); //forces discharge
  Contacts::setModeRx(); //switch to comms mode
  
  //DEBUG:
  if( g_fixmode < FIXMODE_BODY1 ) {
    TestCommon::consoleBridge(TO_CONTACTS,5000,0,BRIDGE_OPT_CHG_DISABLE);
  }
  
  //flush systest line buffer
  Contacts::write("\n\n\n");
  
  //Run some tests
  cmdSend(CMD_IO_CONTACTS, "getvers");
  
  //XXX: --- add hardware tests here ---
  
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
}

const int BodyBootMaxSize = 0x2000;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= STM32F030XX_PAGESIZE , body_buffer_size_check );
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= BodyBootMaxSize , body_boot_buffer_size_check );

static void BodyLoadProductionFirmware(void)
{
  //erase and power cycle resets any strange internal states from previous fw
  mcu_power_up_();
  mcu_swd_init_();
  mcu_flash_erase_();
  mcu_power_down_();
  
  //assign ESN & HW ids (preserve, if exist)
  bodyid.model = BODYID_MODEL_BLACK_STD;
  if( bodyid.hwrev == BODYID_HWREV_EMPTY && g_fixmode == FIXMODE_BODY1 )
    bodyid.hwrev = CURRENT_BODY_HW_REV; //only set valid hwrev in production mode
  if( !bodyid.esn || bodyid.esn == BODYID_ESN_EMPTY )
  {
    if( g_fixmode == FIXMODE_BODY1 )
      bodyid.esn = fixtureGetSerial(); //get next 12.20 esn in the sequence
    else
      bodyid.esn = BODYID_ESN_EMPTY;
  }
  
  //buffer the bootlaoder image and insert ESN/HW info
  const int bodybootSize = g_BodyBootEnd - g_BodyBoot;
  uint8_t* bodyboot = app_global_buffer;
  memcpy( bodyboot, g_BodyBoot, bodybootSize );
  *((uint32_t*)(bodyboot+16)) = bodyid.hwrev;
  *((uint32_t*)(bodyboot+20)) = bodyid.model;
  *((uint32_t*)(bodyboot+24)) = bodyid.esn;
  
  //Erase and flash boot/app
  ConsolePrintf("load: ESN %08x, hwrev %u, model %u\n", bodyid.esn, bodyid.hwrev, bodyid.model);
  mcu_power_up_();
  mcu_flash_program_(FLASH_ADDR_SYSBOOT, bodyboot, bodyboot+bodybootSize, 1, "bootloader");
  mcu_flash_program_(FLASH_ADDR_SYSCON,  g_Body,     g_BodyEnd,     1, "application");
  mcu_flash_verify_( FLASH_ADDR_SYSBOOT, bodyboot, bodyboot+bodybootSize );
  //mcu_flash_verify_( FLASH_ADDR_SYSCON,  g_Body,     g_BodyEnd     );
  if( g_fixmode == FIXMODE_BODY1 && g_isReleaseBuild ) {
    ConsolePrintf("LOCKING JTAG. POINT OF NO RETURN!\n");
    mcu_jtag_lock_();
  }
  mcu_power_down_();
}

//Power cycle and listen for the "booted" message from sycon application
static void BodyBootcheckProductionFirmware(void)
{
  mcu_power_down_();
  
  //DEBUG:
  if( g_fixmode < FIXMODE_BODY1 ) {
    //only works if body has a battery?
    TestCommon::consoleBridge(TO_CONTACTS,3000,0,BRIDGE_OPT_CHG_DISABLE);
  }
  
  //Power up and test comms
  Board::powerOn(PWR_VEXT,0); //must provide VEXT to wake up the mcu
  if( g_fixmode < FIXMODE_BODY3 ) //BODY3 has battery installed
    Board::powerOn(PWR_VBAT, 0);
  Timer::delayMs(150); //wait for mcu to boot into main and enable battery power
  Contacts::setModeRx(); //switch to comms mode
  
  bool allow_skip = g_fixmode < FIXMODE_BODY1; //DEBUG
  ConsolePrintf("listening for syscon boot message. %s\n", (allow_skip ? "press a key to skip" : "") );
  
  while( ConsoleReadChar() > -1 );
  uint32_t Tstart = Timer::get();
  while(1)
  {
    if( Timer::elapsedUs(Tstart) > 4*1000*1000 ) { //nominal delay is ~2.5s
      ConsolePrintf("timeout waiting for syscon boot\n");
      throw ERROR_BODY_NO_BOOT_MSG;
    }
    
    //Debug: shortcut outta here
    if( ConsoleReadChar() > -1 && allow_skip )
      break;
    
    char *line = Contacts::getline();
    if( line ) {
      ConsolePrintf("'%s' @ %ums\n", line, Timer::elapsedUs(Tstart)/1000 );
      if( !strcmp(line, "booted") )
        break;
    }
  }
}

//check if this board has been factory programmed (swd lockout)
static void BodyCheckProgramLockout(void)
{
  ConsolePrintf("test swd connection\n");
  error_t err = ERROR_OK;
  
  mcu_power_up_();
  try {
    mcu_swd_init_();
    swd_read32(FLASH_ADDR_SYSBOOT+16); //dummy read
  } catch(int e) { 
    err = e;
  }
  
  if( err == ERROR_SWD_CHIPINIT || err == ERROR_SWD_INIT_FAIL )
  {
    ConsolePrintf("swd [chip]init fail %i. boot checking...\n", err);
    
    //if we don't detect boot msg, assume unprogrammed and throw the original swd error
    try { BodyBootcheckProductionFirmware(); } catch(...) { throw err; }
    
    throw ERROR_BODY_PROGRAMMED; //detected boot msg. this board is programmed/locked
  }
  
  mcu_power_down_();
  if( err != ERROR_OK )
    throw err;
}

static void BodyFlexFlowReport(void)
{
  FLEXFLOW::printf("<flex> ESN %08x HwRev %u Model %u </flex>\n", bodyid.esn, bodyid.hwrev, bodyid.model);
}

static void BodyChargeContactElectricalDebug(void)
{
  BodyLoadTestFirmware();
  
  Board::powerOn(PWR_VEXT,0); //must provide VEXT to wake up the mcu
  if( g_fixmode < FIXMODE_BODY3 )
    Board::powerOn(PWR_VBAT, 0);
  Timer::delayMs(150); //wait for systest to boot into main and enable battery power
  Contacts::setModeRx(); //switch to comms mode
  
  int x=0;
  while( ConsoleReadChar() > -1 ); //clear console input
  while(1)
  {
    //DEBUG: various TX/RX lengths to test VEXT analog performance
    ConsolePrintf("---START MEASUREMENTS NOW!---(%u)\n",x+1);
    uint32_t Twait = Timer::get();
    while( Timer::elapsedUs(Twait) < 5*1000*1000 ) {
      if( ConsoleReadChar() > -1 )
        break;
    }
    
    //VEXT idle high to pre-charge some gate circuits
    Contacts::powerOn(); //Contacts::setModeTx();
    Timer::delayMs(25);
    Contacts::powerOff();
    Timer::delayMs(100);
    
    cmdSend(CMD_IO_CONTACTS, "getvers", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
    cmdSend(CMD_IO_CONTACTS, "getvers", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
    Timer::delayMs(50);
    cmdSend(CMD_IO_CONTACTS, "testcommand  extra  long  string  to  measure  gate  rise  abcd  1234567890", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
    Timer::delayMs(50);
    
    cmdSend(CMD_IO_CONTACTS, "getvers", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
    Timer::delayMs(100);
    
    Contacts::setModeTx();
    Timer::delayMs(50);
    cmdSend(CMD_IO_CONTACTS, "getvers plus some extra ignored params to create a really really long transmit cyclce", CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
    
    if( ConsoleReadChar() > -1 ) //exit test on console input
      break;
  }
  
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
}

TestFunction* TestBody0GetTests(void)
{
  static TestFunction m_tests_0a[] = {
    BodyShortCircuitTest,
    BodyCheckProgramLockout,
    BodyTryReadSerial, //--skip serial read to force blank state
    //BodyLoadTestFirmware,
    BodyChargeContactElectricalDebug,
    NULL,
  };
  static TestFunction m_tests_0[] = {
    BodyShortCircuitTest,
    BodyCheckProgramLockout,
    BodyTryReadSerial,
    BodyLoadTestFirmware,
    BodyLoadProductionFirmware,
    BodyBootcheckProductionFirmware,
    NULL,
  };
  return (g_fixmode == FIXMODE_BODY0A) ? m_tests_0a : m_tests_0;
}

TestFunction* TestBody1GetTests(void)
{
  static TestFunction m_tests[] = {
    BodyShortCircuitTest,
    BodyCheckProgramLockout,
    BodyTryReadSerial,
    BodyLoadTestFirmware,
    BodyLoadProductionFirmware,
    BodyBootcheckProductionFirmware,
    BodyFlexFlowReport,
    NULL,
  };
  return m_tests;
}

TestFunction* TestBody2GetTests(void)
{
  static TestFunction m_tests[] = {
    BodyShortCircuitTest,
    BodyCheckProgramLockout,
    BodyTryReadSerial, //skip serial read to force blank state (generate new ESN)
    //BodyLoadTestFirmware,
    BodyLoadProductionFirmware,
    BodyBootcheckProductionFirmware,
    BodyFlexFlowReport,
    NULL,
  };
  return m_tests;
}

TestFunction* TestBody3GetTests(void)
{
  static TestFunction m_tests[] = {
    BodyShortCircuitTest,
    BodyCheckProgramLockout,
    BodyTryReadSerial,
    BodyLoadProductionFirmware,
    //BodyBootcheckProductionFirmware,
    BodyFlexFlowReport,
    NULL,
  };
  return m_tests;
}

