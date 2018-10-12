/***********************************************************************************************************************
 *
 *  BehaviorReactToSound
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/


// #include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToSound.h"
#include "behaviorReactToSound.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMicDirection.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/components/movementComponent.h"
#include "engine/engineTimeStamp.h"
#include "clad/types/animationTrigger.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"

#include <algorithm>


#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", format, ##__VA_ARGS__ )


namespace Anki {
namespace Vector {


CONSOLE_VAR( float,                        kRTS_MaxReactionTime_s,                  "SoundReaction", 1.00f ); // we have this much time to respond to a sound
CONSOLE_VAR( float,                        kRTS_ReReactionCooldown_s,               "SoundReaction", 0.25f ); // we have this much time to respond to a sound


namespace
{
  const char* const kMicDirectionReactionBehavior       = "micDirectionReactionBehavior";
  const char* const kAbsolutePowerThresholdKey          = "micAbsolutePowerThreshold";
  const char* const kMinPowerThresholdKey               = "micMinPowerThreshold";
  const char* const kConfidenceThresholdAtMinPowerKey   = "micConfidenceThresholdAtMinPower";
  
  // Time (ms) threshold for considering the lift to have recently been moving.
  // Used for filtering out slamming noises from the lift when Vector lowers it to the ground.
  const u32 RECENT_LIFT_MOTION_TIME_LIMIT_MS = 500;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::InstanceConfig::InstanceConfig() :
  reactorId( kInvalidSoundReactorId )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::BehaviorReactToSound( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  const char* debugKey = "BehaviorReactToSound.Params.ObservationStatus";
  
  _iVars.absolutePowerThreshold = JsonTools::ParseFloat( config, kAbsolutePowerThresholdKey, debugKey);
  _iVars.minPowerThreshold = JsonTools::ParseFloat( config, kMinPowerThresholdKey, debugKey);
  _iVars.confidenceThresholdAtMinPower = JsonTools::ParseUInt32( config, kConfidenceThresholdAtMinPowerKey, debugKey);

  // get the reaction behavior
  _iVars.reactionBehaviorString = JsonTools::ParseString( config, kMicDirectionReactionBehavior, "BehaviorReactToSound" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::InitBehavior()
{
  BehaviorID reactionId;
  if ( BehaviorTypesWrapper::BehaviorIDFromString( _iVars.reactionBehaviorString, reactionId ) )
  {
    GetBEI().GetBehaviorContainer().FindBehaviorByIDAndDowncast( reactionId,
                                                                BEHAVIOR_CLASS(ReactToMicDirection),
                                                                _iVars.reactionBehavior );
  }
  else
  {
    DEV_ASSERT_MSG( false, "BehaviorReactToSound.InitBehavior",
                   "Invalid behavior id string (%s)", _iVars.reactionBehaviorString.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.behaviorAlwaysDelegates               = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMicDirectionReactionBehavior,
    kAbsolutePowerThresholdKey,
    kMinPowerThresholdKey,
    kConfidenceThresholdAtMinPowerKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{
  delegates.insert( _iVars.reactionBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::WantsToBeActivatedBehavior() const
{
  // don't activate if any intent is pending since that will likely cause an unclaimed intent reaction afterwards
  const auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool intentPending = uic.IsAnyUserIntentPending();
  
  return (!intentPending) && _iVars.reactionBehavior->WantsToBeActivatedBehavior();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnBehaviorActivated()
{
  RespondToSound();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnBehaviorDeactivated()
{
  OnResponseComplete();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnBehaviorEnteredActivatableScope()
{
  // register for mic power updates
  MicDirectionHistory& micHistory = GetBEI().GetMicComponent().GetMicDirectionHistory();
  _iVars.reactorId = micHistory.RegisterSoundReactor( std::bind( &BehaviorReactToSound::OnMicPowerSampleRecorded,
                                                                 this,
                                                                 std::placeholders::_1,
                                                                 std::placeholders::_2,
                                                                 std::placeholders::_3 ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnBehaviorLeftActivatableScope()
{
  if ( kInvalidSoundReactorId != _iVars.reactorId )
  {
    MicDirectionHistory& micHistory = GetBEI().GetMicComponent().GetMicDirectionHistory();
    micHistory.UnRegisterSoundReactor( _iVars.reactorId );

    _iVars.reactorId = kInvalidSoundReactorId;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::BehaviorUpdate()
{
  // if we're active, it means we're responding to sound
  // if we're not active, it means we're listing for sound in order to activate us
  // let's clear out any triggers we haven't responded to in the specified amount of time
  if ( !IsActivated() )
  {
    if ( kInvalidMicDirectionIndex != _triggeredDirection )
    {
      const EngineTimeStamp_t currentTime = GetCurrentTimeMS();
      const TimeStamp_t reactionTime = static_cast<TimeStamp_t>( kRTS_MaxReactionTime_s * 1000.f );
      if ( currentTime >= ( _triggerDetectedTime + reactionTime ) )
      {
        PRINT_DEBUG( "Did no respond to sound direction in time, clearing triggered direction" );
        ClearTriggerDirection();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::SetTriggerDirection( MicDirectionIndex direction )
{
  if ( kInvalidMicDirectionIndex != direction )
  {
    _triggeredDirection = direction;
    _triggerDetectedTime = GetCurrentTimeMS();

    _iVars.reactionBehavior->SetReactDirection( _triggeredDirection );
  }
  else
  {
    ClearTriggerDirection();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::ClearTriggerDirection()
{
  _triggeredDirection = kInvalidMicDirectionIndex;
  _iVars.reactionBehavior->ClearReactDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::RespondToSound()
{
  if ( kInvalidMicDirectionIndex != _triggeredDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Responding to sound from direction [%d]", _triggeredDirection );

    // Send das message ONLY when we're reacting to a valid sound
    DASMSG( robot_reacted_to_sound, "robot.reacted_to_sound", "Robot is reacting to a valid sound" );
    DASMSG_SET( s1, GetDebugLabel(), "Behavior handling the reaction");
    DASMSG_SET( i1, _triggeredMicPower * 1000.0f, "1000 * the log10 of the mic power level" );
    DASMSG_SET( i2, _triggeredConfidence, "The direction confidence returned by SE" );
    DASMSG_SET( i3, _triggeredDirection, "The direction the sound came from (0 in front, clockwise, 12 is unknown)" );
    DASMSG_SEND();

    if ( _iVars.reactionBehavior->WantsToBeActivated() )
    {
      DelegateIfInControl( _iVars.reactionBehavior.get() );
      _triggeredDirection = kInvalidMicDirectionIndex; // reset this so we can detect new sounds
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnResponseComplete()
{
  // reset any response we may have been carrying out and start cooldowns
  ClearTriggerDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::OnMicPowerSampleRecorded( double power, MicDirectionConfidence confidence, MicDirectionIndex direction )
{
  if ( CanReactToSound() )
  {
    const bool powerThresholdReached = ( power >= _iVars.absolutePowerThreshold );
    const bool powerConfidenceThresholdReached = ( power >= _iVars.minPowerThreshold );
    const bool confidenceThresholdReached = ( confidence >= _iVars.confidenceThresholdAtMinPower );

    // we want to react in the following cases:
    // + power is above a specific high threshold (kRTS_AbsolutePowerThreshold)
    // + power is above a lower threshold (kRTS_MinPowerThreshold) AND our confidence is above a threshold (kRTS_ConfidenceThresholdAtMinPower)
    if ( powerThresholdReached || ( powerConfidenceThresholdReached && confidenceThresholdReached ) )
    {
      if ( CanReactToDirection( direction ) )
      {
        OnValidSoundDetected( direction, power, confidence );
        return true;
      }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::CanReactToSound() const
{
  const MovementComponent& moveComponent = GetBEI().GetMovementComponent();
  // VIC-5979: Should not react to sound if the lift is moving and it is near the bottom of its range, since the sound
  // of the lift smacking the bottom stop can be loud enough to trigger this
  const float liftLowTol_mm = 25.f;
  const bool isLiftLow = ( GetBEI().GetRobotInfo().GetLiftHeight() < ( LIFT_HEIGHT_LOWDOCK + liftLowTol_mm ) );
  const auto lastTimeLiftWasMoving = moveComponent.GetLastTimeLiftWasMoving();
  const auto latestRobotTime = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  const bool wasLiftMovingRecently = (latestRobotTime - lastTimeLiftWasMoving) < RECENT_LIFT_MOTION_TIME_LIMIT_MS;

  // don't turn if we're currently already turning to a sound ... add in a small cooldown after a reaction
  const bool areWheelsMoving = moveComponent.AreWheelsMoving();
  const EngineTimeStamp_t cooldownTimeStamp = ( _triggerDetectedTime + static_cast<EngineTimeStamp_t>(kRTS_ReReactionCooldown_s * 1000.f ));
  const bool tooSoonSincePrevious = ( GetCurrentTimeMS() < cooldownTimeStamp );

  bool canReact = true;
  canReact &= !( wasLiftMovingRecently && isLiftLow );
  canReact &= !( IsActivated() && ( areWheelsMoving || tooSoonSincePrevious ) );

  return canReact;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::CanReactToDirection( MicDirectionIndex direction ) const
{
  const bool isDirectionSame = ( direction == _triggeredDirection );
  const bool hasValidDirection = ( kInvalidMicDirectionIndex != direction );

  // if we're already reacting to a sound, ignore new sounds that are coming from a cone directly in front of us
  // we're treating these as most likely the same noise we're already responding to
  bool alreadyReactingToDirection = false;
  switch ( direction )
  {
    case 11:
    case 0:
    case 1:
      alreadyReactingToDirection = IsActivated();
      break;

    default:
      alreadyReactingToDirection = false;
  }

  return !isDirectionSame && hasValidDirection && !alreadyReactingToDirection;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnValidSoundDetected( MicDirectionIndex direction, double micPower, MicDirectionConfidence confidence )
{
  // if we were currently responding to a previous sound, stop reacting to it
  if ( IsActivated() )
  {
    CancelDelegates( false );
  }

  // store these for das events
  _triggeredMicPower = micPower;
  _triggeredConfidence = confidence;

  SetTriggerDirection( direction );
  PRINT_DEBUG( "Heard valid sound from direction [%d] (power=%f, confidence=%u)", direction, micPower, confidence );

  // if we were already responding to a previous sound, we need to kick off a new reaction to the new sound
  if ( IsActivated() )
  {
    RespondToSound();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineTimeStamp_t BehaviorReactToSound::GetCurrentTimeMS() const
{
  return BaseStationTimer::getInstance()->GetCurrentTimeStamp(); // internally just a cast from double
}

}
}

