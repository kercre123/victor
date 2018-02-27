/**
 * File: behaviorPounceWithProx.h
 *
 * Author: Kevin M. Karol
 * Created: 2017-12-6
 *
 * Description: Test out prox sensor pounce vocabulary
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPounceWithProx.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

namespace{
static const Radians tiltRads(MIN_HEAD_ANGLE);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPounceWithProx::BehaviorPounceWithProx(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedMotion
  });
  _instanceParams.inRangeCondition =
    BEIConditionFactory::CreateBEICondition( config["wantsToBeActivatedCondition"], GetDebugLabel() );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPounceWithProx::WantsToBeActivatedBehavior() const 
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::InitBehavior()
{
  _instanceParams.inRangeCondition->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorActivated()
{
  _instanceParams.inRangeCondition->SetActive(GetBEI(), true);

  const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lifetimeParams = LifetimeParams();
  _lifetimeParams.pounceAtTime_s = currentTimeInSeconds + _instanceParams.pounceTimeout_s;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorDeactivated()
{
  _instanceParams.inRangeCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetValue<ProxSensorComponent>();
  u16 dummyDistance_mm = 0;
  const bool isSensorReadingValid = proxSensor.GetLatestDistance_mm(dummyDistance_mm);
  if(isSensorReadingValid){
    // Transition conditions
    switch(_lifetimeParams.pounceState){
      case PounceState::WaitForMotion:
      {
        if(!_instanceParams.inRangeCondition->AreConditionsMet(GetBEI())){
          CancelSelf();
          return;
        }

        const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if(_lifetimeParams.motionObserved ||
           (_lifetimeParams.pounceAtTime_s < currentTimeInSeconds)){
          _lifetimeParams.pounceState = PounceState::PounceOnMotion;
        }else if(!IsControlDelegated()){
          DelegateIfInControl(
            new CompoundActionParallel({
              new MoveHeadToAngleAction(MoveHeadToAngleAction::Preset::GROUND_PLANE_VISIBLE),
              new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::CARRY)
            })
          );
        }
        break;
      }
      case PounceState::PounceOnMotion:
      {
        // Pounce on the object
        if(!IsControlDelegated()){
          const auto& robotInfo = GetBEI().GetRobotInfo();
          _lifetimeParams.prePouncePitch = robotInfo.GetPitchAngle().ToFloat();
          TransitionToPounce();
          _lifetimeParams.pounceState = PounceState::ReactToPounce;
        }
        break;
      }
      case PounceState::ReactToPounce:
      {
        break;
      }
    }
  }else{
    // Sensor reading not valid - check if lift is in way
    const bool liftBlocking = proxSensor.IsLiftInFOV();
    if(!IsControlDelegated()){
      if(liftBlocking){
        DelegateIfInControl(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::CARRY));
      }else{
        CancelSelf();
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::TransitionToResultAnim()
{
   
  const bool caught = IsFingerCaught();

  IActionRunner* newAction = nullptr;
  if( caught ) {
    newAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::PounceSuccess);
    PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult.Caught", "got it!");
  }
  else {
    newAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::PounceFail);
    PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult.Miss", "missed...");
  }
  
  DelegateIfInControl(newAction, [this](){
                                _lifetimeParams.pounceState = PounceState::WaitForMotion;
                              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPounceWithProx::IsFingerCaught()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const float liftHeightThresh = 35.5f;
  const float bodyAngleThresh = 0.02f;
  
  float robotBodyAngleDelta = robotInfo.GetPitchAngle().ToFloat() - _lifetimeParams.prePouncePitch;
  
  // check the lift angle, after some time, transition state
  PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult", "lift: %f body: %fdeg (%frad) (%f -> %f)",
                robotInfo.GetLiftHeight(),
                RAD_TO_DEG(robotBodyAngleDelta),
                robotBodyAngleDelta,
                RAD_TO_DEG(_lifetimeParams.prePouncePitch),
                robotInfo.GetPitchAngle().getDegrees());
  return robotInfo.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::TransitionToPounce()
{  
  _lifetimeParams.motionObserved = false;
  AnimationTrigger anim = AnimationTrigger::PounceWProxForward;
  
  DelegateIfInControl(new TriggerAnimationAction(anim),
                      &BehaviorPounceWithProx::TransitionToResultAnim);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::HandleWhileActivated(const EngineToGameEvent& event)
{
  using namespace ExternalInterface;
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // don't update the pounce location while we are active but go back.
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _instanceParams.minGroundAreaForPounce;
      
      const float robotOffsetX = motionObserved.ground_x;
      const float robotOffsetY = motionObserved.ground_y;
      
      // we haven't started the pounce, so update the pounce location
      if ((_lifetimeParams.pounceState == PounceState::WaitForMotion) && inGroundPlane )
      {
        float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
        if ( dist <= (_instanceParams.maxPounceDist*2) )
        {
          //Set the exit state information and then cancel the hang action
          _lifetimeParams.motionObserved = true;
        }
        else
        {
          PRINT_CH_INFO("Behaviors", "BehaviorPounceWithProx.IgnorePose",
                        "got pose, but dist of %f is too large, ignoring",
                        dist);
        }
      }
      break;
    }
    default: {
      PRINT_NAMED_ERROR("BehaviorPounceWithProx.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
}

} // namespace Cozmo
} // namespace Anki
