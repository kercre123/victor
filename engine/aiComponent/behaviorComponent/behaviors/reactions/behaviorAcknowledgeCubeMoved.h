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
#include "coretech/common/engine/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorAcknowledgeCubeMoved : public ICozmoBehavior
{
  
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeCubeMoved(const Json::Value& config);


public:
  virtual bool WantsToBeActivatedBehavior() const override;
  void SetObjectToAcknowledge(const ObjectID& objID){_activeObjectID = objID;}

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void OnBehaviorDeactivated() override;
  virtual void OnBehaviorActivated() override;
  
  // allows the reaction to interrupt itself
  virtual void BehaviorUpdate() override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
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
  
  void TransitionToPlayingSenseReaction();
  void TransitionToTurningToLastLocationOfBlock();
  void TransitionToReactingToBlockAbsence();
  
  void HandleObservedObject(const ExternalInterface::RobotObservedObject& msg);

  void SetState_internal(State state, const std::string& stateName);

  
}; // class BehaviorAcknowledgeCubeMoved

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeCubeMoved_H__
