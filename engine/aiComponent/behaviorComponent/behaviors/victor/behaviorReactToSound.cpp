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


// #include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorReactToSound.h"
#include "behaviorReactToSound.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "clad/types/animationTrigger.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <chrono>


#define DEBUG_MIC_OBSERVING_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

namespace {
  using MicDirectionConfidence  = MicDirectionHistory::DirectionConfidence;
  using MicDirectionIndex       = MicDirectionHistory::DirectionIndex;
}

CONSOLE_VAR( float, kSoundReaction_ReactiveWindowDuration,            "MicData",  5.00f ); // once we react to a sound, we continue to react for this many seconds
CONSOLE_VAR( float, kSoundReaction_Cooldown,                          "MicData", 20.00f ); // cooldown between "reactive windows"
CONSOLE_VAR( float, kSoundReaction_MaxReactionTime,                   "MicData",  1.00f ); // we have this much time to respond to a sound

CONSOLE_VAR( bool, kSoundReaction_TuningMode,                         "MicData", false );
CONSOLE_VAR( MicDirectionConfidence, kSoundReaction_TuningThreshold,  "MicData", 15000 );

CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeDirection,         "MicData", MicDirectionHistory::kDirectionUnknown );
CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeConfidence,        "MicData", kSoundReaction_TuningThreshold );

