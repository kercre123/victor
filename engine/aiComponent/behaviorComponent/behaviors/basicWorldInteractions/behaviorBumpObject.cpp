/**
 * File: behaviorBumpObject.cpp
 *
 * Author: ross
 * Created: 2018-07-02
 *
 * Description: a playful bump of whatever is in front of the robot
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorBumpObject.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/navMap/mapComponent.h"
#include "engine/utils/robotPointSamplerHelper.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* const kMinDistKey = "minDist_mm";
  const char* const kMaxDistKey = "maxDist_mm";
  const char* const kProbabilityEvilKey = "pEvil";
  const char* const kProbabilityBumpWhenNotEvilKey = "pBumpWhenNotEvil";
  const char* const kProbabilityBumpWhenEvilKey = "pBumpWhenEvil";
  
  const float kFirstBumpBuffer_mm = 8.0f; // dist after making contact
  const float kFirstBumpSpeed_mmps = 140.0f;
  
  const bool kExploringPushObject = true;
  
  const float kSecondBumpBuffer_mm = 60.0f; // let the treads spin a bit longer. No worries, unexpected movement is disabled
  const float kSecondBumpEvilBuffer_mm = 150.0f;
  
  const float kPauseDuration_s = 0.6f;
  
  const float kPaddingPushedObject_mm = 30.0f; // distance extending from drive center where an object would fall off a cliff
  const float kCliffWidth_mm = 100.0f;

  CONSOLE_VAR_RANGED( float, kExploringPostBumpReferenceProb, "BehaviorExploring", 1.0f, 0.0f, 1.0f);
  
  static const DrivingAnimationHandler::DrivingAnimations kSlowDrivingAnimations {
    AnimationTrigger::Count,
    AnimationTrigger::PokeObjectDriveLoop,
    AnimationTrigger::Count
  };
  static const DrivingAnimationHandler::DrivingAnimations kFastDrivingAnimations {
    AnimationTrigger::Count,
    AnimationTrigger::BumpObjectFastLoop,
    AnimationTrigger::Count
  };

}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::DynamicVariables::DynamicVariables()
{
  unexpectedMovement = false;
  state = State::Invalid;
  bumpedAgain = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::BehaviorBumpObject(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.minDist_mm = JsonTools::ParseInt32( config, kMinDistKey, GetDebugLabel() );
  _iConfig.maxDist_mm = std::numeric_limits<decltype(_iConfig.maxDist_mm)>::max();
  _iConfig.maxDist_mm = config.get( kMaxDistKey, _iConfig.maxDist_mm ).asFloat();
  
  _iConfig.pEvil = config.get( kProbabilityEvilKey, 0.0f ).asFloat();
  _iConfig.pBumpWhenNotEvil = config.get( kProbabilityBumpWhenNotEvilKey, 0.0f ).asFloat();
  _iConfig.pBumpWhenEvil = config.get( kProbabilityBumpWhenEvilKey, 0.0f ).asFloat();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.referenceHumanBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(ExploringReferenceHuman) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.referenceHumanBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBumpObject::WantsToBeActivatedBehavior() const
{
  auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  uint16_t proxDist_mm = 0;
  const bool sensorReadingValid = proxSensor.GetLatestDistance_mm( proxDist_mm );
  const bool closeEnough = (proxDist_mm <= _iConfig.maxDist_mm);
  const bool notTooClose = (proxDist_mm >= _iConfig.minDist_mm);
  return sensorReadingValid && closeEnough && notTooClose;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMinDistKey,
    kMaxDistKey,
    kProbabilityEvilKey,
    kProbabilityBumpWhenEvilKey,
    kProbabilityBumpWhenNotEvilKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  GetBEI().GetRobotInfo().GetDrivingAnimationHandler().PushDrivingAnimations( kSlowDrivingAnimations, GetDebugLabel() );
  
  DoFirstBump(); // not to be confused with "fist bump"
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::DoFirstBump()
{
  _dVars.state = State::FirstBump;
  auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  uint16_t proxDist_mm = 0;
  if( !proxSensor.GetLatestDistance_mm( proxDist_mm ) ) {
    PRINT_NAMED_WARNING( "BehaviorBumpObject.OnBehaviorActivated.InvalidProxReading",
                        "%s started but has an invalid sensor reading. Cancelling.",
                        GetDebugLabel().c_str() );
    return; // which ends the behavior
  }
  
  auto* action = new CompoundActionSequential();
  
  // animation
  action->AddAction( new TriggerLiftSafeAnimationAction{ AnimationTrigger::PokeObjectGetIn } );
  
  
  // bump
  const float distForward_mm = kFirstBumpBuffer_mm + static_cast<float>(proxDist_mm);
  const float speedForward_mmps = kFirstBumpSpeed_mmps;
  action->AddAction( new DriveStraightAction( distForward_mm, speedForward_mmps, true) );
  
  // retreat animation
  action->AddAction( new TriggerLiftSafeAnimationAction{ AnimationTrigger::PokeObjectGetOut } );
  
  DelegateIfInControl( action, [this](const ActionResult& res){
    
    GetBEI().GetRobotInfo().GetDrivingAnimationHandler().RemoveDrivingAnimations( GetDebugLabel() );
    GetBEI().GetRobotInfo().GetDrivingAnimationHandler().PushDrivingAnimations( kFastDrivingAnimations, GetDebugLabel() );
    
    DoSecondBumpIfDesired();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::DoSecondBumpIfDesired()
{
  // don't do the second bump if it probably wasnt centered well the first time, or not desired
  if( _dVars.unexpectedMovement || !kExploringPushObject ) {
    DoReferenceHumanIfDesired();
    return;
  }
  
  // check if we still see the object
  
  auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  uint16_t proxDist_mm = 0;
  const bool sensorReadingValid = proxSensor.GetLatestDistance_mm( proxDist_mm );
  const bool closeEnough = (proxDist_mm <= _iConfig.maxDist_mm);
  // don't bother checking if it's too close here, like in WantsToBeActivatedBehavior
  
  if( !(sensorReadingValid && closeEnough) ) {
    DoReferenceHumanIfDesired();
    return;
  }
  
  const float niceDistForward_mm = static_cast<float>(proxDist_mm) + kSecondBumpBuffer_mm;
  const float evilDistForward_mm = static_cast<float>(proxDist_mm) + kSecondBumpEvilBuffer_mm;
  
  // todo: inform this choice based on MoodManager::GetEmotionValue( EmotionType::Evil );
  const bool evil = (GetRNG().RandDbl() <= _iConfig.pEvil);
  
  const bool shouldBump = (evil && (GetRNG().RandDbl() <= _iConfig.pBumpWhenEvil)) ||
                          (!evil && (GetRNG().RandDbl() <= _iConfig.pBumpWhenNotEvil));
  
  if( !shouldBump ) {
    DoReferenceHumanIfDesired();
    return;
  }
  
  _dVars.bumpedAgain = (int)evil;
  
  float distForward_mm;
  
  // if a regular bump would push something off a cliff and we're not evil, dont do it at all.
  // else if a LONG bump would push something off a cliff and we ARE evil, use the longer distance.
  // otherwise don't do it at all
  if( !evil ) {
    if( WouldBumpPushSomethingOffCliff(niceDistForward_mm) ) {
      DoReferenceHumanIfDesired();
      return;
    } else {
      distForward_mm = niceDistForward_mm;
    }
  } else {
    if( !WouldBumpPushSomethingOffCliff(evilDistForward_mm) ) {
      DoReferenceHumanIfDesired();
      return;
    } else {
      distForward_mm = evilDistForward_mm;
    }
  }
  
  _dVars.state = State::SecondBump;
  
  auto* action = new CompoundActionSequential();
  
  // animation
  action->AddAction( new TriggerLiftSafeAnimationAction{ AnimationTrigger::PokeObjectGetIn } );
  
  // pause a bit
  // todo (VIC-4230) would be a good time to reference a human pose, with a mischievous expression
  action->AddAction( new WaitAction{ kPauseDuration_s } );
  
  // bump again. HARD this time! like you MEAN IT!
  float speedForward_mmps = MAX_SAFE_WHEEL_SPEED_MMPS;
  action->AddAction( new DriveStraightAction( distForward_mm, speedForward_mmps, true) );
  
  DelegateIfInControl( action, [this](const ActionResult& res){
    // retreat animation
    auto* retreat = new TriggerLiftSafeAnimationAction{ AnimationTrigger::BumpObjectFastGetOut };
    DelegateIfInControl( retreat, &BehaviorBumpObject::DoReferenceHumanIfDesired);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::DoReferenceHumanIfDesired()
{
  if( kExploringPostBumpReferenceProb <= 0.0f ) {
    // no chance to bump, end behavior now
    return;
  }
  
  const bool refBehaviorWantsToBeActivated = _iConfig.referenceHumanBehavior->WantsToBeActivated();
  const bool shouldActivate = ( GetRNG().RandDbl() < kExploringPostBumpReferenceProb );
  if( refBehaviorWantsToBeActivated && shouldActivate ) {
    _dVars.state = State::ReferenceHumanAfterBump;
    DelegateIfInControl( _iConfig.referenceHumanBehavior.get() ); // end this behavior once the delegate finishes
  }
  // else, the behavior ends now
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::BehaviorUpdate()
{
  if( IsActivated() ) {
    // todo: we might consider canceling if this is true often
    _dVars.unexpectedMovement |= GetBEI().GetMovementComponent().IsUnexpectedMovementDetected();
    
    if( _dVars.state == State::SecondBump ) {
      auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
      uint16_t proxDist_mm = 0;
      if( proxSensor.GetLatestDistance_mm( proxDist_mm ) && (proxDist_mm > 150) ) {
        // robot lost sight of whatever it's pushing
        CancelDelegates(true); // run callbacks to play the retreat anim / reference behavior if desired
      }
    }
    
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::OnBehaviorDeactivated()
{
  GetBEI().GetRobotInfo().GetDrivingAnimationHandler().RemoveDrivingAnimations( GetDebugLabel() );
  
  DASMSG(behavior_exploring_poked_object, "behavior.exploring.poke", "The robot intentionally poked something");
  DASMSG_SET(i1, 1 + (int)(_dVars.bumpedAgain >= 0), "Number of pokes");
  DASMSG_SET(i2, _dVars.bumpedAgain, "If there was a second poke, whether it was evil");
  DASMSG_SEND();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBumpObject::WouldBumpPushSomethingOffCliff( float driveDist_mm ) const
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto  memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  RobotPointSamplerHelper::RejectIfWouldCrossCliff cliffDetector{ kCliffWidth_mm };
  cliffDetector.SetRobotPosition( Vec2f{robotPose.GetTranslation()} );
  cliffDetector.UpdateCliffs( memoryMap );
  
  Vec3f offset{ kPaddingPushedObject_mm + driveDist_mm, 0.0f, 0.0f};
  Rotation3d rot = Rotation3d{0.f, Z_AXIS_3D()};
  const Point2f pt = (robotPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
  
  const bool hitsCliff = !cliffDetector( pt );
  return hitsCliff;
}

}
}
