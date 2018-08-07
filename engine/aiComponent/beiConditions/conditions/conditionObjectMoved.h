/**
* File: ConditionObjectMoved.h
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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectMoved_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectMoved_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

#include "coretech/common/engine/objectIDs.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

namespace ExternalInterface {
struct ObjectMoved;
struct ObjectStoppedMoving;
struct ObjectUpAxisChanged;
struct RobotObservedObject;
}


//Forward declarations
class ReactionObjectData;

class ConditionObjectMoved : public IBEICondition, private IBEIConditionEventHandler
{
public:
  ConditionObjectMoved(const Json::Value& config);

  virtual ~ConditionObjectMoved();

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  

private:
  mutable std::vector<ReactionObjectData> _reactionObjects; //Tracking for all active objects
  
  typedef std::vector<ReactionObjectData>::iterator Reaction_iter;
  
  void HandleObjectMoved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectMoved& msg);
  void HandleObjectStopped(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectStoppedMoving& msg);
  void HandleObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectUpAxisChanged& msg);
  void HandleObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  void HandleRobotDelocalized();
  Reaction_iter GetReactionaryIterator(ObjectID objectID);

  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectMoved_H__
