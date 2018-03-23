/**
* File: timerUtility.h
*
* Author: Kevin M. Karol
* Created: 2/5/18
*
* Description: Keep track of information relating to the user facing
* timer utility
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/timerUtility.h"
#include "engine/aiComponent/timerUtilityDevFunctions.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

#include <ctime>
#include <ratio>
#include <chrono>

namespace Anki {
namespace Cozmo {
  
namespace{
Anki::Cozmo::TimerUtility* sTimerUtility = nullptr;
}

CONSOLE_VAR(u32, kAdvanceTimerSeconds,   "TimerUtility.AdvanceTimerSeconds", 60);
CONSOLE_VAR(u32, kAdvanceTimerAndAnticSeconds,   "TimerUtility.AdvanceTimerAndAnticSeconds", 60);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimer(ConsoleFunctionContextRef context)
{ 
  AdvanceTimerBySeconds(kAdvanceTimerSeconds);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimerAndAntic(ConsoleFunctionContextRef context)
{  
  AdvanceTimerBySeconds(kAdvanceTimerAndAnticSeconds);
  AdvanceAnticBySeconds(kAdvanceTimerAndAnticSeconds);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceTimerBySeconds(int seconds)
{  
  if(sTimerUtility != nullptr){
    sTimerUtility->AdvanceTimeBySeconds(seconds);
  }
}



CONSOLE_FUNC(AdvanceTimer, "TimerUtility.AdvanceTimer");
CONSOLE_FUNC(AdvanceTimerAndAntic, "TimerUtility.AdvanceTimerAndAntic");


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimerHandle::GetSystemTime_s()
{
  using namespace std::chrono;
  auto time_s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
  return static_cast<int>(time_s);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TimerUtility
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::TimerUtility()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::TimerUtility){
  if( sTimerUtility != nullptr ) {
    PRINT_NAMED_WARNING("TimerUtility.Constructor.MultipleInstances","TimerUtility instance exists already");
  }
  sTimerUtility = this;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::~TimerUtility()
{
  sTimerUtility = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::SharedHandle TimerUtility::StartTimer(int timerLength_s)
{
  ANKI_VERIFY(_activeTimer == nullptr,
              "TimerUtility.StartTimer.TimerAlreadySet", 
              "Current design says we don't overwrite timers - remove this verify if that changes");
  _activeTimer = std::make_shared<TimerHandle>(timerLength_s);
  return _activeTimer;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerUtility::ClearTimer()
{
  _activeTimer.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int TimerUtility::GetSystemTime_s() const
{
  using namespace std::chrono;
  auto time_s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
  return static_cast<int>(time_s);
}


} // namespace Cozmo
} // namespace Anki
