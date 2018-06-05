#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "fixture.h"
#include "flexflow.h"
#include "hwid.h"
#include "meter.h"
#include "portable.h"
#include "testcommon.h"
#include "timer.h"

static headid_t headnfo;
static const int CURRENT_HEAD_HW_REV = HEADID_HWREV_PVT;
static const int CURRENT_HEAD_MODEL = 1;

static uint32_t m_previous_esn = ~0;
uint32_t TestHeadGetPrevESN(void)
{
  //initialize on first use (app display at boot)
  if( m_previous_esn & 0x80000000 ) {
    m_previous_esn = fixtureReadSerial(); //readSerial returns the next s/n to be allocated
    if( fixtureReadSequence() > 0 ) //adjust to last used esn
      m_previous_esn -= 1;
  }
  
  return m_previous_esn;
}

bool TestHeadDetect(void)
{
  memset( &headnfo, 0, sizeof(headnfo) );
  
  // Make sure power is not applied, as it messes up the detection code below
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT); //just in case
  
  //weakly pulled-up - it will detect as grounded when the board is attached
  DUT_CS::init(MODE_INPUT, PULL_NONE); //Z
  DUT_RESET::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  
  bool detected = !DUT_RESET::read(); //true if signal is pulled down by dut circuit
  DUT_RESET::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things to mcu (phantom power?)
  
  return detected;
}

void TestHeadCleanup(void)
{
  //if( g_fixmode == FIXMODE_HEAD2 )
  //  Board::powerOn(PWR_VEXT), return;
  
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  
  DUT_RESET::init(MODE_INPUT, PULL_NONE);
  DUT_CS::init(MODE_INPUT, PULL_NONE);
}

void TestHeadForceBoot(void)
{
  //Power cycle to reset DUT head
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  Timer::delayMs(1000);
  
  ConsolePrintf("Force USB Boot\n");
  
  //FORCE_USB signal at POR preps for OS bootload
  DUT_CS::reset();
  DUT_CS::init(MODE_OUTPUT); //voltage divider gnd (disabled for detect)
  DUT_RESET::set(); 
  DUT_RESET::init(MODE_OUTPUT); //2.8V -> 1.8V logic level
  
  //power on (with short circuit detection)
  const int ima_limit_VBAT = 9999;
  const int ima_limit_VEXT = 9999; //XXX: what's our max inrush current?
  Board::powerOff(PWR_VBAT);
  Board::powerOff(PWR_VEXT);
  TestCommon::powerOnProtected(PWR_VBAT, 200, ima_limit_VBAT, 2); 
  TestCommon::powerOnProtected(PWR_VEXT, 200, ima_limit_VEXT, 2);
  
  //DEBUG
  if( g_fixmode == FIXMODE_HEAD2 ) {
    int timeout_s = 5;
    ConsolePrintf("---DELAY %us FOR MANUAL FORCE_USB---\n", timeout_s);
    while( timeout_s-- > 0 ) {
      Timer::delayMs(1000);
      if( timeout_s ) //&& timeout_s % 10 == 0 )
        ConsolePrintf("%us\n", timeout_s);
    }
  }
}

void TestHeadDutProgram(void)
{
  const int timeout_s = 90 + 60; //XXX: DVT4-RC clocked in at ~87s. Add some margin.
  char b[50]; int bz = sizeof(b);
  
  //provision ESN
  headnfo.esn = g_fixmode == FIXMODE_HEAD1 ? fixtureGetSerial() : 0x00100000;
  m_previous_esn = headnfo.esn; //even if programming fails, report the (now unusable) ESN
  int hwrev = g_isReleaseBuild && g_fixmode == FIXMODE_HEAD1 ? CURRENT_HEAD_HW_REV : HEADID_HWREV_DEBUG;
  int model = g_fixmode == FIXMODE_HEAD1 ? CURRENT_HEAD_MODEL : 1 /*debug*/ ;
  
  //helper head does the rest
  snformat(b,bz,"dutprogram %u %08x %04x %04x %s", timeout_s, headnfo.esn, hwrev, model, g_fixmode == FIXMODE_HEAD1 ? "" : "nocert");
  cmdSend(CMD_IO_HELPER, b, (timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
  if( cmdStatus() >= ERROR_HEADPGM && cmdStatus() < ERROR_HEADPGM_RANGE_END ) //headprogram exit code range
    throw cmdStatus();
  else if( cmdStatus() != 0 )
    throw ERROR_HEADPGM; //default
}

static void TestHeadFuseLockdown(void)
{
  //Power cycle to blow the fuses (only if proper secdat was written)
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  Timer::delayMs(1000);
  
  ConsolePrintf("blow secdat fuses...");
  Board::powerOn(PWR_VEXT);
  Board::powerOn(PWR_VBAT);
  Timer::delayMs(5000); //thar she blows!
  ConsolePrintf("done\n");
  
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
}

static void HeadFlexFlowReport(void)
{
  FLEXFLOW::printf("<flex> ESN %08x </flex>\n", headnfo.esn);
}

TestFunction* TestHead1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestHeadForceBoot,
    TestHeadDutProgram,
    TestHeadFuseLockdown,
    HeadFlexFlowReport,
    NULL,
  };
  return m_tests;
}
TestFunction* TestHelper1GetTests(void) {
  return TestHead1GetTests();
}

TestFunction* TestHead2GetTests(void) {
  return TestHead1GetTests();
}

