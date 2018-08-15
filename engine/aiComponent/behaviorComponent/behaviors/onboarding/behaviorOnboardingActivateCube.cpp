/**
 * File: behaviorOnboardingActivateCube.cpp
 *
 * Author: ross
 * Created: 2018-06-30
 *
 * Description: faces and possibly approaches cube, then plays an animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingActivateCube.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/animationComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/navMap/iNavMap.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "util/math/math.h"


namespace Anki {
namespace Vector {
  
namespace {
  constexpr float kNominalCubeDist_mm = 150.0f;
  constexpr float kTooFarDist_mm = 300.0f;
  constexpr float kTooCloseDist_mm = 100.0f;
  
  const float kVerifyActionTimeout_s = 3.0f;
  
  const std::string kCubeActivationAnimation = "anim_cube_psychic_01";
  
  static_assert( kTooFarDist_mm > kNominalCubeDist_mm, "");
  static_assert( kTooCloseDist_mm < kNominalCubeDist_mm, "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingActivateCube::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingActivateCube::DynamicVariables::DynamicVariables()
{
  state = State::NotStarted;
  activationStartTime_s = 0.0f;
  cubeActivationTime_s = 0.0f;
  requireConfirmation = true;
  approached = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingActivateCube::BehaviorOnboardingActivateCube(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingActivateCube::WantsToBeActivatedBehavior() const
{
  // this behavior is considered done if it is interrupted after most of the animation sequence has ended
  return !WasSuccessful() && _dVars.targetID.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::OnBehaviorActivated() 
{
  const bool firstActivation = (_dVars.state == State::NotStarted);
  const auto targetID = _dVars.targetID;
  const auto requireConfirmation = _dVars.requireConfirmation;
  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.targetID = targetID;
  _dVars.requireConfirmation = requireConfirmation;
  
  // after any sort of interruption, we need to make sure we're facing the cube. turn and verify it
  _dVars.state = State::Facing;
  auto* turnAction = new TurnTowardsObjectAction( _dVars.targetID, Radians{M_PI_F}, _dVars.requireConfirmation );
  DelegateIfInControl(turnAction, [this,firstActivation](const ActionResult& res){
    if( firstActivation ) {
      // never started before. react to seeing the cube
      TransitionToReactToCube();
    } else {
      // set spacing if needed, then activate
      float dist_mm = 0.0f;
      if( ShouldApproachCube( dist_mm ) || ShouldBackUpFromCube( dist_mm ) ) {
        TransitionToSetIdealSpacing( dist_mm );
      } else {
        TransitionToActivateCube();
      }
    }
  });
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::OnBehaviorDeactivated()
{
  _dVars.targetID.UnSet();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  const bool verifyDidntEnd = _dVars.requireConfirmation && !_dVars.verifyAction.expired();
  
  if( _dVars.state == State::Activating ) {
    const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool verifyTimeout = (currTime - _dVars.activationStartTime_s > kVerifyActionTimeout_s);
    if( verifyDidntEnd && verifyTimeout ) {
      // we're trying to activate a cube but don't see it. Cancel, which should cause the parent behavior
      // to look for a cube again and re-run this behavior
      CancelSelf();
    }
    if( currTime > _dVars.cubeActivationTime_s ) {
      // let there be light!
      GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger( _dVars.targetID,
                                                               CubeAnimationTrigger::Onboarding );
      _dVars.state = State::WaitingForAnimToEnd;
    }
  }
  
  if( (_dVars.state == State::Activating) || (_dVars.state == State::WaitingForAnimToEnd) ) {
    const bool activationEnded = _dVars.activationAction.expired();
    if( verifyDidntEnd && activationEnded ) {
      PRINT_NAMED_WARNING( "BehaviorOnboardingActivateCube.BehaviorUpdate.AnimTooQuick",
                           "The activation animation ended so quickly that we didn't have time to verify the cube" );
      CancelDelegates(false);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingActivateCube::ShouldApproachCube( float& howMuchToDrive_mm ) const
{
  float dist = GetDistToCube();
  if( dist > kTooFarDist_mm ) {
    howMuchToDrive_mm = dist - kNominalCubeDist_mm;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingActivateCube::ShouldBackUpFromCube( float& howMuchToDrive_mm ) const
{
  float dist = GetDistToCube();
  if( dist < kTooCloseDist_mm ) {
    const auto memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
    if( memoryMap != nullptr ) {
      const float distBackwards = dist - kNominalCubeDist_mm;
      // line segment extending |distBackwards| behind robot
      Vec3f offset(distBackwards, 0.0f, 0.0f);
      Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
      const Pose3d& robotPose = GetBEI().GetRobotInfo().GetPose();
      const Point2f p2 = (robotPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
      // kTypesThatAreObstacles includes cubes, but we already know a cube is in front of us, and onboarding
      // is not likely to have two cubes
      const bool hasCollision = memoryMap->HasCollisionWithTypes( {{Point2f{robotPose.GetTranslation()}, p2}},
                                                                  MemoryMapTypes::kTypesThatAreObstacles );
      if( !hasCollision ) {
        howMuchToDrive_mm = distBackwards;
        return true;
      }
    }
  }
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorOnboardingActivateCube::GetDistToCube() const
{
  float distToCube_mm = kNominalCubeDist_mm;
  const Pose3d& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* object = GetBEI().GetBlockWorld().GetLocatedObjectByID( _dVars.targetID );
  if( object != nullptr ) {
    float dist = 0.0f;
    if( ComputeDistanceBetween( object->GetPose(), robotPose, dist) ) {
      distToCube_mm = dist;
    }
  }
  return distToCube_mm;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::TransitionToReactToCube()
{
  _dVars.state = State::Reacting;
  auto* huhAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingCubeHuh };
  DelegateIfInControl( huhAction, [this](const ActionResult& res) {
    float dist_mm = 0.0f;
    if( ShouldApproachCube( dist_mm ) || ShouldBackUpFromCube( dist_mm ) ) {
      TransitionToSetIdealSpacing( dist_mm );
    } else {
      TransitionToActivateCube();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::TransitionToSetIdealSpacing( float howMuchToDrive_mm )
{
  _dVars.approached = true;
  _dVars.state = State::SettingSpacing;
  
  auto* action = new CompoundActionSequential();
  // this doesn't involve the planner, since the pickup action will already need to do that, and we
  // don't want to spend too much time driving back and forth around the cube.
  action->AddAction( new DriveStraightAction{ howMuchToDrive_mm } );
  // turn if not already facing it, but don't verify it here
  action->AddAction( new TurnTowardsObjectAction{ _dVars.targetID } );
  
  DelegateIfInControl( action, [this](const ActionResult& res) {
    TransitionToActivateCube();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingActivateCube::TransitionToActivateCube()
{
  _dVars.state = State::Activating;
  _dVars.activationStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  auto* action = new CompoundActionParallel();
  
  if( _dVars.requireConfirmation && _dVars.approached ) {
    auto* verifyAction = new VisuallyVerifyObjectAction( _dVars.targetID );
    _dVars.verifyAction = action->AddAction( verifyAction );
  } else {
    _dVars.verifyAction.reset();
  }
  
  auto* animAction = new PlayAnimationAction{ kCubeActivationAnimation };
  _dVars.activationAction = action->AddAction( animAction );
  
  const float cubeEventTime_s = GetCubeEventTime();
  // todo: we'll need some arbitrary latency correction factor here, but until the animation is
  // extended, it's hard to tell when the keyframe is
  _dVars.cubeActivationTime_s = _dVars.activationStartTime_s + cubeEventTime_s;
  
  DelegateIfInControl( action ); // behavior ends when this is done
}
  
float BehaviorOnboardingActivateCube::GetCubeEventTime() const
{
  const auto& dataAccessorComp
    = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if( animContainer != nullptr ) {
    const Animation* animPtr = animContainer->GetAnimation( kCubeActivationAnimation );
    if( animPtr != nullptr ) {
      const auto& track = animPtr->GetTrack<EventKeyFrame>();
      const auto* keyframe = track.GetFirstKeyFrame();
      if( keyframe != nullptr ) {
        return Util::MilliSecToSec( static_cast<float>(keyframe->GetTriggerTime_ms()) );
      } else {
        PRINT_NAMED_WARNING( "BehaviorOnboardingActivateCube.GetCubeEventTime.NoKeyframe",
                             "No cube keyframe found in animation" );
      }
    } else {
      PRINT_NAMED_ERROR( "BehaviorOnboardingActivateCube.GetCubeEventTime.InvalidAnim",
                         "Animations need to be manually loaded on engine side - %s is not",
                         kCubeActivationAnimation.c_str() );
    }
  }
  return 0.0f;
}
  

}
}