const MicDirectionIndex BehaviorReactToSound::kInvalidDirectionIndex = { MicDirectionHistory::kDirectionUnknown };

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

  static_assert( sizeof(kTriggerThreshold_Asleep)/sizeof(BehaviorReactToSound::DirectionTrigger) == MicDirectionHistory::kNumDirections,
                "The array [kTriggerThreshold_Asleep] is missing data for all mic directions (in behaviorReactToSound.cpp)" );

  const BehaviorReactToSound::DirectionTrigger kTriggerThreshold_Awake[] =
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

  static_assert( sizeof(kTriggerThreshold_Awake)/sizeof(BehaviorReactToSound::DirectionTrigger) == MicDirectionHistory::kNumDirections,
                "The array [kTriggerThreshold_Awake] is missing data for all mic directions (in behaviorReactToSound.cpp)" );


  // temp until we get anims ...
  const Radians kFacingAhead  = Radians( 0 );
  const Radians kFacingRight  = Radians( -M_PI_F / 2.0f );
  const Radians kFacingBehind = Radians( M_PI_F );
  const Radians kFacingLeft   = Radians( M_PI_F / 2.0f );

  const BehaviorReactToSound::DirectionResponse kSoundResponses_AsleepOnCharger[] =
  {
    /*  0 */ { AnimationTrigger::ReactToSoundOnChargerAsleepFront,          kFacingAhead },
    /*  1 */ { AnimationTrigger::ReactToSoundOnChargerAsleepFront,          kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToSoundOnChargerAsleepRight,          kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToSoundOnChargerAsleepRight,          kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToSoundOnChargerAsleepRight,          kFacingRight },
    /*  5 */ { AnimationTrigger::ReactToSoundOnChargerAsleepBehind,         kFacingBehind },
    /*  6 */ { AnimationTrigger::ReactToSoundOnChargerAsleepBehind,         kFacingBehind },
    /*  7 */ { AnimationTrigger::ReactToSoundOnChargerAsleepBehind,         kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToSoundOnChargerAsleepLeft,           kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToSoundOnChargerAsleepLeft,           kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToSoundOnChargerAsleepLeft,           kFacingLeft },
    /* 11 */ { AnimationTrigger::ReactToSoundOnChargerAsleepFront,          kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AsleepOnCharger)/sizeof(BehaviorReactToSound::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AsleepOnCharger] is missing data for all mic directions (in behaviorReactToSound.cpp)" );

  const BehaviorReactToSound::DirectionResponse kSoundResponses_AwakeOnCharger[] =
  {
    /*  0 */ { AnimationTrigger::ReactToSoundOnChargerObserveFront,         kFacingAhead },
    /*  1 */ { AnimationTrigger::ReactToSoundOnChargerObserveFront,         kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToSoundOnChargerObserveRight,         kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToSoundOnChargerObserveRight,         kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToSoundOnChargerObserveRight,         kFacingRight },
    /*  5 */ { AnimationTrigger::ReactToSoundOnChargerObserveBehind,        kFacingBehind },
    /*  6 */ { AnimationTrigger::ReactToSoundOnChargerObserveBehind,        kFacingBehind },
    /*  7 */ { AnimationTrigger::ReactToSoundOnChargerObserveBehind,        kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToSoundOnChargerObserveLeft,          kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToSoundOnChargerObserveLeft,          kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToSoundOnChargerObserveLeft,          kFacingLeft },
    /* 11 */ { AnimationTrigger::ReactToSoundOnChargerObserveFront,         kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AwakeOnCharger)/sizeof(BehaviorReactToSound::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AwakeOnCharger] is missing data for all mic directions (in behaviorReactToSound.cpp)" );

  const BehaviorReactToSound::DirectionResponse kSoundResponses_AsleepOffCharger[] =
  {
    /*  0 */ { AnimationTrigger::ReactToSoundOffChargerAsleepFront,         kFacingAhead },
    /*  1 */ { AnimationTrigger::ReactToSoundOffChargerAsleepFront,         kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToSoundOffChargerAsleepFrontRight,    kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToSoundOffChargerAsleepRight,         kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToSoundOffChargerAsleepRight,         kFacingRight },
    /*  5 */ { AnimationTrigger::ReactToSoundOffChargerAsleepBehindRight,   kFacingBehind },
    /*  6 */ { AnimationTrigger::ReactToSoundOffChargerAsleepBehind,        kFacingBehind },
    /*  7 */ { AnimationTrigger::ReactToSoundOffChargerAsleepBehindLeft,    kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToSoundOffChargerAsleepLeft,          kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToSoundOffChargerAsleepLeft,          kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToSoundOffChargerAsleepFrontLeft,     kFacingLeft },
    /* 11 */ { AnimationTrigger::ReactToSoundOffChargerAsleepFront,         kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AsleepOffCharger)/sizeof(BehaviorReactToSound::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AsleepOffCharger] is missing data for all mic directions (in ehaviorReactToSound.cpp)" );

  const BehaviorReactToSound::DirectionResponse kSoundResponses_AwakeOffCharger[] =
  {
    /*  0 */ { AnimationTrigger::ReactToSoundOffChargerObserveFront,        kFacingAhead },
    /*  1 */ { AnimationTrigger::ReactToSoundOffChargerObserveFront,        kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToSoundOffChargerObserveFrontRight,   kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToSoundOffChargerObserveRight,        kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToSoundOffChargerObserveRight,        kFacingRight },
    /*  5 */ { AnimationTrigger::ReactToSoundOffChargerObserveBehindRight,  kFacingBehind },
    /*  6 */ { AnimationTrigger::ReactToSoundOffChargerObserveBehind,       kFacingBehind },
    /*  7 */ { AnimationTrigger::ReactToSoundOffChargerObserveBehindLeft,   kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToSoundOffChargerObserveLeft,         kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToSoundOffChargerObserveLeft,         kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToSoundOffChargerObserveFrontLeft,    kFacingLeft },
    /* 11 */ { AnimationTrigger::ReactToSoundOffChargerObserveFront,        kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AwakeOffCharger)/sizeof(BehaviorReactToSound::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AwakeOffCharger] is missing data for all mic directions (in behaviorReactToSound.cpp)" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::BehaviorReactToSound( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  // whether or not we're responding from the asleep or awake/observing state will be determined via json config.
  // this allows us to have different state "tree/graphs" for when victor is in the sleep or awake behavior state.
  const bool isSleeping = JsonTools::ParseBool( config, "FromSleep", "BehaviorReactToSound.Params.ObservationStatus" );
  _observationStatus = ( isSleeping ? EObservationStatus::EObservationStatus_Asleep : EObservationStatus::EObservationStatus_Awake );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.behaviorAlwaysDelegates               = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::WantsToBeActivatedBehavior() const
{
  // we heard a sound that we want to focus on, so let's activate our behavior so that we can respond to this
  // setting _triggeredDirection will cause us to activate
  if ( kInvalidDirectionIndex != _triggeredDirection )
  {
    return true;
  }

  return false;
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
void BehaviorReactToSound::BehaviorUpdate()
{
  // if we're active, it means we're responding to sound
  // if we're not active, it means we're listing for sound in order to activate us
  if ( !IsActivated() )
  {
    // make sure we're not on cooldown, or other reason we shouldn't react
    if ( CanReactToSound() )
    {
      MicDirectionIndex index = kInvalidDirectionIndex;
      if ( HeardValidSound( index ) )
      {
        // setting _triggeredDirection will kick off a reaction via WantsToBeActivatedBehavior()
        _triggeredDirection = index;
      }
    }
  }

  #if REMOTE_CONSOLE_ENABLED
  {
    // this is const when console vars are disabled
    kSoundReaction_FakeDirection = kInvalidDirectionIndex;
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionHistory::NodeList BehaviorReactToSound::GetLatestMicDirectionData() const
{
  const MicDirectionHistory& micHistory = GetBEI().GetMicDirectionHistory();
  MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( kSoundReaction_MaxReactionTime );
  if ( !nodeList.empty() )
  {
    MicDirectionHistory::DirectionNode& node = nodeList.back(); // most recent node

    // allow us to fake some data for testing purposes
    if ( kSoundReaction_FakeDirection != MicDirectionHistory::kDirectionUnknown )
    {
      node.directionIndex = kSoundReaction_FakeDirection;
      node.confidenceMax = kSoundReaction_FakeConfidence;
      node.timestampAtMax = GetCurrentTimeMS();
    }
  }

  return nodeList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::DirectionTrigger BehaviorReactToSound::GetTriggerData( MicDirectionHistory::DirectionIndex index ) const
{
  using DirectionTriggerList = const DirectionTrigger*;

  ASSERT_NAMED_EVENT( index >= 0, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index < MicDirectionHistory::kNumDirections, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );

  const bool isAwake = ( _observationStatus == EObservationStatus::EObservationStatus_Awake );
  const DirectionTriggerList triggerList = ( isAwake ? kTriggerThreshold_Awake : kTriggerThreshold_Asleep );

  DirectionTrigger trigger = triggerList[index];

  // special tuning mode to override any static data thresholds
  if ( kSoundReaction_TuningMode )
  {
    trigger.threshold = kSoundReaction_TuningThreshold;
  }

  return trigger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::DirectionResponse BehaviorReactToSound::GetResponseData( MicDirectionHistory::DirectionIndex index ) const
{
  using DirectionResponseList = const DirectionResponse*;

  ASSERT_NAMED_EVENT( index >= 0, "BehaviorReactToSound.GetResponseData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index < MicDirectionHistory::kNumDirections, "BehaviorReactToSound.GetResponseData", "Invalid index [%d]", index );

  static const DirectionResponseList kAllSoundResponses[EChargerStatus_Num][EObservationStatus_Num] =
  {
    /*  OnCharger */ { kSoundResponses_AsleepOnCharger, kSoundResponses_AwakeOnCharger },
    /* OffCharger */ { kSoundResponses_AsleepOffCharger, kSoundResponses_AwakeOffCharger }
  };
  
  const BEIRobotInfo& robotInfo = GetBEI().GetRobotInfo();

  // we need to update what set of "sound response data" to use depending on Victor's status.
  // victor can resond differently depeding on his current state (which is on/off charger and awake/asleep)
  const EChargerStatus chargerStatus = ( robotInfo.IsOnChargerPlatform() ? EChargerStatus::EChargerStatus_OnCharger : EChargerStatus::EChargerStatus_OffCharger );
  const DirectionResponse* possibleResponses = kAllSoundResponses[chargerStatus][_observationStatus];

  const DirectionResponse response = possibleResponses[index];
  return response;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::CanReactToSound() const
{
  // we can react to sound as long as we're not on cooldown
  const TimeStamp_t currentTime = GetCurrentTimeMS();
  const TimeStamp_t cooldownBeginTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownEndTime = GetCooldownEndTime();

  const bool notOnCooldown = ( ( currentTime < cooldownBeginTime ) || ( currentTime >= cooldownEndTime ) );
  const bool enoughTimeHasPassed = ( currentTime > kSoundReaction_MaxReactionTime ); // avoids "underflow" in case reaction happens at bootup

  return ( notOnCooldown && enoughTimeHasPassed );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::RespondToSound()
{
  // normally we would play an animation, but currently we don't have animation ready, so for now we'll just turn to sound
  if ( kInvalidDirectionIndex != _triggeredDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Responding to sound from direction [%d]", _triggeredDirection );

    // We want to preserve the time of the first reaction to occur after the cooldown is over
    const TimeStamp_t currentTime = GetCurrentTimeMS();
    const TimeStamp_t reTriggerTime = GetCooldownEndTime();
    if ( currentTime >= reTriggerTime )
    {
      _reactionTriggeredTime = GetCurrentTimeMS();
    }

    const DirectionResponse& response = GetResponseData( _triggeredDirection );
    const AnimationTrigger anim = response.animation;
    DelegateIfInControl( new TriggerAnimationAction( anim ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToSound::OnResponseComplete()
{
  // reset any response we may have been carrying out and start cooldowns
  _triggeredDirection = MicDirectionHistory::kDirectionUnknown;
  _reactionEndedTime = GetCurrentTimeMS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::HeardValidSound( MicDirectionHistory::DirectionIndex& outIndex ) const
{
  using NodeList = MicDirectionHistory::NodeList;

  bool shouldReactToSound = false;

  // grab all of the directions we've heard sound from recently
  const NodeList nodeList = GetLatestMicDirectionData();
  for ( auto rit = nodeList.rbegin(); rit != nodeList.rend(); ++rit )
  {
    const MicDirectionHistory::DirectionNode& currentDirectionNode = *rit;
    if ( kInvalidDirectionIndex != currentDirectionNode.directionIndex )
    {
      const DirectionTrigger triggerData = GetTriggerData( currentDirectionNode.directionIndex );
      const TimeStamp_t reactionWindowTime = GetReactionWindowBeginTime();

      #if DEBUG_MIC_OBSERVING_VERBOSE
      {
        PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Heard sound ... direction [%d], confidence [%d >= %d], time [%d >= %d]",
                        currentDirectionNode.directionIndex,
                        currentDirectionNode.confidenceMax,
                        triggerData.threshold,
                        currentDirectionNode.timestampAtMax,
                        reactionWindowTime );
      }
      #endif

      // + we've seen a spike in confidence above a certain threshold
      if ( currentDirectionNode.timestampAtMax >= reactionWindowTime )
      {
        shouldReactToSound |= ( currentDirectionNode.confidenceMax >= triggerData.threshold );
      }

  //      // + we have an average confidence above a certain threshold
  //      if ( currentDirectionNode.timestampEnd >= reactionWindowTime )
  //      {
  //        chooseDirection |= ( currentDirectionNode.confidenceAvg >= kMinimumConfidenceAverage );
  //      }

      if ( shouldReactToSound )
      {
        outIndex = currentDirectionNode.directionIndex;

        PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Heard valid sound from direction [%d] with max conf (%d)",
                      currentDirectionNode.directionIndex, currentDirectionNode.confidenceMax );

        break;
      }
    }
  }

  return shouldReactToSound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCurrentTimeMS() const
{
  using namespace std::chrono;

  const auto currentTime = time_point_cast<milliseconds>(steady_clock::now());
  return static_cast<TimeStamp_t>(currentTime.time_since_epoch().count());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCooldownBeginTime() const
{
  const TimeStamp_t reactiveWindowDuration = static_cast<TimeStamp_t>(Util::SecToMilliSec(kSoundReaction_ReactiveWindowDuration));
  return ( _reactionTriggeredTime + reactiveWindowDuration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetCooldownEndTime() const
{
  const TimeStamp_t reactiveWindowEndTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownDuration = static_cast<TimeStamp_t>(Util::SecToMilliSec(kSoundReaction_Cooldown));
  return ( reactiveWindowEndTime + cooldownDuration );
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorReactToSound::GetReactionWindowBeginTime() const
{
  const TimeStamp_t currentTime = GetCurrentTimeMS();
  const TimeStamp_t reactionWindowDuration = static_cast<TimeStamp_t>(Util::SecToMilliSec(kSoundReaction_MaxReactionTime));
  const TimeStamp_t reactionWindowTime = ( currentTime - reactionWindowDuration );

  return Anki::Util::Max( reactionWindowTime, _reactionEndedTime );
}

}
}

