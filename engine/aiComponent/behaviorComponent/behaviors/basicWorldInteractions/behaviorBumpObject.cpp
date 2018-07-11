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

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/navMap/mapComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/utils/robotPointSamplerHelper.h"


namespace Anki {
namespace Cozmo {
  
namespace {
  const char* const kMinDistKey = "minDist_mm";
  const char* const kMaxDistKey = "maxDist_mm";
  const char* const kProbabilityEvilKey = "pEvil";
  
  const float kFirstBumpBuffer_mm = 10.0f; // dist after making contact
  const float kFirstBumpSpeed_mmps = 80.0f;
  
  const float kFirstRetreatDist_mm = 10.0f;
  const float kRetreatSpeed_mmps = 30.0f;
  
  const float kSecondBumpBuffer_mm = 60.0f; // let the treads spin a bit longer. No worries, unexpected movement is disabled
  const float kSecondBumpEvilBuffer_mm = 150.0f;
  const float kSecondRetreatDist_mm = 20.0f;
  
  const float kPauseDuration_s = 0.6f;
  
  const float kPaddingPushedObject_mm = 30.0f; // distance extending from drive center where an object would fall off a cliff
  const float kCliffWidth_mm = 100.0f;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::DynamicVariables::DynamicVariables()
{
  unexpectedMovement = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBumpObject::BehaviorBumpObject(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.minDist_mm = JsonTools::ParseInt32( config, kMinDistKey, GetDebugLabel() );
  _iConfig.maxDist_mm = std::numeric_limits<decltype(_iConfig.maxDist_mm)>::max();
  _iConfig.maxDist_mm = config.get( kMaxDistKey, _iConfig.maxDist_mm ).asFloat();
  
  _iConfig.pEvil = config.get( kProbabilityEvilKey, 0.0f ).asFloat();
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
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  DoFirstBump(); // not to be confused with "fist bump"
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::DoFirstBump()
{
  auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  uint16_t proxDist_mm = 0;
  if( !proxSensor.GetLatestDistance_mm( proxDist_mm ) ) {
    PRINT_NAMED_WARNING( "BehaviorBumpObject.OnBehaviorActivated.InvalidProxReading",
                        "%s started but has an invalid sensor reading. Cancelling.",
                        GetDebugLabel().c_str() );
    return; // which ends the behavior
  }
  
  // todo: push driving animations
  auto* action = new CompoundActionSequential();
  
  // bump
  const float distForward_mm = kFirstBumpBuffer_mm + static_cast<float>(proxDist_mm);
  const float speedForward_mmps = kFirstBumpSpeed_mmps;
  action->AddAction( new DriveStraightAction( distForward_mm, speedForward_mmps, false) );
  
  // retreat
  const float distBackward_mm = -kFirstRetreatDist_mm;
  const float speedBackward_mmps = kRetreatSpeed_mmps;
  action->AddAction( new DriveStraightAction( distBackward_mm, speedBackward_mmps, false) );
  
  DelegateIfInControl( action, [this](const ActionResult& res){
    // todo: pop driving animations
    DoSecondBumpIfDesired();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::DoSecondBumpIfDesired()
{
  // don't do the second bump if it probably wasnt centered well the first time
  if( _dVars.unexpectedMovement ) {
    return; // which ends the behavior
  }
  
  // check if we still see the object
  
  auto& proxSensor = GetBEI().GetComponentWrapper( BEIComponentID::ProxSensor ).GetComponent<ProxSensorComponent>();
  uint16_t proxDist_mm = 0;
  const bool sensorReadingValid = proxSensor.GetLatestDistance_mm( proxDist_mm );
  const bool closeEnough = (proxDist_mm <= _iConfig.maxDist_mm);
  // don't bother checking if it's too close here, like in WantsToBeActivatedBehavior
  
  if( !(sensorReadingValid && closeEnough) ) {
    return; // which ends the behavior
  }
  
  const float niceDistForward_mm = static_cast<float>(proxDist_mm) + kSecondBumpBuffer_mm;
  const float evilDistForward_mm = static_cast<float>(proxDist_mm) + kSecondBumpEvilBuffer_mm;
  
  // todo: inform this choice based on MoodManager::GetEmotionValue( EmotionType::Evil );
  const bool evil = (GetRNG().RandDbl() <= _iConfig.pEvil);
  
  // if a regular bump would push something off a cliff and we're not evil, dont do it at all.
  // else if a LONG bump would push something off a cliff and we ARE evil, use the longer distance.
  // otherwise use the regular distance
  if( !evil && WouldBumpPushSomethingOffCliff(niceDistForward_mm) ) {
    return;
  }
  float distForward_mm;
  if( evil && WouldBumpPushSomethingOffCliff(evilDistForward_mm) ) {
    distForward_mm = evilDistForward_mm;
  } else {
    distForward_mm = niceDistForward_mm;
  }
  
  // todo: push driving animations
  
  auto* action = new CompoundActionSequential();
  
  // pause a bit
  // todo (VIC-4230) would be a good time to reference a human pose, with a mischievous expression
  action->AddAction( new WaitAction{ kPauseDuration_s } );
  
  // bump again. HARD this time! like you MEAN IT!
  float speedForward_mmps = MAX_SAFE_WHEEL_SPEED_MMPS;
  action->AddAction( new DriveStraightAction( distForward_mm, speedForward_mmps, false) );
  
  // retreat again
  const float distBackward_mm = -kSecondRetreatDist_mm;
  float speedBackward_mmps = kRetreatSpeed_mmps;
  action->AddAction( new DriveStraightAction( distBackward_mm, speedBackward_mmps, false) );
  
  // no animations yet, but this would be a good spot for a "proud" one
  
  DelegateIfInControl( action, [](const ActionResult& res){
    // todo: pop driving animations
    
    // and then the behavior ends
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBumpObject::BehaviorUpdate()
{
  if( IsActivated() ) {
    _dVars.unexpectedMovement |= GetBEI().GetMovementComponent().IsUnexpectedMovementDetected();
    // todo: we might consider canceling if this is true often
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBumpObject::WouldBumpPushSomethingOffCliff( float driveDist_mm ) const
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
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
