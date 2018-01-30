#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "testcommon.h"
#include "timer.h"

#define HEAD_CMD_OPTS   (CMD_OPTS_DEFAULT)
//#define HEAD_CMD_OPTS   (CMD_OPTS_LOG_ERRORS | CMD_OPTS_REQUIRE_STATUS_CODE) /*disable exceptions*/

bool TestHeadDetect(void)
{
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

void TestHeadDutProgram(void)
{
  const int timeout_s = 120 + 60; //XXX: first working version clocked in at ~120s. Add some margin.
  char b[40]; int bz = sizeof(b);
  
  //XXX: get a unique id/sn for the head
  srand( Timer::get() );
  int head_id = (rand() << 16) | (rand() & 0xffff); //RAND_MAX is < 0xFFFFffff
  
  //Power cycle to reset DUT head
  Board::powerOff(PWR_VEXT);
  Board::powerOff(PWR_VBAT);
  Timer::delayMs(1000);
  
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
  
  //helper head does the rest
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"dutprogram %u [id=%08x]", timeout_s, head_id), (timeout_s+10)*1000, HEAD_CMD_OPTS | CMD_OPTS_ALLOW_STATUS_ERRS );
  if( cmdStatus() != 0 )
  {
    //map programming error to appropriate fixture error
    ConsolePrintf("programming failed: error %i\n", cmdStatus() );
    throw ERROR_TESTPORT_CMD_FAILED;
  }
}

void TestHeadDutProgramManual(void)
{
  //FORCE_USB signal at POR preps for OS bootload
  DUT_CS::reset();
  DUT_CS::init(MODE_OUTPUT); //voltage divider gnd (disabled for detect)
  DUT_RESET::set(); 
  DUT_RESET::init(MODE_OUTPUT); //2.8V -> 1.8V logic level
  
  Board::powerOff(PWR_VBAT);
  Board::powerOff(PWR_VEXT);
  TestCommon::powerOnProtected(PWR_VBAT, 200, 9999, 2); 
  TestCommon::powerOnProtected(PWR_VEXT, 200, 9999, 2);
  
  int timeout_s = 5*60;
  ConsolePrintf("FORCE_USB=1 for %us\n", timeout_s);
  while( timeout_s-- > 0 ) {
    Timer::delayMs(1000);
    if( timeout_s && timeout_s % 10 == 0 )
      ConsolePrintf("%us\n", timeout_s);
  }
}

TestFunction* TestHead1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestHeadDutProgram,
    NULL,
  };
  return m_tests;
}

TestFunction* TestHead2GetTests(void)
{
  static TestFunction m_tests[] = {
    TestHeadDutProgramManual,
    NULL
  };
  return m_tests;
}

