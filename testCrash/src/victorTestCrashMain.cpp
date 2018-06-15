/**
* File: victorTestCrashMain.cpp
*
* Author: Paul Terry
* Created: 04/03/18
*
* Description: Victor test crash tool for LE
*
* Copyright: Anki, inc. 2018
*
*/
#include "anki/cozmo/shared/cozmoConfig.h"

#include "platform/victorCrashReports/google_breakpad.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/victorLogger.h"

#include <stdio.h>
#include <iomanip>
#include <thread>
#include <signal.h>

using namespace Anki;

#define LOG_CHANNEL "VictorTestCrash"


namespace {
  bool gShutdown = false;

  enum
  {
    CT_NullPointer,
    CT_Abort,
    CT_StackOverflow,
    CT_SIGABRT,
    CT_SIGFPE,
    CT_SIGILL,
    CT_SIGSEGV,
    NumCrashTypes
  };

  static const char* crashTypeNames[NumCrashTypes] = {
    "null",
    "abort",
    "stackoverflow",
    "SIGABRT",
    "SIGFPE",
    "SIGILL",
    "SIGSEGV"
  };

  static const float kDefaultCrashTime_s = 1.0f;
  static const float kDefaultTimeout_s = 5.0f;
}

static void Shutdown(int signum)
{
  LOG_INFO("CozmoTestCrash.Shutdown", "Shutdown on signal %d", signum);
  gShutdown = true;
}

static void DoNullPointer()
{
  volatile int* a = reinterpret_cast<volatile int*>(0);
  *a = 42;
}

static void DoAbort()
{
  abort();
}

static int DoStackOverflow(int counter);
static int StackOverflowFoo(int counter)
{
  static const int sizeOfArray = 268435456;
  volatile int largeArray[sizeOfArray];
  largeArray[counter % sizeOfArray] = counter;
  return DoStackOverflow(counter);
}
static int DoStackOverflow(int counter)
{
  if (counter < 0) {
    return counter;
  }
  static const int sizeOfArray = 268435456;
  volatile int largeArray[sizeOfArray];
  largeArray[counter % sizeOfArray] = counter;
  return StackOverflowFoo(counter - 1);
}

void DoSIGABRT() { raise(SIGABRT); } __attribute__((noinline))
void DoSIGFPE()  { raise(SIGFPE); }  __attribute__((noinline))
void DoSIGILL()  { raise(SIGILL); }  __attribute__((noinline))
void DoSIGSEGV() { raise(SIGSEGV); } __attribute__((noinline))


static void CrashMe(int crashType, int cmdLineSpecialCount)
{
  switch (crashType)
  {
    case CT_NullPointer:
      {
        DoNullPointer();
      }
      break;

    case CT_Abort:
      {
        DoAbort();
      }
      break;

    case CT_StackOverflow:
      {
        const int result = DoStackOverflow(cmdLineSpecialCount);
        // Should not get here unless we fail to crash.  But
        // we want something here to use 'result' so this all
        // doesn't just get optimized away.
        printf("stack overflow call returned %i\n", result);
      }
      break;

    case CT_SIGABRT:  DoSIGABRT();  break;
    case CT_SIGFPE:   DoSIGFPE();   break;
    case CT_SIGILL:   DoSIGILL();   break;
    case CT_SIGSEGV:  DoSIGSEGV();  break;
  }
}


int main(int argc, char* argv[])
{
  signal(SIGTERM, Shutdown);

  static const char* filenamePrefix = "testCrash";
  GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);

  // - create and set logger
  Util::VictorLogger logger("vic-testcrash");
  Util::gLoggerProvider = &logger;

  if (argc < 2)
  {
    printf("Usage:  vic-testcrash <crashType> [<secsBeforeCrash>] [<secsTimeout>]\n");
    printf("crashType: can be one of:  ");
    for (const auto& p : crashTypeNames)
    {
      printf("%s ", p);
    }
    printf("\nsecsBeforeCrash: float number of seconds to wait before crashing (defaults to %f)\n",
           kDefaultCrashTime_s);
    printf("secsTimeout: float number of seconds before program ends normally (defaults to %f)\n",
           kDefaultTimeout_s);
    exit(1);
  }

  int crashType = NumCrashTypes;
  for (int i = 0; i < NumCrashTypes; i++)
  {
    if (!strcmp(crashTypeNames[i], argv[1]))
    {
      crashType = i;
    }
  }
  if (crashType == NumCrashTypes)
  {
    printf("Unrecognized crash type \"%s\"\n", argv[1]);
    exit(1);
  }

  float crashTime_s = kDefaultCrashTime_s;
  if (argc > 2)
  {
    crashTime_s = std::stof(argv[2]);
  }

  float timeout_s = kDefaultTimeout_s;
  if (argc > 3)
  {
    timeout_s = std::stof(argv[3]);
  }

  int cmdLineSpecialCount = 1 << 29;
  if (argc > 4)
  {
    cmdLineSpecialCount = std::stoi(argv[4]);
  }

  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();

  srand(time(NULL));

  // Set the target time for the end of the first frame
  static const uint32_t TEST_CRASH_TIME_STEP_US = 100'000;
  auto targetEndFrameTime = runStart + (microseconds)(TEST_CRASH_TIME_STEP_US);
  const auto crashTime_ms = static_cast<uint32_t>(crashTime_s * 1000.0f);
  const auto crashTime = runStart + (milliseconds)(crashTime_ms);
  const auto timeout_ms = static_cast<uint32_t>(timeout_s * 1000.0f);
  const auto timeoutTime = runStart + (milliseconds)(timeout_ms);

  while (!gShutdown)
  {
    const auto tickNow = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickNow);

    // Now we ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
    // allows other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    std::this_thread::sleep_for(std::max(minimumSleepTime_us, remaining_us));

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(TEST_CRASH_TIME_STEP_US);

    if (tickNow > crashTime)
    {
      printf("Crashing (%s)...\n", argv[1]);
      CrashMe(crashType, cmdLineSpecialCount);
      // We should not get here unless we fail to crash
      break;
    }

    if (tickNow > timeoutTime)
    {
      printf("Timeout reached\n");
      break;
    }
  }

  LOG_INFO("victorTestCrashMain.exit", "exit(0)");
  printf("Exiting victorTestCrashMain normally...no crash\n");
  Util::gLoggerProvider = nullptr;

  GoogleBreakpad::UnInstallGoogleBreakpad();

  exit(0);
}
