/**
 * File: behaviorReactToRobotShaken.cpp
 *
 * Author: Matt Michini
 * Created: 2017-01-11
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotShaken.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/animActions.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"


#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace{
  // json keys ...
  // Accelerometer magnitude threshold corresponding to "no longer shaking"
  const char* kKey_ShakeThresholdStop         = "shakeThresholdStop";
  // Dizzy factor thresholds for playing the soft, medium, or hard reactions
  const char* kKey_ShakeThresholdMedium       = "shakeThresholdMedium";
  const char* kKey_ShakeThresholdHard         = "shakeThresholdHard";

  const float kDefaultShakeThresholdStop      = 13000.f;
  const float kDefaultShakeThresholdMedium    = 18000.f;
  const float kDefaultShakeThresholdHard      = 20000.f;
//  const float kShakenDurationThresholdMedium  = 2.5f;
//  const float kShakenDurationThresholdHard    = 5.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::InstanceConfig::InstanceConfig() :
  shakeThresholdStop( kDefaultShakeThresholdStop ),
  shakeThresholdMedium( kDefaultShakeThresholdMedium ),
  shakeThresholdHard( kDefaultShakeThresholdHard )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::BehaviorReactToRobotShaken(const Json::Value& config) :
  ICozmoBehavior( config )
{
  JsonTools::GetValueOptional( config, kKey_ShakeThresholdStop, _iVars.shakeThresholdStop );
  JsonTools::GetValueOptional( config, kKey_ShakeThresholdMedium, _iVars.shakeThresholdMedium );
  JsonTools::GetValueOptional( config, kKey_ShakeThresholdHard, _iVars.shakeThresholdHard );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kKey_ShakeThresholdStop );
  expectedKeys.insert( kKey_ShakeThresholdMedium );
  expectedKeys.insert( kKey_ShakeThresholdHard );
//  expectedKeys.insert( kKey_ShakeDurationMedium );
//  expectedKeys.insert( kKey_ShakeDurationHard );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  _dVars.shakeStartTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.shakeMaxMagnitude = GetBEI().GetRobotInfo().GetHeadAccelMagnitudeFiltered();

  DelegateIfInControl( new TriggerAnimationAction( AnimationTrigger::ReactToShake_GetIn ),
                       &BehaviorReactToRobotShaken::PlayNextShakeReactionLoop );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::BehaviorUpdate()
{
  if ( !IsActivated() )
  {
    return;
  }

  // Master state machine:
  switch ( _dVars.state )
  {
    case EState::ShakeGetIn:
    case EState::Shaking:
    {
      const float accMag = GetBEI().GetRobotInfo().GetHeadAccelMagnitudeFiltered();
      _dVars.shakeMaxMagnitude = std::max( _dVars.shakeMaxMagnitude, accMag );

      // Done shaking? Then transition to the next state.
      if ( ( EState::Shaking == _dVars.state ) && ( accMag < _iVars.shakeThresholdStop ) )
      {
        TransitionToDoneShaking();
      }

      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::PlayNextShakeReactionLoop()
{
  switch ( _dVars.state )
  {
    case EState::ShakeGetIn:
      _dVars.state = EState::Shaking;
      // ... fall through
    case EState::Shaking:
    {
      // we retrigger this looping animation each time through so that we can adjust the "level" in case the shaking
      // magnitude increases
      const AnimationTrigger reactionAnim = GetReactionAnimation( EReactionAnimation::Loop );
      DelegateIfInControl( new TriggerAnimationAction( reactionAnim ), &BehaviorReactToRobotShaken::PlayNextShakeReactionLoop );

      break;
    }

    default:
      DEV_ASSERT( false, "BehaviorReactToRobotShaken.PlayNextShakeReactionLoop.InvalidState" );
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::TransitionToDoneShaking()
{
  _dVars.state = EState::DoneShaking;
  
  // we're done shaking, cache the end time so we know how long we were shaking
  _dVars.shakeEndTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // cancel the looping animation
  CancelDelegates( false );

  auto getOutCallbcak = [this]()
  {
    // now that we've had time to transition out of the shake anim, check if we're on the ground or still in hand and
    // play the appropriate animation
    const bool isOnTreads = ( GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads );
    const EReactionAnimation reactionType = ( isOnTreads ? EReactionAnimation::OnGround : EReactionAnimation::InHand );
    const AnimationTrigger reactionAnim = GetReactionAnimation( reactionType );
    DelegateIfInControl( new TriggerAnimationAction( reactionAnim ) );
  };

  const AnimationTrigger reactionAnim = GetReactionAnimation( EReactionAnimation::ShakeTransition );
  DelegateIfInControl( new TriggerAnimationAction( reactionAnim ), getOutCallbcak );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::OnBehaviorDeactivated()
{
  BehaviorObjectiveAchieved( BehaviorObjective::ReactedToRobotShaken );

  // Log some DAS stuff:
  const float shakeDuration_s = ( _dVars.shakeEndTime - _dVars.shakeStartTime );
  const int shakenDuration_ms = std::round( shakeDuration_s * 1000.f );
  const int maxShakenAccelMag = std::round( _dVars.shakeMaxMagnitude );

  const EReactionLevel level = GetReactionLevelFromMagnitude();
  const std::string levelString = EReactionLevelToString( level );

  // DAS event string: "<shakenDuration_ms>:<maxShakenAccelMag>"
  const std::string& data = std::to_string( shakenDuration_ms ) + ":" + std::to_string( maxShakenAccelMag );
  Anki::Util::sInfo( "robot.dizzy_reaction",
                     {{DDATA, data.c_str()}},
                     levelString.c_str() );

  // Log human-readable completion info:
  LOG_DEBUG( "BehaviorReactToRobotShaken.DizzyReaction",
             "shakenDuration = %.3fs, maxShakingAccelMag = %.1f, reactionPlayed = '%s'",
             shakeDuration_s,
             _dVars.shakeMaxMagnitude,
             levelString.c_str() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::EReactionLevel BehaviorReactToRobotShaken::GetReactionLevelFromMagnitude() const
{
  EReactionLevel reaction = EReactionLevel::Soft;

  if ( _dVars.shakeMaxMagnitude >= _iVars.shakeThresholdHard )
  {
    reaction = EReactionLevel::Hard;
  }
  else if ( _dVars.shakeMaxMagnitude >= _iVars.shakeThresholdMedium )
  {
    reaction = EReactionLevel::Medium;
  }

  return reaction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//BehaviorReactToRobotShaken::EReactionLevel BehaviorReactToRobotShaken::GetReactionLevelFromDuration() const
//{
//  EReactionLevel reaction = EReactionLevel::Soft;
//
//  // if we haven't stopped shaking yet, use the current time for our duration
//  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
//  const float shakeEndTime = ( _dVars.shakeEndTime > 0.f ) ? _dVars.shakeEndTime : currentTime;
//  const float shakeDuration_s = ( shakeEndTime - _dVars.shakeStartTime );
//
//  if ( shakeDuration_s > kShakenDurationThresholdHard )
//  {
//    reaction = EReactionLevel::Hard;
//  }
//  else if ( shakeDuration_s > kShakenDurationThresholdMedium )
//  {
//    reaction = EReactionLevel::Medium;
//  }
//
//  return reaction;
//}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToRobotShaken::GetReactionAnimation( EReactionAnimation type ) const
{
  const EReactionLevel level = GetReactionLevelFromMagnitude();
  return GetReactionAnimation( level, type );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToRobotShaken::GetReactionAnimation( EReactionLevel level, EReactionAnimation type ) const
{
  // Static animations for reactions ...

  static AnimationTrigger kLoopReactions[] =
  {
    AnimationTrigger::ReactToShake_Lvl1Loop,
    AnimationTrigger::ReactToShake_Lvl2Loop,
    AnimationTrigger::ReactToShake_Lvl3Loop,
  };

  static AnimationTrigger kWaitReactions[] =
  {
    AnimationTrigger::ReactToShake_Lvl1Waiting,
    AnimationTrigger::ReactToShake_Lvl2Waiting,
    AnimationTrigger::ReactToShake_Lvl3Waiting,
  };

  static AnimationTrigger kInHandReactions[] =
  {
    AnimationTrigger::ReactToShake_Lvl1InHand,
    AnimationTrigger::ReactToShake_Lvl2InHand,
    AnimationTrigger::ReactToShake_Lvl3InHand,
  };

  static AnimationTrigger kOnGroundReactions[] =
  {
    AnimationTrigger::ReactToShake_Lvl1OnGround,
    AnimationTrigger::ReactToShake_Lvl2OnGround,
    AnimationTrigger::ReactToShake_Lvl3OnGround,
  };
  
  static AnimationTrigger* kReactionAnims[] =
  {
    kLoopReactions,
    kWaitReactions,
    kInHandReactions,
    kOnGroundReactions,
  };

  DEV_ASSERT_MSG( ( (uint8_t)level < 3 ), "BehaviorReactToRobotShaken.GetReactionAnimation",
                                          "EReactionLevel enum too large [%d]", (uint8_t)level );
  DEV_ASSERT_MSG( ( (uint8_t)type < 4 ), "BehaviorReactToRobotShaken.GetReactionAnimation",
                                         "EReactionAnimation enum too large [%d]", (uint8_t)type );

  return kReactionAnims[static_cast<uint8_t>(type)][static_cast<uint8_t>(level)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* BehaviorReactToRobotShaken::EReactionLevelToString( EReactionLevel reaction) const
{
  switch ( reaction )
  {
    case EReactionLevel::Soft:
      return "Soft";
    case EReactionLevel::Medium:
      return "Medium";
    case EReactionLevel::Hard:
      return "Hard";
  }
}


} // namespace Vector
} // namespace Anki
