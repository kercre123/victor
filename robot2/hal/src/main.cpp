#include <stdio.h>
#include <chrono>
#include <thread>

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"


int main (void)
{
  printf("Starting robot process\n");
  
  Anki::Cozmo::Robot::Init();

  auto start = std::chrono::steady_clock::now();
  
  for(;;)
  {
    Anki::Cozmo::HAL::Step();
    if (Anki::Cozmo::Robot::step_MainExecution() != Anki::RESULT_OK)
    {
      printf("ERROR: MainExecution failed\n");
      return -1;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::chrono::duration<double, std::micro> sleepTime = std::chrono::milliseconds(5) - elapsed;
    std::this_thread::sleep_for(sleepTime);
    start = end;
    //printf("Main tic: %lld, Sleep time: %f us\n", elapsed.count(), sleepTime.count());
    //printf("TS: %d\n", Anki::Cozmo::HAL::GetTimeStamp() );
  }
  
  return 0;

}
