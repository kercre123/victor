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

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}

class CompoundActionSequential;
class ObservableObject;

  
class BehaviorPickUpCube : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPickUpCube(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  void HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  
  enum class State
  {
    // Main reaction states
    DoingInitialReaction,
    PickingUpCube,
    DriveWithCube,
    PutDownCube,
    DoingFinalReaction,
  };

  State _state;
  
  void TransitionToDoingInitialReaction(Robot& robot);
  void TransitionToPickingUpCube(Robot& robot);
  void TransitionToDriveWithCube(Robot& robot);
  void TransitionToPutDownCube(Robot& robot);
  void TransitionToDoingFinalReaction(Robot& robot);

  ObjectID    _targetBlock;
  void SetState_internal(State state, const std::string& stateName);
  

}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif
