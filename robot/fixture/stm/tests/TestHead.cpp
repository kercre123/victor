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

static uint32_t m_previous_esn = 0;
uint32_t TestHeadGetPrevESN(void) {
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

void TestHeadDutProgram(void)
{
  const int timeout_s = 210 + 90; //XXX: DVT3 from LE helper clocked in at ~210s. Add some margin.
  char b[40]; int bz = sizeof(b);
  
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
  
  //DEBUG
  if( g_fixmode == FIXMODE_HEAD2 ) {
    int timeout_s = 10;
    ConsolePrintf("---DELAY %us FOR MANUAL FORCE_USB---\n", timeout_s);
    while( timeout_s-- > 0 ) {
      Timer::delayMs(1000);
      if( timeout_s ) //&& timeout_s % 10 == 0 )
        ConsolePrintf("%us\n", timeout_s);
    }
  }
  
  //provision ESN
  headnfo.esn = fixtureGetSerial();
  m_previous_esn = headnfo.esn; //even if programming fails, report the (now unusable) ESN
  
  //helper head does the rest
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"dutprogram %u %08x", timeout_s, headnfo.esn), (timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
  if( cmdStatus() != 0 )
  {
    //map programming error to appropriate fixture error
    switch( cmdStatus() )
    {
      case ERROR_OUT_OF_CLOUD_CERTS:
        throw ERROR_OUT_OF_CLOUD_CERTS;
      default:
        throw ERROR_TESTPORT_CMD_FAILED;
    }
  }
}

static void HeadFlexFlowReport(void)
{
  char b[80]; const int bz = sizeof(b);
  snformat(b,bz,"<flex> ESN %08x\n", headnfo.esn);
  ConsoleWrite(b);
  FLEXFLOW::write(b);
}

TestFunction* TestHead1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestHeadDutProgram,
    HeadFlexFlowReport,
    NULL,
  };
  return m_tests;
}

TestFunction* TestHead2GetTests(void)
{
  static TestFunction m_tests[] = {
    TestHeadDutProgram,
    HeadFlexFlowReport,
    NULL,
  };
  return m_tests;
}

