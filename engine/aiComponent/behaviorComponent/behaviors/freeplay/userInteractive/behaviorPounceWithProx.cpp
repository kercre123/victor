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
namespace Vector {

namespace{
static const Radians tiltRads(MIN_HEAD_ANGLE);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPounceWithProx::InstanceConfig::InstanceConfig()
{
  maxPounceDist = 120.0f;
  minGroundAreaForPounce = 0.01f;
  pounceTimeout_s = 5;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPounceWithProx::DynamicVariables::DynamicVariables()
{
  prePouncePitch = 0.0f;
  motionObserved = false;
  pounceAtTime_s = std::numeric_limits<float>::max();
  pounceState = PounceState::WaitForMotion;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPounceWithProx::BehaviorPounceWithProx(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedMotion
  });
  _iConfig.inRangeCondition =
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
  _iConfig.inRangeCondition->Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorActivated()
{
  _iConfig.inRangeCondition->SetActive(GetBEI(), true);

  const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars = DynamicVariables();
  _dVars.pounceAtTime_s = currentTimeInSeconds + _iConfig.pounceTimeout_s;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorDeactivated()
{
  _iConfig.inRangeCondition->SetActive(GetBEI(), false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  u16 dummyDistance_mm = 0;
  const bool isSensorReadingValid = proxSensor.GetLatestDistance_mm(dummyDistance_mm);
  if(isSensorReadingValid){
    // Transition conditions
    switch(_dVars.pounceState){
      case PounceState::WaitForMotion:
      {
        if(!_iConfig.inRangeCondition->AreConditionsMet(GetBEI())){
          CancelSelf();
          return;
        }

        const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if(_dVars.motionObserved ||
           (_dVars.pounceAtTime_s < currentTimeInSeconds)){
          _dVars.pounceState = PounceState::PounceOnMotion;
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
          _dVars.prePouncePitch = robotInfo.GetPitchAngle().ToFloat();
          TransitionToPounce();
          _dVars.pounceState = PounceState::ReactToPounce;
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
                                _dVars.pounceState = PounceState::WaitForMotion;
                              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPounceWithProx::IsFingerCaught()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const float liftHeightThresh = 35.5f;
  const float bodyAngleThresh = 0.02f;
  
  float robotBodyAngleDelta = robotInfo.GetPitchAngle().ToFloat() - _dVars.prePouncePitch;
  
  // check the lift angle, after some time, transition state
  PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult", "lift: %f body: %fdeg (%frad) (%f -> %f)",
                robotInfo.GetLiftHeight(),
                RAD_TO_DEG(robotBodyAngleDelta),
                robotBodyAngleDelta,
                RAD_TO_DEG(_dVars.prePouncePitch),
                robotInfo.GetPitchAngle().getDegrees());
  return robotInfo.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::TransitionToPounce()
{  
  _dVars.motionObserved = false;
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
      const bool inGroundPlane = motionObserved.ground_area > _iConfig.minGroundAreaForPounce;
      
      const float robotOffsetX = motionObserved.ground_x;
      const float robotOffsetY = motionObserved.ground_y;
      
      // we haven't started the pounce, so update the pounce location
      if ((_dVars.pounceState == PounceState::WaitForMotion) && inGroundPlane )
      {
        float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
        if ( dist <= (_iConfig.maxPounceDist*2) )
        {
          //Set the exit state information and then cancel the hang action
          _dVars.motionObserved = true;
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

} // namespace Vector
} // namespace Anki
