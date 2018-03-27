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
#include "contacts.h"
#include "dut_uart.h"
#include "fixture.h"
#include "meter.h"
#include "motorled.h"
#include "nvReset.h"
#include "random.h"
#include "tests.h"
#include "timer.h"

//static uint32_t detect_ms = 0;
//static uint32_t Tdetect;

//start a test run by flipping detected to true (auto clear at end)
void TestDebugSetDetect(int ms) {
  //Tdetect = Timer::get(); //reset the timebase
  //detect_ms = ms;
}

bool TestDebugDetect(void) {
  //return Timer::elapsedUs(Tdetect) < 1000*detect_ms;
  return false;
}

void TestDebugCleanup(void)
{
}

void TestDebugInit(void)
{
}

void TestDebugAlternateScriptTimeout(void)
{
  static int ofs = -2;
  srand(Timer::get());
  //uint32_t esn = ((rand()&0xffff)<<16) | (rand()&0xffff);
  
  const int shell_timeout_s = 5;
  int script_delay_s = shell_timeout_s + ofs;
  ofs *= (-1);
  
  char b[40]; const int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"dutprogram %u %u", shell_timeout_s, script_delay_s), (shell_timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
}

void TestDebugInspectArgs(void)
{
  const int shell_timeout_s = 5;
  const uint32_t esn = 0xabcd1234;
  char b[40]; const int bz = sizeof(b);
  
  snformat(b,bz,"dutprogram %u %08x", shell_timeout_s, esn);
  cmdSend(CMD_IO_HELPER, b, (shell_timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
  
  snformat(b,bz,"dutprogram %u %08x param1 param2 param3", shell_timeout_s, esn);
  cmdSend(CMD_IO_HELPER, b, (shell_timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
  
  snformat(b,bz,"dutprogram %u %08x   param1  param2  param3 ", shell_timeout_s, esn);
  cmdSend(CMD_IO_HELPER, b, (shell_timeout_s+10)*1000, CMD_OPTS_DEFAULT | CMD_OPTS_ALLOW_STATUS_ERRS );
}

TestFunction* TestDebugGetTests(void)
{
  static TestFunction m_tests[] = {
    TestDebugInit,
    TestDebugInspectArgs,
    //TestDebugInspectArgs,
    //TestDebugInspectArgs,
    //TestDebugAlternateScriptTimeout,
    //TestDebugAlternateScriptTimeout,
    NULL
  };
  return m_tests;
}

