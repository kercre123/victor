/***********************************************************************************************************************
 *
 *  BehaviorObservingSounds
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/


// #include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorObservingSounds.h"
#include "behaviorObservingSounds.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <chrono>


namespace Anki {
namespace Cozmo {

namespace {
  using MicDirectionConfidence  = MicDirectionHistory::DirectionConfidence;
  using MicDirectionIndex       = MicDirectionHistory::DirectionIndex;
}

CONSOLE_VAR( float, kSoundReaction_ReactiveWindowDuration,            "MicData", 5.0f ); // once we react to a sound, we continue to react for this many seconds
CONSOLE_VAR( float, kSoundReaction_Cooldown,                          "MicData", 20.0f ); // cooldown between "reactive windows"
CONSOLE_VAR( float, kSoundReaction_MaxReactionTime,                   "MicData", 0.75f ); // we have this much time to respond to a sound

CONSOLE_VAR( bool, kSoundReaction_TuningMode,                         "MicData", false );
CONSOLE_VAR( MicDirectionConfidence, kSoundReaction_TuningThreshold,  "MicData", 15000 );

CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeDirection,         "MicData", MicDirectionHistory::kDirectionUnknown );
CONSOLE_VAR( MicDirectionIndex, kSoundReaction_FakeConfidence,        "MicData", kSoundReaction_TuningThreshold );

const MicDirectionIndex BehaviorObservingSounds::kInvalidDirectionIndex = { MicDirectionHistory::kDirectionUnknown };

namespace {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ... Static Animation Data ...

  const MicDirectionConfidence kDefaultTriggerThreshold = 15000;
  const BehaviorObservingSounds::DirectionTrigger kTriggerThreshold_Asleep[] =
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

  static_assert( sizeof(kTriggerThreshold_Asleep)/sizeof(BehaviorObservingSounds::DirectionTrigger) == MicDirectionHistory::kNumDirections,
                "The array [kTriggerThreshold_Asleep] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );

  const BehaviorObservingSounds::DirectionTrigger kTriggerThreshold_Awake[] =
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

  static_assert( sizeof(kTriggerThreshold_Awake)/sizeof(BehaviorObservingSounds::DirectionTrigger) == MicDirectionHistory::kNumDirections,
                "The array [kTriggerThreshold_Awake] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );


  // temp until we get anims ...
  const Radians kFacingAhead  = Radians( 0 );
  const Radians kFacingRight  = Radians( M_PI_F / 2.0f );
  const Radians kFacingBehind = Radians( M_PI_F );
  const Radians kFacingLeft   = Radians( -M_PI_F / 2.0f );

  const BehaviorObservingSounds::DirectionResponse kSoundResponses_AsleepOnCharger[] =
  {
    /*  0 */ { AnimationTrigger::ObservingLookStraight,   kFacingAhead },
    /*  1 */ { AnimationTrigger::ObservingLookStraight,   kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToOnRightSide,      kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToOnRightSide,      kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToOnRightSide,      kFacingRight },
    /*  5 */ { AnimationTrigger::ObservingLookStraight,   kFacingBehind },
    /*  6 */ { AnimationTrigger::ObservingLookStraight,   kFacingBehind },
    /*  7 */ { AnimationTrigger::ObservingLookStraight,   kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToOnLeftSide,       kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToOnLeftSide,       kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToOnLeftSide,       kFacingLeft },
    /* 11 */ { AnimationTrigger::ObservingLookStraight,   kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AsleepOnCharger)/sizeof(BehaviorObservingSounds::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AsleepOnCharger] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );

  const BehaviorObservingSounds::DirectionResponse kSoundResponses_AwakeOnCharger[] =
  {
    /*  0 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  1 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  5 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  6 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  7 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 11 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AwakeOnCharger)/sizeof(BehaviorObservingSounds::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AwakeOnCharger] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );

  const BehaviorObservingSounds::DirectionResponse kSoundResponses_AsleepOffCharger[] =
  {
    /*  0 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  1 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  5 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  6 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  7 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 11 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AsleepOffCharger)/sizeof(BehaviorObservingSounds::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AsleepOffCharger] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );

  const BehaviorObservingSounds::DirectionResponse kSoundResponses_AwakeOffCharger[] =
  {
    /*  0 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  1 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
    /*  2 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  3 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  4 */ { AnimationTrigger::ReactToOnRightSide,     kFacingRight },
    /*  5 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  6 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  7 */ { AnimationTrigger::ObservingLookStraight,  kFacingBehind },
    /*  8 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /*  9 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 10 */ { AnimationTrigger::ReactToOnLeftSide,      kFacingLeft },
    /* 11 */ { AnimationTrigger::ObservingLookStraight,  kFacingAhead },
  };

  static_assert( sizeof(kSoundResponses_AwakeOffCharger)/sizeof(BehaviorObservingSounds::DirectionResponse) == MicDirectionHistory::kNumDirections,
                "The array [kSoundResponses_AwakeOffCharger] is missing data for all mic directions (in behaviorObservingSounds.cpp)" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingSounds::BehaviorObservingSounds( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  // whether or not we're responding from the asleep or awake/observing state will be determined via json config.
  // this allows us to have different state "tree/graphs" for when victor is in the sleep or awake behavior state.
  bool isSleeping = false;
  if ( JsonTools::GetValueOptional( config, "FromSleep", isSleeping ) )
  {
    _observationStatus = ( isSleeping ? EObservationStatus::EObservationStatus_Asleep : EObservationStatus::EObservationStatus_Awake );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.behaviorAlwaysDelegates               = false; // behavior never ends currently :)
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorObservingSounds::WantsToBeActivatedBehavior() const
{
  // we heard a sound that we want to focus on, so let's activate our behavior so that we can respond to this
  // setting _triggeredDirection will cause us to activate
  if ( kInvalidDirectionIndex != _triggeredDirection )
  {
    PRINT_CH_INFO( "MicData", "BehaviorObservingSounds", "Responding to sound from direction [%d]",
                  _triggeredDirection );

    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::OnBehaviorActivated()
{
  RespondToSound();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::OnBehaviorDeactivated()
{
  OnResponseComplete();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::BehaviorUpdate()
{
  // if we're active, it means we're responding to sound
  // if we're not active, it means we're listing for sound in order to activate us
  if ( IsActivated() )
  {
    if ( _triggeredDirection == kInvalidDirectionIndex )
    {
      CancelSelf();
    }
  }
  else
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionHistory::DirectionNode BehaviorObservingSounds::GetLatestMicDirectionData() const
{
  MicDirectionHistory::DirectionNode node;
  node.directionIndex = kInvalidDirectionIndex; // default to invalid node

  const BehaviorExternalInterface& behaviorExternalInterface = GetBEI();

  const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
  MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( 0 );
  if ( !nodeList.empty() )
  {
    node = nodeList.back(); // most recent node

    // allow us to fake some data for testing purposes
    if ( kSoundReaction_FakeDirection != MicDirectionHistory::kDirectionUnknown )
    {
      node.directionIndex = kSoundReaction_FakeDirection;
      node.confidenceMax = kSoundReaction_FakeConfidence;
      node.timestampAtMax = GetCurrentTime();

      kSoundReaction_FakeDirection = kInvalidDirectionIndex;
    }
  }

  return node;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingSounds::DirectionTrigger BehaviorObservingSounds::GetTriggerData( MicDirectionHistory::DirectionIndex index ) const
{
  ASSERT_NAMED_EVENT( index >= 0, "BehaviorObservingSounds.GetTriggerData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index < MicDirectionHistory::kNumDirections, "BehaviorObservingSounds.GetTriggerData", "Invalid index [%d]", index );

  const bool isAwake = ( _observationStatus == EObservationStatus::EObservationStatus_Awake );
  DirectionTriggerList triggerList = ( isAwake ? kTriggerThreshold_Awake : kTriggerThreshold_Asleep );

  DirectionTrigger trigger = triggerList[index];

  // special tuning mode to override any static data thresholds
  if ( kSoundReaction_TuningMode )
  {
    trigger.threshold = kSoundReaction_TuningThreshold;
  }

  return trigger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingSounds::DirectionResponse BehaviorObservingSounds::GetResponseData( MicDirectionHistory::DirectionIndex index ) const
{
  ASSERT_NAMED_EVENT( index >= 0, "BehaviorObservingSounds.GetResponseData", "Invalid index [%d]", index );
  ASSERT_NAMED_EVENT( index < MicDirectionHistory::kNumDirections, "BehaviorObservingSounds.GetResponseData", "Invalid index [%d]", index );

  static DirectionResponseList kAllSoundResponses[BehaviorObservingSounds::EChargerStatus_Num][BehaviorObservingSounds::EObservationStatus_Num] =
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
bool BehaviorObservingSounds::CanReactToSound() const
{
  // we can react to sound as long as we're not on cooldown
  const TimeStamp_t currentTime = GetCurrentTime();
  const TimeStamp_t cooldownBeginTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownEndTime = GetCooldownEndTime();

  return ( ( currentTime < cooldownBeginTime ) || ( currentTime >= cooldownEndTime ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::RespondToSound()
{
  // normally we would play an animation, but currently we don't have animation ready, so for now we'll just turn to sound
  if ( kInvalidDirectionIndex != _triggeredDirection )
  {
    _reactionTriggeredTime = GetCurrentTime();

    if ( !kSoundReaction_TuningMode )
    {
      const DirectionResponse& response = GetResponseData( _triggeredDirection );
      const Radians turnAngle = response.facing;
      TurnInPlaceAction* action = new TurnInPlaceAction( turnAngle.ToFloat(), false );

      DelegateIfInControl( action, [this]()
      {
        OnResponseComplete();
      });
    }
    else
    {
      // in our special tuning mode, disable turning so that we can observe the debug face display
      OnResponseComplete();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::OnResponseComplete()
{
  // reset any response we may have been carrying out and start cooldowns
  _triggeredDirection = MicDirectionHistory::kDirectionUnknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorObservingSounds::HeardValidSound( MicDirectionHistory::DirectionIndex& outIndex ) const
{
  bool shouldReactToSound = false;

  const MicDirectionHistory::DirectionNode currentDirectionNode = GetLatestMicDirectionData();
  if ( kInvalidDirectionIndex != currentDirectionNode.directionIndex )
  {
    const DirectionTrigger triggerData = GetTriggerData( currentDirectionNode.directionIndex );

    const TimeStamp_t currentTime = GetCurrentTime();
    const TimeStamp_t reactionWindowTime = ( currentTime - kSoundReaction_MaxReactionTime );

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

      PRINT_CH_INFO( "MicData", "BehaviorObservingSounds", "Heard valid sound from direction [%d] with max conf (%d)",
                    currentDirectionNode.directionIndex, currentDirectionNode.confidenceMax );
    }
  }

  return shouldReactToSound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorObservingSounds::GetCurrentTime() const
{
  using namespace std::chrono;

  const auto currentTime = time_point_cast<milliseconds>(steady_clock::now());
  return static_cast<TimeStamp_t>(currentTime.time_since_epoch().count());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorObservingSounds::GetCooldownBeginTime() const
{
  const TimeStamp_t reactiveWindowDuration = static_cast<TimeStamp_t>(kSoundReaction_ReactiveWindowDuration * 1000.0f);
  return ( _reactionTriggeredTime + reactiveWindowDuration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorObservingSounds::GetCooldownEndTime() const
{
  const TimeStamp_t reactiveWindowEndTime = GetCooldownBeginTime();
  const TimeStamp_t cooldownDuration = static_cast<TimeStamp_t>(kSoundReaction_Cooldown * 1000.0f);
  return ( reactiveWindowEndTime + cooldownDuration );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOTE: Work in progress; prototyping some ways to tweak how/when victor decides what is the "best" sound to turn to
//       None of this has been tested ... leaving it here until I get it under version control

//namespace {
//  const MicDirectionIndex kStraightAhead  { MicDirectionHistory::kFirstIndex };
//
//  // scoring system ...
//  //  const TimeStamp_t kScoreRequiredDuration                            ( 0.5 * 1000 );
//  //  const TimeStamp_t kScoreUpdateInterval = kScoreRequiredDuration;
//  const TimeStamp_t kScoreMaxHistoryTime                              ( 2.0 * 1000 );
//  const MicDirectionConfidence kScoreConfidenceMin  ( 8000 );
//  const MicDirectionConfidence kScoreConfidenceMax  ( 15000 );
//  const float kScoreFocusDirectionBonus                               ( 0.1f );
//  const float kScoreWeightConfidence                                  ( 0.2f );
//  const float kScoreWeightDuration                                    ( 0.4f );
//  const float KScoreWeightRecent                                      ( 0.4f );
//}
//
//std::vector<BehaviorObservingSounds::DirectionScore> BehaviorObservingSounds::GetDirectionScores() const
//{
//  const BehaviorExternalInterface& behaviorExternalInterface = GetBEI();
//  std::vector<DirectionScore> scores;
//
//  // the earliest time stamp that we care about sound data, anything before this time we ignore
//  // even though we're asking for only the previous x ms of data, currently the history
//  const TimeStamp_t currentTime = GetCurrentTime();
//
//  const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
//  const MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( kScoreMaxHistoryTime );
//
//  TimeStamp_t prevNodeEnd = ( currentTime - kScoreMaxHistoryTime );
//  for ( MicDirectionHistory::DirectionNode node : nodeList )
//  {
//    bool nodeIsCandidate = true;
//    nodeIsCandidate &= ( node.timestampEnd > prevNodeEnd );
//    nodeIsCandidate &= ( node.confidenceAvg >= kScoreConfidenceMin );
//
//    if ( nodeIsCandidate )
//    {
//      auto it = std::find_if( scores.begin(), scores.end(), [node]( const DirectionScore& scoreNode )
//      {
//        return ( scoreNode.direction == node.directionIndex );
//      } );
//
//      if ( it != scores.end() )
//      {
//        it->scoreConfidence = Anki::Util::Max( it->scoreConfidence, node.confidenceAvg );
//        it->scoreDuration += ( node.timestampEnd - prevNodeEnd );
//        it->scoreRecent = Anki::Util::Min( it->scoreRecent, ( currentTime - node.timestampEnd ) );
//      }
//      else
//      {
//        DirectionScore newNode =
//        {
//          .direction = MicDirectionHistory::kDirectionUnknown,
//          .score = 0.0f,
//
//          .scoreConfidence = node.confidenceAvg,
//          .scoreDuration = ( node.timestampEnd - prevNodeEnd ),
//          .scoreRecent = ( currentTime - node.timestampEnd ),
//        };
//
//        scores.push_back( newNode );
//      }
//    }
//
//    prevNodeEnd = node.timestampEnd;
//  }
//
//  for ( auto scoreNode : scores )
//  {
//    const MicDirectionConfidence confidence = Anki::Util::Clamp( scoreNode.scoreConfidence, kScoreConfidenceMin, kScoreConfidenceMax );
//    const float confidenceScore = ( ( confidence - kScoreConfidenceMin ) / ( kScoreConfidenceMax - kScoreConfidenceMin ) );
//
//    const float durationScore = ( scoreNode.scoreDuration / kScoreMaxHistoryTime );
//
//    const float recentAlpha = ( scoreNode.scoreRecent / kScoreMaxHistoryTime );
//    const float recentScore = 1.0f - ( recentAlpha * recentAlpha );
//
//    scoreNode.score += ( confidenceScore * kScoreWeightConfidence );
//    scoreNode.score += ( durationScore * kScoreWeightDuration );
//    scoreNode.score += ( recentScore * KScoreWeightRecent );
//
//    if ( scoreNode.direction == kStraightAhead )
//    {
//      scoreNode.score += kScoreFocusDirectionBonus;
//    }
//  }
//
//  std::sort( scores.begin(), scores.end(), []( const DirectionScore& lhs, const DirectionScore& rhs )
//  {
//    return ( lhs.score >= rhs.score );
//  } );
//
//  return scores;
//}

}
}

