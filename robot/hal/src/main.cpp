#include <stdio.h>
#include <chrono>
#include <thread>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/shared/factory/emrHelper.h"

// For development purposes, while HW is scarce, it's useful to be able to run on phones
#ifdef HAL_DUMMY_BODY
  #define HAL_NOT_PROVIDING_CLOCK
#endif


int shutdownSignal = 0;
int shutdownCounter = 2;

void Cleanup(int signum)
{
  Anki::Cozmo::Robot::Destroy();

  // Need to HAL::Step() in order for light commands to go down to robot
  // so set shutdownSignal here to signal process shutdown after 
  // shutdownCounter more tics of main loop.
  shutdownSignal = signum;
}


int main(int argc, const char* argv[])
{
  mlockall(MCL_FUTURE);

  struct sched_param params;
  params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &params);

  signal(SIGTERM, Cleanup);

  AnkiEvent("robot.main", "Starting robot process");

  //Robot::Init calls HAL::INIT before anything else.
  // TODO: move HAL::Init here into HAL main.
  Anki::Cozmo::Robot::Init();

  auto start = std::chrono::steady_clock::now();
  auto stoppedCharging = start;

  for (;;) {
    //HAL::Step should never return !OK, but if it does, best not to trust its data.
    if (Anki::Cozmo::HAL::Step() == Anki::RESULT_OK) {
      if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK) {
        AnkiError("robot.main", "MainExecution failed");
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

    if (shutdownSignal == 0) {
      if (Anki::Cozmo::Factory::GetEMR()->PACKED_OUT_FLAG) {
        if (Anki::Cozmo::HAL::BatteryIsOnCharger()) {
          stoppedCharging = end;
        } else {
          auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - stoppedCharging);
          if (elapsed > std::chrono::seconds(15)) {
            Anki::Cozmo::Robot::Destroy();

            sync();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            Anki::Cozmo::HAL::Shutdown();
            break;
          }
        }
      }
    }

    if (shutdownSignal != 0 && --shutdownCounter == 0) {
      sync();
      AnkiInfo("robot.main.shutdown", "%d", shutdownSignal);
      exit(shutdownSignal);
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