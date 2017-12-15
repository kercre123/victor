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

#include "anki/common/basestation/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <chrono>


namespace Anki {
namespace Cozmo {

// Confidence values and direction detection ...
//CONSOLE_VAR( TimeStamp_t, kConfidenceSpikeRecentTime,                             "MicData", 1 * 1000 ); // if we've seen a spike in confidence this recently
//CONSOLE_VAR( TimeStamp_t, kMaxRecentSoundTime,                                    "MicData", 2 * 1000 ); // if we haven't heard a sound in this time
CONSOLE_VAR( MicDirectionHistory::DirectionConfidence, kMinimumConfidenceSpike,   "MicData", 13000 );
CONSOLE_VAR( MicDirectionHistory::DirectionConfidence, kMinimumConfidenceAverage, "MicData", 10000 );

namespace {
  const MicDirectionHistory::DirectionIndex kStraightAhead  { MicDirectionHistory::kFirstIndex };
  const Radians kRadiansPerDirection                        { ( 2.0f * M_PI_F ) / MicDirectionHistory::kNumDirections };
  const Radians kDirectionTolerance                         { Anki::Util::DegToRad( 3 ) };

  const TimeStamp_t kFocusChangeDelay                       ( 2.0 * 1000 );

  // scoring system ...
//  const TimeStamp_t kScoreRequiredDuration                            ( 0.5 * 1000 );
//  const TimeStamp_t kScoreUpdateInterval = kScoreRequiredDuration;
  const TimeStamp_t kScoreMaxHistoryTime                              ( 2.0 * 1000 );
  const MicDirectionHistory::DirectionConfidence kScoreConfidenceMin  ( 8000 );
  const MicDirectionHistory::DirectionConfidence kScoreConfidenceMax  ( 15000 );
  const float kScoreFocusDirectionBonus                               ( 0.1f );
  const float kScoreWeightConfidence                                  ( 0.2f );
  const float kScoreWeightDuration                                    ( 0.4f );
  const float KScoreWeightRecent                                      ( 0.4f );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingSounds::BehaviorObservingSounds( const Json::Value& config ) :
  ICozmoBehavior( config ),
  _focusDirection( kStraightAhead )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::InitBehavior( BehaviorExternalInterface& behaviorExternalInterface )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorObservingSounds::WantsToBeActivatedBehavior( BehaviorExternalInterface& behaviorExternalInterface ) const
{
  bool hasValidMicHistory = false;

  // make sure we have a MicDirectionHistory component and that it contains valid audio data
  // note: are we guaranteed to always have at least some data?  What if mics are disabled?
  if ( behaviorExternalInterface.HasMicDirectionHistory() )
  {
    const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
    const MicDirectionHistory::NodeList list = micHistory.GetRecentHistory( 0 );
    hasValidMicHistory = ( !list.empty() && list.front().IsValid() );
  }

  return hasValidMicHistory;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorObservingSounds::OnBehaviorActivated( BehaviorExternalInterface& behaviorExternalInterface )
{
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  const TimeStamp_t currentTime = GetCurrentTime();
  const bool holdingFocus = ( currentTime < ( _focusLastChangedTime + kFocusChangeDelay ) );
  if ( _shouldProcessSound && !holdingFocus )
  {
    UpdateSoundDirection( behaviorExternalInterface );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status BehaviorObservingSounds::UpdateInternal_WhileRunning( BehaviorExternalInterface& behaviorExternalInterface )
{
  return Status::Running;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::UpdateSoundDirection( const BehaviorExternalInterface& behaviorExternalInterface )
{
  const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
  MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( 0 );
  if ( !nodeList.empty() )
  {
    const MicDirectionHistory::DirectionIndex bestSoundDirection = GetFocusDirection( behaviorExternalInterface );
    if ( ( bestSoundDirection != MicDirectionHistory::kDirectionUnknown )
      && !IsSameDirection( bestSoundDirection, _focusDirection ) )
    {
      TurnTowardsSound( bestSoundDirection );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingSounds::TurnTowardsSound( MicDirectionHistory::DirectionIndex soundDirection )
{
  if ( soundDirection != kStraightAhead )
  {
    // stop processing sound since we're turning and our incoming sound direction is relative to our robots facing
    // reset our focal direciton to straight ahead since we're turning to face that focal direction
    _shouldProcessSound = false;
    _focusDirection = kStraightAhead;

    // turn towards the sound (note: Radians will convert our turn into +/-PI range for us)
    const Radians turnAngle = kRadiansPerDirection * (float)( kStraightAhead - soundDirection );
    TurnInPlaceAction* action = new TurnInPlaceAction( turnAngle.ToFloat(), false );
    {
      action->SetTolerance( kDirectionTolerance );
    }

    PRINT_CH_INFO( "MicData", "BehaviorObservingSounds", "Turning to direction [%d], an angle of (%f)",
                   soundDirection, turnAngle.getDegrees() );

    DelegateIfInControl( action, [this]( const BehaviorExternalInterface& behaviorExternalInterface )
    {
      _shouldProcessSound = true;
      _focusLastChangedTime = GetCurrentTime();
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionHistory::DirectionIndex BehaviorObservingSounds::GetFocusDirection( const BehaviorExternalInterface& behaviorExternalInterface ) const
{
  MicDirectionHistory::DirectionIndex bestDirection = MicDirectionHistory::kDirectionUnknown;

  const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
  MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( 0 );
  if ( !nodeList.empty() )
  {
    const MicDirectionHistory::DirectionNode& currentDirectionNode = nodeList.back(); // most recent node
    PRINT_CH_INFO( "MicData", "BehaviorObservingSounds", "Processing direction [%d] with conf (%d)",
                   currentDirectionNode.directionIndex, currentDirectionNode.confidenceMax );

    // ignore sounds that are coming from our current facing direction
    if ( currentDirectionNode.directionIndex != kStraightAhead )
    {
      bool chooseDirection = false;

      // + we've seen a spike in confidence above a certain threshold
      if ( currentDirectionNode.timestampAtMax >= _focusLastChangedTime )
      {
        chooseDirection |= ( currentDirectionNode.confidenceMax >= kMinimumConfidenceSpike );
      }

      // + we have an average confidence above a certain threshold
      if ( currentDirectionNode.timestampEnd >= _focusLastChangedTime )
      {
        chooseDirection |= ( currentDirectionNode.confidenceAvg >= kMinimumConfidenceAverage );
      }

      if ( chooseDirection )
      {
        bestDirection = currentDirectionNode.directionIndex;
      }
    }
  }

  return bestDirection;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorObservingSounds::IsSameDirection( MicDirectionHistory::DirectionIndex lhs, MicDirectionHistory::DirectionIndex rhs ) const
{
  // TODO: consider sounds coming from relatively the same direciton as the same (ie. a cone)
  return ( lhs == rhs );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t BehaviorObservingSounds::GetCurrentTime() const
{
  using namespace std::chrono;

  const auto currentTime = time_point_cast<milliseconds>(steady_clock::now());
  return static_cast<TimeStamp_t>(currentTime.time_since_epoch().count());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOTE: Work in progress; prototyping some ways to tweak how/when victor decides what is the "best" sound to turn to
//       None of this has been tested ...
std::vector<BehaviorObservingSounds::DirectionScore> BehaviorObservingSounds::GetDirectionScores( const BehaviorExternalInterface& behaviorExternalInterface ) const
{
  std::vector<DirectionScore> scores;

  // the earliest time stamp that we care about sound data, anything before this time we ignore
  // even though we're asking for only the previous x ms of data, currently the history
  const TimeStamp_t currentTime = GetCurrentTime();

  const MicDirectionHistory& micHistory = behaviorExternalInterface.GetMicDirectionHistory();
  const MicDirectionHistory::NodeList nodeList = micHistory.GetRecentHistory( kScoreMaxHistoryTime );

  TimeStamp_t prevNodeEnd = ( currentTime - kScoreMaxHistoryTime );
  for ( MicDirectionHistory::DirectionNode node : nodeList )
  {
    bool nodeIsCandidate = true;
    nodeIsCandidate &= ( node.timestampEnd > prevNodeEnd );
    nodeIsCandidate &= ( node.confidenceAvg >= kScoreConfidenceMin );

    if ( nodeIsCandidate )
    {
      auto it = std::find_if( scores.begin(), scores.end(), [node]( const DirectionScore& scoreNode )
      {
        return ( scoreNode.direction == node.directionIndex );
      } );

      if ( it != scores.end() )
      {
        it->scoreConfidence = Anki::Util::Max( it->scoreConfidence, node.confidenceAvg );
        it->scoreDuration += ( node.timestampEnd - prevNodeEnd );
        it->scoreRecent = Anki::Util::Min( it->scoreRecent, ( currentTime - node.timestampEnd ) );
      }
      else
      {
        DirectionScore newNode =
        {
          .direction = MicDirectionHistory::kDirectionUnknown,
          .score = 0.0f,

          .scoreConfidence = node.confidenceAvg,
          .scoreDuration = ( node.timestampEnd - prevNodeEnd ),
          .scoreRecent = ( currentTime - node.timestampEnd ),
        };

        scores.push_back( newNode );
      }
    }

    prevNodeEnd = node.timestampEnd;
  }

  for ( auto scoreNode : scores )
  {
    const MicDirectionHistory::DirectionConfidence confidence = Anki::Util::Clamp( scoreNode.scoreConfidence, kScoreConfidenceMin, kScoreConfidenceMax );
    const float confidenceScore = ( ( confidence - kScoreConfidenceMin ) / ( kScoreConfidenceMax - kScoreConfidenceMin ) );

    const float durationScore = ( scoreNode.scoreDuration / kScoreMaxHistoryTime );

    const float recentAlpha = ( scoreNode.scoreRecent / kScoreMaxHistoryTime );
    const float recentScore = 1.0f - ( recentAlpha * recentAlpha );

    scoreNode.score += ( confidenceScore * kScoreWeightConfidence );
    scoreNode.score += ( durationScore * kScoreWeightDuration );
    scoreNode.score += ( recentScore * KScoreWeightRecent );

    if ( scoreNode.direction == kStraightAhead )
    {
      scoreNode.score += kScoreFocusDirectionBonus;
    }
  }

  std::sort( scores.begin(), scores.end(), []( const DirectionScore& lhs, const DirectionScore& rhs )
  {
    return ( lhs.score >= rhs.score );
  } );

  return scores;
}

}
}

