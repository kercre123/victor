#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app.h"
#include "board.h"
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

static uint32_t detect_ms = 0;
static uint32_t Tdetect;

//start a test run by flipping detected to true (auto clear at end)
void TestDebugSetDetect(int ms) {
  Tdetect = Timer::get(); //reset the timebase
  detect_ms = ms;
}

bool TestDebugDetect(void) {
  return Timer::elapsedUs(Tdetect) < 1000*detect_ms;
}

void TestDebugCleanup(void)
{
}

void TestDebugInit(void)
{
}

void TestDebugProcess(void)
{
}

TestFunction* TestDebugGetTests(void)
{
  static TestFunction m_tests[] = {
    TestDebugInit,
    TestDebugProcess,
    NULL
  };
  return m_tests;
}

