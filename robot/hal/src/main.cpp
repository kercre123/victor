#include <stdio.h>
#include <chrono>
#include <thread>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include  "../spine/cc_commander.h"
#include "anki/cozmo/shared/factory/emrHelper.h"

#include "platform/victorCrashReports/google_breakpad.h"

// For development purposes, while HW is scarce, it's useful to be able to run on phones
#ifdef HAL_DUMMY_BODY
  #define HAL_NOT_PROVIDING_CLOCK
#endif

int shutdownSignal = 0;
int shutdownCounter = 2;

#if FACTORY_TEST
// Count of how many ticks we have been on the charger
uint8_t seenChargerCnt = 0;
// How many ticks we need to be on the charger before
// we will stay on
static const uint8_t MAX_SEEN_CHARGER_CNT = 5;
bool wasPackedOutAtBoot = false;
#endif

static void Cleanup(int signum)
{
  Anki::Cozmo::Robot::Destroy();

  // Need to HAL::Step() in order for light commands to go down to robot
  // so set shutdownSignal here to signal process shutdown after
  // shutdownCounter more tics of main loop.
  shutdownSignal = signum;
}


int main(int argc, const char* argv[])
{
  using Result = Anki::Result;

  mlockall(MCL_FUTURE);

  struct sched_param params;
  params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &params);

  signal(SIGTERM, Cleanup);

  static char const* filenamePrefix = "robot";
  GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);

  if (argc > 1) {
    ccc_set_shutdown_function(Cleanup);
    ccc_parse_command_line(argc-1, argv+1);
  }

  AnkiInfo("robot.main", "Starting robot process");

  //Robot::Init calls HAL::INIT before anything else.
  // TODO: move HAL::Init here into HAL main.
  const Result result = Anki::Cozmo::Robot::Init(&shutdownSignal);
  if (result != Result::RESULT_OK) {
    AnkiError("robot.main.InitFailed", "Unable to initialize (result %d)", result);
    GoogleBreakpad::UnInstallGoogleBreakpad();
    sync();
    if (shutdownSignal == SIGTERM) {
      return 0;
    } else if (shutdownSignal != 0) {
      return shutdownSignal;
    } else {
      return result;
    }
  }

  auto start = std::chrono::steady_clock::now();
#if FACTORY_TEST
  auto timeOfPowerOn = start;
  wasPackedOutAtBoot = Anki::Cozmo::Factory::GetEMR()->fields.PACKED_OUT_FLAG;
#endif

  for (;;) {
    //HAL::Step should never return !OK, but if it does, best not to trust its data.
    if (Anki::Cozmo::HAL::Step() == Anki::RESULT_OK) {
      if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK) {
        AnkiError("robot.main", "MainExecution failed");
        GoogleBreakpad::UnInstallGoogleBreakpad();
        return -1;
      }
    }

    auto end = std::chrono::steady_clock::now();
#ifdef HAL_NOT_PROVIDING_CLOCK
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::chrono::duration<double, std::micro> sleepTime = std::chrono::milliseconds(5) - elapsed;
    std::this_thread::sleep_for(sleepTime);
    ///printf("Main tic: %lld, Sleep time: %f us\n", elapsed.count(), sleepTime.count());
#endif
    //printf("TS: %d\n", Anki::Cozmo::HAL::GetTimeStamp() );
    start = end;

#if FACTORY_TEST
    // If we are packed out and have not yet seen the charger
    if (wasPackedOutAtBoot &&
        shutdownSignal == 0 &&
        seenChargerCnt < MAX_SEEN_CHARGER_CNT)
    {
      // Need to be on the charger for some number of ticks
      if (Anki::Cozmo::HAL::BatteryIsOnCharger())
      {
        seenChargerCnt++;
      }
      else
      {
        seenChargerCnt = 0;
      }

      // If it has been more than 15 seconds since power on and we
      // have not been on the charger then shutdown
      auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - timeOfPowerOn);
      if (elapsed > std::chrono::seconds(15))
      {
        Anki::Cozmo::Robot::Destroy();

        sync();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        Anki::Cozmo::HAL::Shutdown();
        break;
      }
    }
#endif

    if (shutdownSignal != 0 && --shutdownCounter == 0) {
      AnkiInfo("robot.main.shutdown", "%d", shutdownSignal);
      GoogleBreakpad::UnInstallGoogleBreakpad();
      sync();
      exit(0);
    }
  }
  return 0;
}

#include "spine/spine.h"
int main_test(int argc, const char* argv[])
{
  mlockall(MCL_FUTURE);

  struct sched_param params;
  params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &params);

  signal(SIGTERM, Cleanup);

  spine_test_setup();

  while (1) {
    spine_test_loop_once();
    Anki::Cozmo::Robot::step_MainExecution();
  }

  if (shutdownSignal != 0 && --shutdownCounter == 0) {
    AnkiInfo("robot.main.shutdown", "%d", shutdownSignal);
    exit(shutdownSignal);
  }
  return 0;
}
