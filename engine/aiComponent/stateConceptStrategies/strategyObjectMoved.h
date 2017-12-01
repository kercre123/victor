/**
* File: StrategyObjectMoved.h
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy that indicates when an object Cozmo cant see starts moving
*
* Copyright: Anki, Inc. 2017
*
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyObjectMoved_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyObjectMoved_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategyEventHandler.h"

#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class StateConceptStrategyMessageHelper;


//Forward declarations
class ReactionObjectData;

class StrategyObjectMoved : public IStateConceptStrategy, private IStateConceptStrategyEventHandler
{
public:
  StrategyObjectMoved(BehaviorExternalInterface& behaviorExternalInterface,
                      IExternalInterface& robotExternalInterface,
                      const Json::Value& config);

  virtual ~StrategyObjectMoved();

protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  

private:
  mutable std::vector<ReactionObjectData> _reactionObjects; //Tracking for all active objects
  
  typedef std::vector<ReactionObjectData>::iterator Reaction_iter;
  
  void HandleObjectMoved(BehaviorExternalInterface& behaviorExternalInterface, const ObjectMoved& msg);
  void HandleObjectStopped(BehaviorExternalInterface& behaviorExternalInterface, const ObjectStoppedMoving& msg);
  void HandleObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ObjectUpAxisChanged& msg);
  void HandleObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  void HandleRobotDelocalized();
  Reaction_iter GetReactionaryIterator(ObjectID objectID);

  std::unique_ptr<StateConceptStrategyMessageHelper> _messageHelper;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StrategyObjectMoved_H__
