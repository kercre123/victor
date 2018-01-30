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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
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
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_backupBehavior.get());
  delegates.insert(_approachBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::InitBehavior()
{
  _backupBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(BEHAVIOR_ID(PounceBackupWithProx));
  _approachBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(BEHAVIOR_ID(PounceApproachWithProx));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorActivated()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::OnBehaviorDeactivated()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetValue<ProxSensorComponent>();
  if(proxSensor.IsSensorReadingValid()){
    // Transition conditions
    switch(_pounceState){
      case PounceState::BackupToIdealDistance:
      {
        if(!IsControlDelegated()){
          if(_backupBehavior->WantsToBeActivated()){
            DelegateIfInControl(_backupBehavior.get());
          }else{
            _pounceState = PounceState::ApproachObject;
          }
        }
        break;
      }
      case PounceState::ApproachObject:
      {
        if(!IsControlDelegated()){
          if(_approachBehavior->WantsToBeActivated()){
            DelegateIfInControl(_approachBehavior.get());
          }else{
            _motionObserved = false;
            _pounceState = PounceState::WaitForMotion;
          }
        }
        break;
      }
      case PounceState::WaitForMotion:
      {
        if(_motionObserved){
          _pounceState = PounceState::PounceOnMotion;
        }
        break;
      }
      case PounceState::PounceOnMotion:
      {
        // Pounce on the object
        if(!IsControlDelegated()){
          const auto& robotInfo = GetBEI().GetRobotInfo();
          _prePouncePitch = robotInfo.GetPitchAngle().ToFloat();
          TransitionToPounce();
          _pounceState = PounceState::ReactToPounce;
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
    auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetValue<ProxSensorComponent>();
    bool liftBlocking = false;
    proxSensor.IsLiftInFOV(liftBlocking);
    if(!IsControlDelegated() &&
      liftBlocking){
        DelegateIfInControl(new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK));
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
                                _pounceState = PounceState::BackupToIdealDistance;
                              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPounceWithProx::IsFingerCaught()
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const float liftHeightThresh = 35.5f;
  const float bodyAngleThresh = 0.02f;
  
  float robotBodyAngleDelta = robotInfo.GetPitchAngle().ToFloat() - _prePouncePitch;
  
  // check the lift angle, after some time, transition state
  PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult", "lift: %f body: %fdeg (%frad) (%f -> %f)",
                robotInfo.GetLiftHeight(),
                RAD_TO_DEG(robotBodyAngleDelta),
                robotBodyAngleDelta,
                RAD_TO_DEG(_prePouncePitch),
                robotInfo.GetPitchAngle().getDegrees());
  return robotInfo.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPounceWithProx::TransitionToPounce()
{  
  IActionRunner* animation = new PlayAnimationAction("anim_pounce_04");
  IActionRunner* action = nullptr;
  if(_motionObserved){
    _motionObserved = false;
    const Point2f motionCentroid(_observedX, _observedY);
    Radians relPanAngle;
    Radians relTiltAngle;
    
    GetBEI().GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, relPanAngle, relTiltAngle);
    TurnInPlaceAction* turnAction = new TurnInPlaceAction(relPanAngle.ToFloat(), false);
    turnAction->SetMaxSpeed(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC);
    turnAction->SetAccel(1000.f);
    action = new CompoundActionSequential({
      turnAction,
      animation
    });
  }else{
    action = animation;
  }

  DelegateIfInControl(action, &BehaviorPounceWithProx::TransitionToResultAnim);
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
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      
      const float robotOffsetX = motionObserved.ground_x;
      const float robotOffsetY = motionObserved.ground_y;
      
      // we haven't started the pounce, so update the pounce location
      if ((_pounceState == PounceState::WaitForMotion) && inGroundPlane )
      {
        float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
        if ( dist <= _maxPounceDist )
        {
          //Set the exit state information and then cancel the hang action
          _observedX = motionObserved.img_x;
          _observedY = motionObserved.img_y;
          _motionObserved = true;
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
