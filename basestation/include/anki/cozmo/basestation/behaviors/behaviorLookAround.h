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
#include "anki/common/basestation/math/pose.h"
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
    LookingForObject,
    ExamineFoundObject,
    WaitToFinishExamining
  };
  
  enum class Destination {
    North = 0,
    West,
    South,
    East,
    Center,
    
    Count
  };
  
  // How long to wait before we trying to look around again (among other factors)
  constexpr static f32 kLookAroundCooldownDuration = 11;
  // How fast to rotate when looking around
  constexpr static f32 kDegreesRotatePerSec = 25;
  // The default radius (in mm) we assume exists for us to move around in
  constexpr static f32 kDefaultSafeRadius = 250;
  
  State _currentState = State::Inactive;
  Destination _currentDestination = Destination::North;
  f32 _lastLookAroundTime = 0;
  Pose3d _moveAreaCenter;
  f32 _safeRadius = kDefaultSafeRadius;
  u32 _currentDriveActionID = 0;
  
  std::vector<Signal::SmartHandle> _eventHandles;
  std::set<ObjectID> _recentObjects;
  std::set<ObjectID> _oldBoringObjects;
  u32 _numObjectsToLookAt = 0;
  
  void StartMoving();
  Pose3d GetDestinationPose(Destination destination);
  void ResetBehavior(float currentTime_sec);
  Destination GetNextDestination(Destination oldDest);
  
  void HandleObjectObserved(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
  void HandleCompletedAction(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
  void HandleRobotPutDown(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorLookAround_H__
