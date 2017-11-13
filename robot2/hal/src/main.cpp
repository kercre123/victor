#include <stdio.h>
#include <chrono>
#include <thread>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/cozmoBot.h"

#include "anki/cozmo/robot/robot_io.h"

// For development purposes, while HW is scarce, it's useful to be able to run on phones
#ifdef USING_ANDROID_PHONE
#define HAL_NOT_PROVIDING_CLOCK 1
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
  signal(SIGTERM, Cleanup);

  AnkiEvent("robot.main", "Starting robot process");

  //Robot::Init calls HAL::INIT before anything else.
  // TODO: move HAL::Init here into HAL main.
  Anki::Cozmo::Robot::Init();

  auto start = std::chrono::steady_clock::now();

  for (;;) {
    //HAL::Step should never return !OK, but if it does, best not to trust its data.
    Anki::Cozmo::RobotIO::Step(); //send pending, rcv new data
    if (Anki::Cozmo::HAL::Step() == Anki::RESULT_OK) {
      if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK) {
        AnkiError("robot.main", "MainExecution failed");
        return -1;
      }
    }

#ifdef HAL_NOT_PROVIDING_CLOCK
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    start = end;
    std::chrono::duration<double, std::micro> sleepTime = std::chrono::milliseconds(5) - elapsed;
#else
    std::chrono::duration<double, std::micro> sleepTime = std::chrono::milliseconds(1);
#endif
    if (shutdownSignal != 0 && --shutdownCounter == 0) {
      AnkiInfo("robot.main.shutdown", "%d", shutdownSignal);
      exit(shutdownSignal);
    }
    std::this_thread::sleep_for(sleepTime);
  }
  return 0;
}
