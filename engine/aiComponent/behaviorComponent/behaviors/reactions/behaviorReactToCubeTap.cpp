/**
 * File: behaviorReactToCubeTap.h
 *
 * Author: Jarboo
 * Created: 3/15/2018
 *
 * Description: Reaction behavior when Victor detects a cube being tapped
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// #include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToCubeTap.h"
#include "behaviorReactToCubeTap.h"

#include "engine/actions/animActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/blockWorld/blockWorld.h"

#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"


namespace {
  const char* kChargerBehaviorKey                            = "onChargerBehavior";
  const char* kCubeInteractionDurationKey                    = "interactionDuration_s";

  // don't bother responding after this amount of time
  const float kCubeTapMaxResponseTime                        = 1.0f;
  const int kDriveToCubeAttempts                             = 2;
}

namespace Anki {
namespace Cozmo {

constexpr BehaviorReactToCubeTap::CubeIntensityAnimations BehaviorReactToCubeTap::kReactToCubeAnimations =
{
  { BehaviorReactToCubeTap::EIntensityLevel::Level1,  AnimationTrigger::ReactToCubeTapCubeTappedLvl1 },
  { BehaviorReactToCubeTap::EIntensityLevel::Level2,  AnimationTrigger::ReactToCubeTapCubeTappedLvl2 },
  { BehaviorReactToCubeTap::EIntensityLevel::Level3,  AnimationTrigger::ReactToCubeTapCubeTappedLvl3 },
};

constexpr BehaviorReactToCubeTap::CubeIntensityAnimations BehaviorReactToCubeTap::kSearchForCubeAnimations =
{
  { BehaviorReactToCubeTap::EIntensityLevel::Level1,  AnimationTrigger::ReactToCubeSearchForCubeLvl1 },
  { BehaviorReactToCubeTap::EIntensityLevel::Level2,  AnimationTrigger::ReactToCubeSearchForCubeLvl2 },
  { BehaviorReactToCubeTap::EIntensityLevel::Level3,  AnimationTrigger::ReactToCubeSearchForCubeLvl3 },
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCubeTap::InstanceConfig::InstanceConfig() :
  cubeInteractionDuration( 0.0f ),
  targetCube( { ObjectID(), 0.0f } )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCubeTap::DynamicVariables::DynamicVariables() :
  state( EState::Invalid ),
  intensity( EIntensityLevel::FirstLevel )
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToCubeTap::BehaviorReactToCubeTap( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  JsonTools::GetValueOptional( config, kCubeInteractionDurationKey, _iVars.cubeInteractionDuration );
  JsonTools::GetValueOptional( config, kChargerBehaviorKey, _iVars.chargerBehaviorString );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::InitBehavior()
{
  // grab our charger behavior ...
  if ( !_iVars.chargerBehaviorString.empty() )
  {
    _iVars.chargerBehavior = FindBehavior( _iVars.chargerBehaviorString );
  }

  SubscribeToTags(
  {
    ExternalInterface::MessageEngineToGame::Tag::ObjectTapped
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  if ( _iVars.chargerBehavior )
  {
    delegates.insert( _iVars.chargerBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = false;
  modifiers.wantsToBeActivatedWhenOnCharger       = ( nullptr != _iVars.chargerBehavior );
  modifiers.wantsToBeActivatedWhenOffTreads       = false;
  modifiers.behaviorAlwaysDelegates               = true;

  // allow us to find the cubes as best as we can
  modifiers.visionModesForActivatableScope->insert( { VisionMode::DetectingMarkers, EVisionUpdateFrequency::High } );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  static const char* list[] =
  {
    kChargerBehaviorKey,
    kCubeInteractionDurationKey,
  };

  expectedKeys.insert( std::begin( list ), std::end( list ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCubeTap::WantsToBeActivatedBehavior() const
{
  return _iVars.targetCube.id.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::AlwaysHandleInScope( const EngineToGameEvent& event )
{
  if ( event.GetData().GetTag() == ExternalInterface::MessageEngineToGame::Tag::ObjectTapped )
  {
    // record the cube tap
    // if we're not currently active, this will activate us

     const ExternalInterface::ObjectTapped& payload = event.GetData().Get_ObjectTapped();

    _iVars.targetCube.id              = payload.objectID;
    _iVars.targetCube.lastTappedTime  = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    PRINT_CH_DEBUG( "Behaviors", "BehaviorReactToCubeTap", "Received Cube Tapped event [%u]", payload.objectID );

    if ( IsActivated() )
    {
      // if we're currently active, we want to react to the tap depending on our current state.
      OnCubeTappedWhileActive();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  ReactToCubeTapped();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::OnBehaviorDeactivated()
{
  _iVars.targetCube.id.UnSet();
  _iVars.targetCube.lastTappedTime = 0.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::BehaviorUpdate()
{
  if ( IsActivated() )
  {
    // if we're looking for a cube, we want to bail as soon as we have the cube
    if ( ( _dVars.state == EState::FindCube ) && IsCubeLocated() )
    {
      // stop searching for a cube
      CancelDelegates( false );

      // now go get that cube!
      TransitionToGoToCube( true );
    }
  }
  else
  {
    // reset the target cube each x seconds we're not active
    // this is so that if we cannot respond to the cube tap immediately (eg. Victor is tied up doing something else)
    // we don't want to react to said cube down the line
    if ( _iVars.targetCube.id.IsSet() )
    {
      const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if ( currentTime >= ( _iVars.targetCube.lastTappedTime + kCubeTapMaxResponseTime ) )
      {
        _iVars.targetCube.id.UnSet();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::ReactToCubeTapped()
{
  // begin playing our response to cube tap anim, then transition into figuring out if we need to drive off the charger or not

  const AnimationTrigger animation = GetCubeReactionAnimation();
  ASSERT_NAMED_EVENT( AnimationTrigger::Count != animation, "BehaviorReactToCubeTap.OnBehaviorActivated",
                     "No react to cube animation was found" );

  DelegateIfInControl( new TriggerAnimationAction( animation ), &BehaviorReactToCubeTap::TransitionToGetOffCharger );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::OnCubeTappedWhileActive()
{
  DEV_ASSERT_MSG( IsActivated(), "BehaviorReactToCubeTap.State", "Calling OnCubeTappedWhileActive() while not Activated" );
  if ( EState::FindCube == _dVars.state )
  {
    if ( _dVars.intensity < EIntensityLevel::LastLevel )
    {
      // note: for now we don't care if it's the same cube or a different one; ONE CUBE TO RULE THEM ALL

      // stop searching for the cube
      CancelDelegates( false );

      // start searching again, but with more "spaz"
      _dVars.intensity = static_cast<EIntensityLevel>( _dVars.intensity + 1 );

      PRINT_CH_DEBUG( "Behaviors", "BehaviorReactToCubeTap", "Reacting to multiple cube taps ... spaz level [%d]!", _dVars.intensity );
      ReactToCubeTapped();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::TransitionToGetOffCharger()
{
  PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.State", "Transitioning to GetOffCharger state" );
  _dVars.state = EState::GetOffCharger;

  // if we have a 'get off charger' behavior, attempt to delegate
  // if we're on the charger, BehaviorDriveOffCharger::WantsToBeActivated() will return true
  if ( ( nullptr != _iVars.chargerBehavior ) && _iVars.chargerBehavior->WantsToBeActivated() )
  {
    DelegateIfInControl( _iVars.chargerBehavior.get(), &BehaviorReactToCubeTap::TransitionToFindCube );
  }
  else
  {
    TransitionToFindCube();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::TransitionToFindCube()
{
  PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.State", "Transitioning to FindCube state" );
  _dVars.state = EState::FindCube;

  if ( !IsCubeLocated() )
  {
    // if we find a cube before the animation is complete, then we will cancel the delegate and proceed to going to
    // said cube.  If we make it to the end of the animation, we assume no cube was found and bail.

    const AnimationTrigger animation = GetSearchForCubeAnimation();
    ASSERT_NAMED_EVENT( AnimationTrigger::Count != animation, "BehaviorReactToCubeTap.TransitionToFindCube",
                       "No search for cube animation was found" );

    DelegateIfInControl( new TriggerAnimationAction( animation ), &BehaviorReactToCubeTap::OnCubeNotFound );
  }
  else
  {
    TransitionToGoToCube( false );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToCubeTap::IsCubeLocated() const
{
  const ObservableObject* locatedCube = GetBEI().GetBlockWorld().GetLocatedObjectByID( _iVars.targetCube.id, ObjectFamily::LightCube );
  return ( nullptr != locatedCube );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::OnCubeNotFound()
{
  // no callback so that we'll exit the behavior when we're done delegation
  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::ReactToCubeTapCubeNotFound ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::TransitionToGoToCube( bool playFoundCubeAnimation )
{
  PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.State", "Transitioning to GoToCube state" );
  _dVars.state = EState::GoToCube;

  if ( playFoundCubeAnimation )
  {
    // can't supply DriveToTargetCube() directly as the callback since it has arguments
    auto driveToCube = [this]() { DriveToTargetCube(); };

    // play our "holy shit we found a cube" animation, then go to that cube
    DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::ReactToCubeTapCubeFound ), driveToCube );
  }
  else
  {
    DriveToTargetCube();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::DriveToTargetCube( unsigned int attempt )
{
  // note: find face prior to this and line up to face

  auto onCompleteCallback = [this, attempt]( ActionResult result ) mutable
  {
    // note: what to do about ActionResult::VISUAL_OBSERVATION_FAILED?
    switch ( result )
    {
      case ActionResult::SUCCESS:
        TransitionToInteractWithCube();
        break;

      case ActionResult::RETRY:
        // attempt to drive to the cube at most kDriveToCubeAttempts times, then give up
        if ( attempt < kDriveToCubeAttempts )
        {
          PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.DriveToTargetCube", "DriveToTargetCube failed, retrying attempt #%u", attempt+1 );
          DriveToTargetCube( attempt + 1 );
        }
        else
        {
          PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.DriveToTargetCube", "DriveToTargetCube failed, too many attempts, exiting behavior" );
          OnCubeNotFound();
        }
        break;

      default:
        // if we didn't succeed, bail
        PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.DriveToTargetCube", "DriveToTargetCube failed, exiting behavior" );
        OnCubeNotFound();
        break;
    }
  };

  DelegateIfInControl( new DriveToObjectAction( _iVars.targetCube.id, PreActionPose::ActionType::DOCKING, 20.0f ),
                       onCompleteCallback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToCubeTap::TransitionToInteractWithCube()
{
  PRINT_CH_INFO( "Behaviors", "BehaviorReactToCubeTap.State", "Transitioning to InteractWithCube state" );
  _dVars.state = EState::InteractWithCube;

  if ( _iVars.cubeInteractionDuration > 0.0f )
  {
    CompoundActionSequential* cubeInterAction = new CompoundActionSequential();
    cubeInterAction->AddAction( new TriggerAnimationAction( AnimationTrigger::ReactToCubeTapInteractionLoop,
                                                            0, true, (u8)AnimTrackFlag::NO_TRACKS,
                                                            _iVars.cubeInteractionDuration ), true );

    cubeInterAction->AddAction( new TriggerAnimationAction( AnimationTrigger::ReactToCubeTapInteractionGetOut ) );

    DelegateIfInControl( cubeInterAction );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToCubeTap::GetCubeReactionAnimation() const
{
  static_assert( IsSequentialArray( kReactToCubeAnimations ), "The array does not define each entry once and only once!" );
  DEV_ASSERT_MSG( ( _dVars.intensity >= 0 ) && ( _dVars.intensity < EIntensityLevel::Count ),
                  "BehaviorReactToCubeTap", "Invalid Intensity level" );

  return kReactToCubeAnimations[_dVars.intensity].Value();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToCubeTap::GetSearchForCubeAnimation() const
{
  static_assert( IsSequentialArray( kSearchForCubeAnimations ), "The array does not define each entry once and only once!" );
  DEV_ASSERT_MSG( ( _dVars.intensity >= 0 ) && ( _dVars.intensity < EIntensityLevel::Count ),
                  "BehaviorReactToCubeTap", "Invalid Intensity level" );

  return kSearchForCubeAnimations[_dVars.intensity].Value();
}

} // namespace Cozmo
} // namespace Anki
