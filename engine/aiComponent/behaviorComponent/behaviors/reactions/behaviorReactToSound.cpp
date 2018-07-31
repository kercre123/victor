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

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <chrono>


#define DEBUG_MIC_OBSERVING_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

CONSOLE_VAR( float, kSoundReaction_ReactiveWindowDuration,            "MicData",  5.00f ); // once we react to a sound, we continue to react for this many seconds
CONSOLE_VAR( float, kSoundReaction_Cooldown,                          "MicData", 20.00f ); // cooldown between "reactive windows"
CONSOLE_VAR( float, kSoundReaction_MaxReactionTime,                   "MicData",  1.00f ); // we have this much time to respond to a sound

CONSOLE_VAR( bool, kSoundReaction_TuningMode,                         "MicData", false );
CONSOLE_VAR( MicDirectionConfidence, kSoundReaction_TuningThreshold,  "MicData", 15000 );

CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeDirection,         "MicData", kInvalidMicDirectionIndex );
CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeConfidence,        "MicData", kSoundReaction_TuningThreshold );

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::InstanceConfig::InstanceConfig()
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
  if ( kInvalidMicDirectionIndex != _triggeredDirection )
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
    _triggeredDirection = kInvalidMicDirectionIndex;
    // make sure we're not on cooldown, or other reason we shouldn't react
    if ( CanReactToSound() )
    {
      MicDirectionIndex index = kInvalidMicDirectionIndex;
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
    kSoundReaction_FakeDirection = kInvalidMicDirectionIndex;
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionNodeList BehaviorReactToSound::GetLatestMicDirectionData() const
{
  const MicDirectionHistory& micHistory = GetBEI().GetMicComponent().GetMicDirectionHistory();
  MicDirectionNodeList nodeList = micHistory.GetRecentHistory( kSoundReaction_MaxReactionTime );
  if ( !nodeList.empty() )
  {
    MicDirectionNode& node = nodeList.back(); // most recent node

    // allow us to fake some data for testing purposes
    if ( kSoundReaction_FakeDirection != kInvalidMicDirectionIndex )
    {
      node.directionIndex = kSoundReaction_FakeDirection;
      node.confidenceMax = kSoundReaction_FakeConfidence;
      node.timestampAtMax = GetCurrentTimeMS();
    }
  }

  return nodeList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToSound::DirectionTrigger BehaviorReactToSound::GetTriggerData( MicDirectionIndex index ) const
{
  using DirectionTriggerList = const DirectionTrigger*;

  ASSERT_NAMED_EVENT( index >= 0, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index <= kLastMicDirectionIndex, "BehaviorReactToSound.GetTriggerData", "Invalid index [%d]", index );

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
  if ( kInvalidMicDirectionIndex != _triggeredDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToSound", "Responding to sound from direction [%d]", _triggeredDirection );

    // We want to preserve the time of the first reaction to occur after the cooldown is over
    const TimeStamp_t currentTime = GetCurrentTimeMS();
    const TimeStamp_t reTriggerTime = GetCooldownEndTime();
    if ( currentTime >= reTriggerTime )
    {
      _reactionTriggeredTime = GetCurrentTimeMS();
    }

    _iVars.reactionBehavior->SetReactDirection( _triggeredDirection );
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
  _triggeredDirection = kInvalidMicDirectionIndex;
  _reactionEndedTime = GetCurrentTimeMS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToSound::HeardValidSound( MicDirectionIndex& outIndex ) const
{
  bool shouldReactToSound = false;

  // grab all of the directions we've heard sound from recently
  const MicDirectionNodeList nodeList = GetLatestMicDirectionData();
  for ( auto rit = nodeList.rbegin(); rit != nodeList.rend(); ++rit )
  {
    const MicDirectionNode& currentDirectionNode = *rit;
    if ( currentDirectionNode.IsValid() )
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

