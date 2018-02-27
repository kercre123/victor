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
#include "coretech/common/engine/utils/timer.h"

#include <ctime>
#include <ratio>
#include <chrono>

namespace Anki {
namespace Cozmo {
  
namespace{
  
}

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
{
  _activeTimer = std::make_shared<TimerHandle>(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility::~TimerUtility()
{

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
