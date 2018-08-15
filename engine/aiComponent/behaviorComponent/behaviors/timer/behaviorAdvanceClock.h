/**
 * File: BehaviorAdvanceClock.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-30
 *
 * Description: Advance the clock display between two times over the specified interval
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAdvanceClock__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAdvanceClock__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"
#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Vector {

class BehaviorAdvanceClock : public BehaviorProceduralClock
{
public: 
  virtual ~BehaviorAdvanceClock();

  void SetAdvanceClockParams(int startTime_sec, int endTime_sec, int displayTime_sec);

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAdvanceClock(const Json::Value& config);  

  virtual void GetBehaviorJsonKeysInternal(std::set<const char*>& expectedKeys) const override;

  virtual void TransitionToShowClockInternal() override;


private:
  int _startTime_sec = 0;
  int _endTime_sec   = 0;

  BehaviorProceduralClock::GetDigitsFunction BuildTimerFunction() const;

  int GetTotalNumberOfUpdates() const {
    const float floatMs = Util::SecToMilliSec(static_cast<float>(GetTimeDisplayClock_sec()));
    return floatMs/ANIM_TIME_STEP_MS;
  }
  int GetTotalSecToAdvance() const { return std::abs(_endTime_sec - _startTime_sec);}
  bool TimeGoesUp() const { return _endTime_sec > _startTime_sec; }

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAdvanceClock__
