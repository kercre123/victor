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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorAcknowledgeCubeMoved : public ICozmoBehavior
{
  
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorAcknowledgeCubeMoved(const Json::Value& config);


public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  void SetObjectToAcknowledge(const ObjectID& objID){_activeObjectID = objID;}

protected:
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // allows the reaction to interrupt itself
  virtual ICozmoBehavior::Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:  
  ObjectID _activeObjectID; //Most recent move - object to turn towards
  
  enum class State {
    PlayingSenseReaction,
    TurningToLastLocationOfBlock,
    ReactingToBlockAbsence,
    ReactingToBlockPresence
  };
  
  State _state;
  //  active object observed - play acknowledge reaction
  bool _activeObjectSeen;
  
  void TransitionToPlayingSenseReaction(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToTurningToLastLocationOfBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToReactingToBlockAbsence(BehaviorExternalInterface& behaviorExternalInterface);
  
  void HandleObservedObject(const BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);

  void SetState_internal(State state, const std::string& stateName);

  
}; // class BehaviorAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeCubeMoved_H__
