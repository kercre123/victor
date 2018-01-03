#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "cube.h"
#include "display.h"
#include "fixture.h"
#include "flash.h"
#include "meter.h"
#include "motorled.h"
#include "nvReset.h"
#include "portable.h"
#include "random.h"
#include "testport.h"
#include "tests.h"
#include "timer.h"
#include "uart.h"

static uint32_t detect_ms = 0;
static uint32_t Tdetect;

//start a test run by flipping detected to true (auto clear at end)
void TestDebugSetDetect(int ms) {
  Tdetect = Timer::get(); //reset the timebase
  detect_ms = ms;
}

bool TestDebugDetect(void) {
  return Timer::elapsedUs(Tdetect) < (1000*detect_ms);
}

void TestDebugCleanup(void)
{
}

//---------------------------------------------

void TestDebugStart(void) {
  ConsolePrintf("Debug. Simulate test flow for helper head development\n");
}

void TestDebug1(void) {
  ConsolePrintf("Debug test stage 1\n");
  Timer::delayMs(500);
}

void TestDebug2(void) {
  ConsolePrintf("Debug test stage 2...");
  Timer::delayMs(750);
  ConsolePrintf("ok\n");
}

void TestDebug3(void) {
  ConsolePrintf("Debug test stage 3\n");
  ConsolePrintf(">command to cube or body\n");
  for(int x=0; x<8; x++) {
    ConsolePrintf(" adr %05x some page operation?\n", 1<<x );
    Timer::delayMs(100);
  }
  ConsolePrintf("<response 0\n");
}

void TestDebug4(void)
{
  char b[40]; int bz = sizeof(b);
  
  ConsolePrintf("DEBUG: verify helper is properly responding to unknown commands\n");
  cmdSend(CMD_IO_HELPER, "not-a-real-command 1234", 500, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS); //& ~CMD_OPTS_EXCEPTION_EN
  if( cmdStatus() == 0 ) {
    ConsolePrintf("unknown commands MUST NOT return success! (e=%i)\n", cmdStatus() );
    throw ERROR_TESTPORT_RSP_BAD_ARG;
  }
  
  srand( Timer::get() );
  ConsolePrintf("DEBUG: test helper's shell timeout feature\n");
  int timeout_s = (rand() & 0x7) + 3; //3-10s random
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"shell-timeout-test %u", timeout_s), (timeout_s+5)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
  if( cmdStatus() != 0 ) {
    ConsolePrintf("shell returned error %i\n", cmdStatus() );
    //throw ERROR_TESTPORT_RSP_BAD_ARG;
  }
}

void TestDebugEnd() {
  const int displayms = 3000;
  ConsolePrintf("display result for ~%ums\n", displayms);
  TestDebugSetDetect(displayms); //detect for another second or so (to display result)
  
  //take turns showing success/error
  static bool e = 1;
  e = !e;
  if(e)
    throw ERROR_TIMEOUT;
}

TestFunction* TestDebugGetTests(void)
{
  static TestFunction m_tests[] = {
    TestDebugStart,
    TestDebug2,
    TestDebug3,
    TestDebug4,
    TestDebugEnd,
    NULL
  };
  return m_tests;
}
