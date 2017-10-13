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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/robot.h"

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
void BehaviorVictorDemoFeeding::GetAllDelegatesInternal(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_eatFoodBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVictorDemoFeeding::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return _foodCubeID.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:(bn) remove UpdateInternal_WhileActivatable and use BehaviorUpdate instead
void BehaviorVictorDemoFeeding::UpdateInternal_WhileActivatable(BehaviorExternalInterface& behaviorExternalInterface)
{
  // the "food cube" is always a specific type and it must be upright

  BlockWorldFilter filter;
  filter.SetAllowedTypes( { ObjectType::Block_LIGHTCUBE1 } );
  filter.SetFilterFcn( [](const ObservableObject* obj){
      return obj->IsPoseStateKnown() &&
        obj->GetPose().GetWithRespectToRoot().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;
    });

  const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
  const ObservableObject* obj = blockWorld.FindLocatedObjectClosestTo(behaviorExternalInterface.GetRobot().GetPose(),
                                                                      filter);
  if( obj != nullptr ) {
    _foodCubeID = obj->GetID();
  }
  else {
    _foodCubeID.UnSet();
  }  
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
  if( !IsControlDelegated() && _foodCubeID.IsSet() ) {
    Robot& robot = behaviorExternalInterface.GetRobot();
    IActionRunner* animAction = new TriggerAnimationAction(robot, AnimationTrigger::FeedingReactToFullCube_Normal);

    DelegateIfInControl(animAction, &BehaviorVictorDemoFeeding::TransitionToVerifyFood);
  }

  return Status::Running;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToVerifyFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!IsControlDelegated(), "BehaviorVictorDemoFeeding.TransitionToVerifyFood.NotInControl");
  
  if( _foodCubeID.IsSet() ) {
    Robot& robot = behaviorExternalInterface.GetRobot();
    const bool verifyWhenDone = true;
    IActionRunner* turnAndVerifyAction = new TurnTowardsObjectAction(robot,
                                                                     _foodCubeID,
                                                                     Radians{M_PI_F},
                                                                     verifyWhenDone);

    DelegateIfInControl( turnAndVerifyAction, &BehaviorVictorDemoFeeding::TransitionToEating );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!IsControlDelegated(), "BehaviorVictorDemoFeeding.TransitionToEating.NotInControl");

  if( _foodCubeID.IsSet() ) {
    _eatFoodBehavior->SetTargetObject( _foodCubeID );

    if( _eatFoodBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
      DelegateIfInControl( behaviorExternalInterface, _eatFoodBehavior.get() );
      // TODO:(bn) error handling if this doesn't work
    }
  }
}

}
}
