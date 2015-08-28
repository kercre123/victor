/**
 * File: behaviorLookAround.h
 *
 * Author: Lee
 * Created: 08/13/15
 *
 * Description: Behavior for looking around the environment for stuff to interact with.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface { class MessageEngineToGame; }
  
class BehaviorLookAround : public IBehavior
{
public:
  BehaviorLookAround(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(float currentTime_sec) const override;
  
  virtual Result Init() override;
  
  virtual Status Update(float currentTime_sec) override;
  
  // Finish placing current object if there is one, otherwise good to go
  virtual Result Interrupt(float currentTime_sec) override;
  
  virtual bool GetRewardBid(Reward& reward) override;
  
private:
  enum class State {
    Inactive,
    StartLooking,
    ContinueLooking,
    LookingForObject,
    ExamineFoundObject,
    WaitToFinishExamining
  };
  
  // Constant that decides how long to wait before we trying to look around again (among other factors)
  static const float kLookAroundCooldownDuration;
  static const float kDegreesRotatePerSec;
  
  State _currentState;
  f32 _lastLookAroundTime;
  f32 _totalRotation;
  
  std::vector<Signal::SmartHandle> _eventHandles;
  std::set<ObjectID> _recentObjects;
  std::set<ObjectID> _oldBoringObjects;
  u32 _numObjectsToLookAt = 0;
  
  void ResetBehavior(float currentTime_sec);
  void HandleObjectObserved(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
  void HandleCompletedAction(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
