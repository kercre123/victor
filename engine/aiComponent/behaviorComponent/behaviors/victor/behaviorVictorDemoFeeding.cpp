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
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/dockingComponent.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const float kTimeToWaitForCube = 4.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVictorDemoFeeding::BehaviorVictorDemoFeeding(const Json::Value& config)
  : ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(FeedingEat),
                                 BEHAVIOR_CLASS(FeedingEat),
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
  // not yet waiting for food
  _imgTimeStartedWaitngForFood = 0;
  
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorVictorDemoFeeding::UpdateInternal_WhileRunning(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_imgTimeStartedWaitngForFood > 0 ) {
    // TODO:(bn) share this filter with AI whiteboard. It's the same, but also checks observation time
    BlockWorldFilter filter;
    filter.SetAllowedTypes( { ObjectType::Block_LIGHTCUBE1 } );
    filter.SetFilterFcn( [this](const ObservableObject* obj){
        return obj->IsPoseStateKnown() &&
          obj->GetPose().GetWithRespectToRoot().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS &&
          obj->GetLastObservedTime() > _imgTimeStartedWaitngForFood;
      });

    const Robot& robot = behaviorExternalInterface.GetRobot();
    const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
    const ObservableObject* obj = blockWorld.FindLocatedObjectClosestTo(robot.GetPose(), filter);
    if( obj != nullptr ) {
      PRINT_CH_INFO("Behaviors",
                    "VictorDemoFeeding.FoundFoodAfterWaiting",
                    "found food after started waiting, varifying now");
      _imgTimeStartedWaitngForFood = 0;
      CancelDelegates(false);
      TransitionToVerifyFood(behaviorExternalInterface);
    }
  }
  
  if( !IsControlDelegated() &&
      WantsToBeActivatedBehavior(behaviorExternalInterface) &&
     _imgTimeStartedWaitngForFood == 0) {
    Robot& robot = behaviorExternalInterface.GetRobot();
    IActionRunner* animAction = new TriggerAnimationAction(robot, AnimationTrigger::FeedingReactToFullCube_Normal);

    DelegateIfInControl(animAction, &BehaviorVictorDemoFeeding::TransitionToVerifyFood);
  }

  return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToVerifyFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("VerifyFood");
    
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

    DelegateIfInControl(
      turnAndVerifyAction,
      [this](ActionResult result, BehaviorExternalInterface& behaviorExternalInterface) {
        const ActionResultCategory category = IActionRunner::GetActionResultCategory(result);
        if( category == ActionResultCategory::SUCCESS ) {
          // grab the cube and check if it's on the ground (so we can eat it)
          const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
          const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
          const Robot& robot = behaviorExternalInterface.GetRobot();

          const ObjectID foodCubeID = whiteboard.Victor_GetCubeToEat();
          const ObservableObject* cube = blockWorld.GetLocatedObjectByID(foodCubeID);

          if( cube &&
              robot.GetDockingComponent().CanPickUpObjectFromGround(*cube) ) {
            TransitionToEating(behaviorExternalInterface);
            return;
          }
        }
        // otherwise, wait for the cube
        TransitionToWaitForFood(behaviorExternalInterface);
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToWaitForFood(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("WaitForFood");

  // TODO:(bn) better animation or something here, but for now just look down and wait
  Robot& robot = behaviorExternalInterface.GetRobot();
  _imgTimeStartedWaitngForFood = robot.GetLastImageTimeStamp();

  // if wait finishes without interruption, we lost the food, so also queue a frustrated anim

  CompoundActionSequential* action = new CompoundActionSequential(robot, {
      new MoveHeadToAngleAction(robot, MoveHeadToAngleAction::Preset::IDEAL_BLOCK_VIEW),
      new WaitAction(robot, kTimeToWaitForCube),
      new TriggerAnimationAction( robot, AnimationTrigger::FeedingSearchFailure )
        });

  // Update will interrupt this if the cube is seen, otherwise after the timeout the behavior will end
  DelegateIfInControl(action);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoFeeding::TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("Eating");

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
