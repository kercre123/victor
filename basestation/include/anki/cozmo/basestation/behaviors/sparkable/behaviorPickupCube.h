/**
 * File: behaviorPickupCube.h
 *
 * Author: Molly Jameson
 * Created: 2016-07-26
 *
 * Description:  Spark to pick up and put down
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPickUpCube_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPickUpCube_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
namespace BlockConfigurations{
enum class ConfigurationType;
}

class CompoundActionSequential;
class ObservableObject;

  
class BehaviorPickUpCube : public IBehavior
{
  using super = IBehavior;
protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPickUpCube(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual void   StopInternalFromDoubleTap(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}

  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;
  
  virtual void UpdateTargetBlocksInternal(const Robot& robot) const override { CheckForNearbyObject(robot); }
  
  virtual std::set<AIWhiteboard::ObjectUseIntention> GetBehaviorObjectUseIntentions() const override { return {AIWhiteboard::ObjectUseIntention::PickUpAnyObject}; }
  
private:
  
  void CheckForNearbyObject(const Robot& robot) const;
  
  enum class DebugState
  {
    DoingInitialReaction,
    PickingUpCube,
    DriveWithCube,
    PutDownCube,
    DoingFinalReaction,
  };
  
  void FailedToPickupObject(Robot& robot);
  
  void TransitionToDoingInitialReaction(Robot& robot);
  void TransitionToPickingUpCube(Robot& robot);
  void TransitionToDriveWithCube(Robot& robot);
  void TransitionToPutDownCube(Robot& robot);
  void TransitionToDoingFinalReaction(Robot& robot);

  mutable ObjectID    _targetBlockID;
  mutable TimeStamp_t _lastBlockWorldCheck_s;
  
  // loaded in from config
  bool _shouldPutCubeBackDown;
  
  std::vector<BlockConfigurations::ConfigurationType> _configurationsToIgnore;
  

}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif
