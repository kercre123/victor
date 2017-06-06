/**
 * File: BehaviorAcknowledgeCubeMoved.h
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeCubeMoved_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeCubeMoved_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorAcknowledgeCubeMoved : public IBehavior
{
  
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeCubeMoved(Robot& robot, const Json::Value& config);


public:
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeObject& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}

protected:
  virtual void StopInternal(Robot& robot) override;
  virtual Result InitInternal(Robot& robot) override;
  
  // allows the reaction to interrupt itself
  virtual IBehavior::Status UpdateInternal(Robot& robot) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;


private:
  Robot& _robot;
  
  mutable ObjectID _activeObjectID; //Most recent move - object to turn towards
  
  enum class State {
    PlayingSenseReaction,
    TurningToLastLocationOfBlock,
    ReactingToBlockAbsence,
    ReactingToBlockPresence
  };
  
  State _state;
  //  active object observed - play acknowledge reaction
  bool _activeObjectSeen;
  
  void TransitionToPlayingSenseReaction(Robot& robot);
  void TransitionToTurningToLastLocationOfBlock(Robot& robot);
  void TransitionToReactingToBlockAbsence(Robot& robot);
  
  void HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg);

  void SetState_internal(State state, const std::string& stateName);

  
}; // class BehaviorAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeCubeMoved_H__
