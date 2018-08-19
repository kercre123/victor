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


CONSOLE_VAR( float,                        kRTS_MaxReactionTime,                  "SoundReaction", 1.00f ); // we have this much time to respond to a sound


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
    /* 12 */ { kDefaultTriggerThreshold }, // ambient direction
  };

  static_assert( sizeof(kTriggerThreshold_Asleep)/sizeof(BehaviorReactToSound::DirectionTrigger) == ( kNumMicDirections + 1 ),
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
    /* 12 */ { kDefaultTriggerThreshold }, // ambient direction
  };

  static_assert( sizeof(kTriggerThreshold_Awake)/sizeof(BehaviorReactToSound::DirectionTrigger) == ( kNumMicDirections + 1 ),
                "The array [kTriggerThreshold_Awake] is missing data for all mic directions (in behaviorReactToSound.cpp)" );


  const char* const kFromSleepKey = "FromSleep";
  const char* const kMicDirectionReactionBehavior = "micDirectionReactionBehavior";
  const char* const kAbsolutePowerThresholdKey = "micAbsolutePowerThreshold";
  const char* const kMinPowerThresholdKey = "micMinPowerThreshold";
  const char* const kConfidenceThresholdAtMinPowerKey = "micConfidenceThresholdAtMinPower";
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
  
  // whether or not we're responding from the asleep or awake/observing state will be determined via json config.
  // this allows us to have different state "tree/graphs" for when victor is in the sleep or awake behavior state.
  const bool isSleeping = JsonTools::ParseBool( config, kFromSleepKey, debugKey );
  _observationStatus = ( isSleeping ? EObservationStatus::EObservationStatus_Asleep : EObservationStatus::EObservationStatus_Awake );

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
    kFromSleepKey,
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
      const TimeStamp_t reactionTime = static_cast<TimeStamp_t>( kRTS_MaxReactionTime * 1000.0 );
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
BehaviorReactToSound::DirectionTrigger BehaviorReactToSound::GetTriggerData( MicDirectionIndex index ) const
{
  using DirectionTriggerList = const DirectionTrigger*;

  ASSERT_NAMED_EVENT( index >= 0, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index <= kLastMicDirectionIndex, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );

  const bool isAwake = ( _observationStatus == EObservationStatus::EObservationStatus_Awake );
  const DirectionTriggerList triggerList = ( isAwake ? kTriggerThreshold_Awake : kTriggerThreshold_Asleep );

  return triggerList[index];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::CanReactToSound() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::RespondToSound()
{
  if ( kInvalidMicDirectionIndex != _triggeredDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Responding to sound from direction [%d]", _triggeredDirection );

    // Send das message ONLY when we're reacting to a valid sound
    DASMSG( robot_reacted_to_sound, "robot.reacted_to_sound", "Robot is reacting to a valid sound" );
    DASMSG_SET( i1, _triggeredMicPower, "The log10 of the mic power level" );
    DASMSG_SET( i2, _triggeredConfidence, "The direction confidence returned by SE" );
    DASMSG_SET( i3, _triggeredDirection, "The direction the sound came from (0 in front, clockwise, 12 is unknown)" );
    DASMSG_SEND();

    if ( _iVars.reactionBehavior->WantsToBeActivated() )
    {
      DelegateIfInControl( _iVars.reactionBehavior.get() );
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
  if ( !IsActivated() && CanReactToSound() )
  {
    const bool powerThresholdReached = ( power >= _iVars.absolutePowerThreshold );
    const bool powerConfidenceThresholdReached = ( power >= _iVars.minPowerThreshold );
    const bool confidenceThresholdReached = ( confidence >= _iVars.confidenceThresholdAtMinPower );

    // we want to react in the following cases:
    // + power is above a specific high threshold (kRTS_AbsolutePowerThreshold)
    // + power is above a lower threshold (kRTS_MinPowerThreshold) AND our confidence is above a threshold (kRTS_ConfidenceThresholdAtMinPower)
    if ( powerThresholdReached || ( powerConfidenceThresholdReached && confidenceThresholdReached ) )
    {
      const bool isDirectionSame = ( direction == _triggeredDirection );
      const bool hasValidDirection = ( kInvalidMicDirectionIndex != direction );
      if ( !isDirectionSame && hasValidDirection )
      {
        // store these for das events
        _triggeredMicPower = power;
        _triggeredConfidence = confidence;

        SetTriggerDirection( direction );
        PRINT_DEBUG( "Heard valid sound from direction [%d] (power=%f, confidence=%u)", direction, power, confidence );

        return true;
      }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineTimeStamp_t BehaviorReactToSound::GetCurrentTimeMS() const
{
  return BaseStationTimer::getInstance()->GetCurrentTimeStamp(); // internally just a cast from double
}

}
}

