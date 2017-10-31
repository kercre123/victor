/**
 * File: behaviorVictorDemoFeeding.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-12
 *
 * Description: Observing demo version of feeding. This is a replacement for the old feeding activity
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorVictorDemoFeeding.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/robot.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVictorDemoFeeding::BehaviorVictorDemoFeeding(const Json::Value& config)
  : ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast(BehaviorID::FeedingEat,
                                 BehaviorClass::FeedingEat,
                                 _eatFoodBehavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_eatFoodBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVictorDemoFeeding::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  return whiteboard.Victor_HasCubeToEat();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorVictorDemoFeeding::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorVictorDemoFeeding::UpdateInternal_WhileRunning(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  if( !IsControlDelegated() && WantsToBeActivatedBehavior(behaviorExternalInterface) ) {
    Robot& robot = behaviorExternalInterface.GetRobot();
    IActionRunner* animAction = new TriggerAnimationAction(robot, AnimationTrigger::FeedingReactToFullCube_Normal);

    DelegateIfInControl(animAction, &BehaviorVictorDemoFeeding::TransitionToVerifyFood);
  }

  return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToVerifyFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!IsControlDelegated(), "BehaviorVictorDemoFeeding.TransitionToVerifyFood.NotInControl");

  const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  const ObjectID foodCubeID = whiteboard.Victor_GetCubeToEat();
  
  if( foodCubeID.IsSet() ) {
    Robot& robot = behaviorExternalInterface.GetRobot();
    const bool verifyWhenDone = true;
    IActionRunner* turnAndVerifyAction = new TurnTowardsObjectAction(robot,
                                                                     foodCubeID,
                                                                     Radians{M_PI_F},
                                                                     verifyWhenDone);

    DelegateIfInControl( turnAndVerifyAction, &BehaviorVictorDemoFeeding::TransitionToEating );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!IsControlDelegated(), "BehaviorVictorDemoFeeding.TransitionToEating.NotInControl");

  const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  const ObjectID foodCubeID = whiteboard.Victor_GetCubeToEat();

  if( foodCubeID.IsSet() ) {
    _eatFoodBehavior->SetTargetObject( foodCubeID );

    if( _eatFoodBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
      DelegateIfInControl( behaviorExternalInterface, _eatFoodBehavior.get() );
      // TODO:(bn) error handling if this doesn't work
    }
  }
}

}
}
