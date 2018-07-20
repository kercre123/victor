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
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "clad/types/animationTrigger.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <functional>


#define DEBUG_MIC_OBSERVING_VERBOSE 1 // add some verbose debugging if trying to track down issues

#if DEBUG_MIC_OBSERVING_VERBOSE
  #define PRINT_DEBUG( format, ... ) \
    PRINT_NAMED_WARNING( "BehaviorReactToSound", format, ##__VA_ARGS__ )
#else
  #define PRINT_DEBUG( format, ... ) \
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", format, ##__VA_ARGS__ )
#endif


namespace Anki {
namespace Cozmo {

CONSOLE_VAR( double, kSoundReaction_ReactiveWindowDuration,            "MicData",  5.00 ); // once we react to a sound, we continue to react for this many seconds
CONSOLE_VAR( double, kSoundReaction_Cooldown,                          "MicData",  0.0 ); // cooldown between "reactive windows"
CONSOLE_VAR( double, kSoundReaction_MaxReactionTime,                   "MicData",  1.00 ); // we have this much time to respond to a sound

// a mic power above this will always be considered a valid reaction sound
CONSOLE_VAR( MicDirectionHistory::MicPowerLevelType,  kRTS_AbsolutePowerThreshold,          "MicData", 2.90 );
// a mic power above this will require a confidence of at least kRTS_ConfidenceThresholdAtMinPower to be considered a valid reaction sound
CONSOLE_VAR( MicDirectionHistory::MicPowerLevelType,  kRTS_MinPowerThreshold,               "MicData", 1.90 );
CONSOLE_VAR( MicDirectionConfidence,                  kRTS_ConfidenceThresholdAtMinPower,   "MicData", 5000 );

namespace {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ... Static Animation Data ...

  const MicDirectionConfidence kDefaultTriggerThreshold = 15000;
  const BehaviorReactToSound::DirectionTrigger kTriggerThreshold_Asleep[] =
  {
    /*  0 */ { kDefaultTriggerThreshold },
    /*  1 */ { kDefaultTriggerThreshold },
    /*  2 */ { kDefaultTriggerThreshold },
    /*  3 */ { kDefaultTriggerThreshold },
    /*  4 */ { kDefaultTriggerThreshold },
    /*  5 */ { kDefaultTriggerThreshold },
    /*  6 */ { kDefaultTriggerThreshold },
    /*  7 */ { kDefaultTriggerThreshold },
    /*  8 */ { kDefaultTriggerThreshold },
    /*  9 */ { kDefaultTriggerThreshold },
    /* 10 */ { kDefaultTriggerThreshold },
    /* 11 */ { kDefaultTriggerThreshold },
  };

  static_assert( sizeof(kTriggerThreshold_Asleep)/sizeof(BehaviorReactToSound::DirectionTrigger) == kNumMicDirections,
                "The array [kTriggerThreshold_Asleep] is missing data for all mic directions (in behaviorReactToSound.cpp)" );

  const BehaviorReactToSound::DirectionTrigger kTriggerThreshold_Awake[] =
  {
    /*  0 */ { 21000 }, // higher for now as a hack to work around hearing his own audio
    /*  1 */ { kDefaultTriggerThreshold },
    /*  2 */ { kDefaultTriggerThreshold },
    /*  3 */ { kDefaultTriggerThreshold },
    /*  4 */ { kDefaultTriggerThreshold },
    /*  5 */ { kDefaultTriggerThreshold },
    /*  6 */ { kDefaultTriggerThreshold },
    /*  7 */ { kDefaultTriggerThreshold },
    /*  8 */ { kDefaultTriggerThreshold },
    /*  9 */ { kDefaultTriggerThreshold },
    /* 10 */ { kDefaultTriggerThreshold },
    /* 11 */ { kDefaultTriggerThreshold },
  };

  static_assert( sizeof(kTriggerThreshold_Awake)/sizeof(BehaviorReactToSound::DirectionTrigger) == kNumMicDirections,
                "The array [kTriggerThreshold_Awake] is missing data for all mic directions (in behaviorReactToSound.cpp)" );
  
  const char* const kFromSleepKey = "FromSleep";
  const char* const kMicDirectionReactionBehavior = "micDirectionReactionBebavior";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::InstanceConfig::InstanceConfig() :
  reactorId( kInvalidSoundReactorId )
{

}

BehaviorReactToSound::DynamicVariables::DynamicVariables()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::BehaviorReactToSound( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  // whether or not we're responding from the asleep or awake/observing state will be determined via json config.
  // this allows us to have different state "tree/graphs" for when victor is in the sleep or awake behavior state.
  const bool isSleeping = JsonTools::ParseBool( config, kFromSleepKey, "BehaviorReactToSound.Params.ObservationStatus" );
  _observationStatus = ( isSleeping ? EObservationStatus::EObservationStatus_Asleep : EObservationStatus::EObservationStatus_Awake );

  // get the behavior to play after an intent comes in
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
    kFromSleepKey,
    kMicDirectionReactionBehavior
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
  // we heard a sound that we want to focus on, so let's activate our behavior so that we can respond to this
  // setting _triggeredDirection will cause us to activate
  return _iVars.reactionBehavior->WantsToBeActivatedBehavior();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
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
  MicDirectionHistory& micHistory = GetBEI().GetMicComponent().GetMicDirectionHistory();
  _iVars.reactorId = micHistory.RegisterSoundReactor( std::bind( &BehaviorReactToSound::OnMicPowerSampleRecorded, this,
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
  if ( !IsActivated())
  {
    if ( kInvalidMicDirectionIndex != _triggeredDirection )
    {
      const TimeStamp_t currentTime = GetCurrentTimeMS();
      const TimeStamp_t reactionTime = static_cast<TimeStamp_t>( kSoundReaction_MaxReactionTime * 1000.0 );
      if ( currentTime >= ( _triggerDetectedTime + reactionTime ) )
      {
        PRINT_DEBUG( "Clearing triggered direction" );
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::ClearTriggerDirection()
{
  _triggeredDirection = kInvalidMicDirectionIndex;
  _iVars.reactionBehavior->ClearReactDirection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::DirectionTrigger BehaviorReactToSound::GetTriggerData( MicDirectionIndex index ) const
{
  using DirectionTriggerList = const DirectionTrigger*;

  ASSERT_NAMED_EVENT( index >= 0, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index < kNumMicDirections, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );

  const bool isAwake = ( _observationStatus == EObservationStatus::EObservationStatus_Awake );
  const DirectionTriggerList triggerList = ( isAwake ? kTriggerThreshold_Awake : kTriggerThreshold_Asleep );

  DirectionTrigger trigger = triggerList[index];
  return trigger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::CanReactToSound() const
{
  // we can react to sound as long as we're not on cooldown
  const TimeStamp_t currentTime = GetCurrentTimeMS();
  const TimeStamp_t cooldownBeginTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownEndTime = GetCooldownEndTime();

  const bool notOnCooldown = ( ( currentTime < cooldownBeginTime ) || ( currentTime >= cooldownEndTime ) );
  return notOnCooldown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::RespondToSound()
{
  PRINT_DEBUG( "Responding to sound from direction [%d]", _triggeredDirection );

  if ( _iVars.reactionBehavior->WantsToBeActivated() )
  {
    // We want to preserve the time of the first reaction to occur after the cooldown is over
    const TimeStamp_t currentTime = GetCurrentTimeMS();
    const TimeStamp_t reTriggerTime = GetCooldownEndTime();
    if ( currentTime >= reTriggerTime )
    {
      _reactionTriggeredTime = GetCurrentTimeMS();
    }

    DelegateIfInControl( _iVars.reactionBehavior.get() );
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
  if ( !IsActivated() )
  {
    const bool powerThresholdReached = ( power >= kRTS_AbsolutePowerThreshold );
    const bool powerConfidenceThresholdReached = ( power >= kRTS_MinPowerThreshold );
    const bool confidenceThresholdReached = ( confidence >= kRTS_ConfidenceThresholdAtMinPower );
    if ( powerThresholdReached || ( powerConfidenceThresholdReached && confidenceThresholdReached ) )
    {
      const bool isDirectionSame = ( direction == _triggeredDirection );
      const bool hasValidDirection = ( kInvalidMicDirectionIndex != direction );

      if ( !isDirectionSame && hasValidDirection && CanReactToSound() )
      {
        SetTriggerDirection( direction );
        PRINT_DEBUG( "Heard valid sound from direction [%d]", direction );

        return true;
      }
#if ANKI_DEV_CHEATS
      else
      {
        if ( !isDirectionSame ) {
          std::string debug = "Reached";
          if ( !hasValidDirection ) { debug +=    " | invalid dir"; }
          if ( !CanReactToSound() ) { debug +=    " | on cooldown"; }
          GetBEI().GetMicComponent().GetMicDirectionHistory().SendWebvizDebugString( debug );
        }
      }
#endif
    }
#if ANKI_DEV_CHEATS
    else if ( powerConfidenceThresholdReached )
    {
      GetBEI().GetMicComponent().GetMicDirectionHistory().SendWebvizDebugString( "Reached | conf too low" );
    }
#endif
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCurrentTimeMS() const
{
  return static_cast<TimeStamp_t>( BaseStationTimer::getInstance()->GetCurrentTimeInSecondsDouble() * 1000.0 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCooldownBeginTime() const
{
  const TimeStamp_t reactiveWindowDuration = static_cast<TimeStamp_t>( kSoundReaction_ReactiveWindowDuration * 1000.0 );
  return static_cast<TimeStamp_t>( _reactionTriggeredTime + reactiveWindowDuration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCooldownEndTime() const
{
  const TimeStamp_t reactiveWindowEndTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownDuration = static_cast<TimeStamp_t>( kSoundReaction_Cooldown * 1000.0 );
  return ( reactiveWindowEndTime + cooldownDuration );
}

}
}

