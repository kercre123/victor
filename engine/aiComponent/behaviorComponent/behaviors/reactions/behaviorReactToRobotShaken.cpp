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
#include "util/logging/DAS.h"


#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace{
  // json keys ...
  const char* kKey_RenderInEyeHue             = "renderInEyeHue";
  const char* kKey_AnimGetIn                  = "getInAnimation";
  const char* kKey_Animations                 = "shakeAnimations";

  // local to struct ShakeReaction
  const char* kKey_TriggerThreshold           = "threshold";
  const char* kKey_AnimLooping                = "looping";
  const char* kKey_AnimWaiting                = "waiting";
  const char* kKey_AnimInHand                 = "inHand";
  const char* kKey_AnimOnGround               = "onGround";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::InstanceConfig::InstanceConfig() :
  renderInEyeHue( false ),
  getInAnimation( AnimationTrigger::InvalidAnimTrigger )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::BehaviorReactToRobotShaken(const Json::Value& config) :
  ICozmoBehavior( config )
{
  LoadReactionAnimations( config );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kKey_Animations );
  expectedKeys.insert( kKey_AnimGetIn );
  expectedKeys.insert( kKey_RenderInEyeHue );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  _dVars.shakeStartTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.shakeMaxMagnitude = GetBEI().GetRobotInfo().GetHeadAccelMagnitudeFiltered();

  TriggerAnimationAction* animationAction = new TriggerAnimationAction( _iVars.getInAnimation );
  animationAction->SetRenderInEyeHue( _iVars.renderInEyeHue );

  DelegateIfInControl( animationAction, &BehaviorReactToRobotShaken::PlayNextShakeReactionLoop );
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
      const float stopThreshold = GetShakeStopThreshold();
      if ( ( EState::Shaking == _dVars.state ) && ( accMag < stopThreshold ) )
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
    {
      _dVars.state = EState::Shaking;
      // the first shaking loop always begins with the EReactionLevel::Soft, so no need to update anything here ...

      break;
    }

    case EState::Shaking:
    {
      // update our current reaction level so that we grab the appropriate animation
      UpdateCurrentReactionLevel();

      break;
    }

    default:
      DEV_ASSERT( false, "BehaviorReactToRobotShaken.PlayNextShakeReactionLoop.InvalidState" );
      return;
  }

  // we retrigger this looping animation each time through so that we can adjust the "level" in case the shaking
  // magnitude increases
  const AnimationTrigger reactionAnim = GetReactionAnimation( EReactionAnimation::Loop );
  TriggerAnimationAction* animationAction = new TriggerAnimationAction( reactionAnim );
  animationAction->SetRenderInEyeHue( _iVars.renderInEyeHue );

  DelegateIfInControl( animationAction, &BehaviorReactToRobotShaken::PlayNextShakeReactionLoop );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::TransitionToDoneShaking()
{
  _dVars.state = EState::DoneShaking;
  
  // we're done shaking, cache the end time so we know how long we were shaking
  _dVars.shakeEndTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // cancel the looping animation
  CancelDelegates( false );

  auto getOutCallback = [this]()
  {
    // now that we've had time to transition out of the shake anim, check if we're on the ground or still in hand and
    // play the appropriate animation
    const bool isOnTreads = ( GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads );
    const EReactionAnimation reactionType = ( isOnTreads ? EReactionAnimation::OnGround : EReactionAnimation::InHand );
    const AnimationTrigger reactionAnim = GetReactionAnimation( reactionType );

    TriggerAnimationAction* animationAction = new TriggerAnimationAction( reactionAnim );
    animationAction->SetRenderInEyeHue( _iVars.renderInEyeHue );

    DelegateIfInControl( animationAction );
  };

  const AnimationTrigger reactionAnim = GetReactionAnimation( EReactionAnimation::ShakeTransition );
  TriggerAnimationAction* animationAction = new TriggerAnimationAction( reactionAnim );
  animationAction->SetRenderInEyeHue( _iVars.renderInEyeHue );

  DelegateIfInControl( animationAction, getOutCallback );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::OnBehaviorDeactivated()
{
  // Log some DAS stuff:
  const float shakeDuration_s = ( _dVars.shakeEndTime - _dVars.shakeStartTime );
  const int shakenDuration_ms = std::round( shakeDuration_s * 1000.f );
  const int maxShakenAccelMag = std::round( _dVars.shakeMaxMagnitude );

  DASMSG( robot_dizzy_reaction, "robot.dizzy_reaction", "Robot is dizzy after being shaken" );
  DASMSG_SET( i1, maxShakenAccelMag, "max shake magnitude while robot was shaken" );
  DASMSG_SET( i2, shakenDuration_ms, "the duration the robot was shaken in ms" );
  DASMSG_SEND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::UpdateCurrentReactionLevel()
{
  const ReactionLevel targetLevel = GetReactionLevelFromMagnitude();
  if ( targetLevel > _dVars.currentLevel )
  {
    ++_dVars.currentLevel;
  }
  else if ( targetLevel < _dVars.currentLevel )
  {
    --_dVars.currentLevel;
  }

  LOG_DEBUG( "BehaviorReactToRobotShaken.UpdateCurrentReactionLevel", "Current ReactionLevel now %d", (int)_dVars.currentLevel );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::ReactionLevel BehaviorReactToRobotShaken::GetReactionLevelFromMagnitude() const
{
  const size_t maxIndex = ( _iVars.shakeReactions.size() - 1 );

  // user our instantaneous magnitude to determine the shake level
  // note: we could use a rolling average, but I don't feel it's necessary
  const float shakeMagnitude = GetBEI().GetRobotInfo().GetHeadAccelMagnitudeFiltered();
  for ( size_t index = maxIndex; index >= 1; --index )
  {
    if ( shakeMagnitude >= _iVars.shakeReactions[index].threshold )
    {
      return index;
    }
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotShaken::ShakeReaction::ShakeReaction( const Json::Value& config )
{
  static const std::string debugString = "BehaviorReactToRobotShaken.ShakeReaction.Load";

  const auto loadAnimation = [&config]( const char* animKey )->AnimationTrigger
  {
    ASSERT_NAMED_EVENT( config.isMember(animKey), debugString.c_str(), "must specify shake animation (%s)", animKey );

    const std::string& trigger = JsonTools::ParseString( config, animKey, debugString );
    return AnimationTriggerFromString( trigger );
  };


  threshold = JsonTools::ParseFloat( config, kKey_TriggerThreshold, debugString );

  animations[(size_t)EReactionAnimation::Loop]            = loadAnimation( kKey_AnimLooping );
  animations[(size_t)EReactionAnimation::ShakeTransition] = loadAnimation( kKey_AnimWaiting );
  animations[(size_t)EReactionAnimation::InHand]          = loadAnimation( kKey_AnimInHand );
  animations[(size_t)EReactionAnimation::OnGround]        = loadAnimation( kKey_AnimOnGround );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotShaken::LoadReactionAnimations( const Json::Value& config )
{
  const std::string& debugString = "BehaviorReactToRobotShaken.LoadReactionAnimations";

  // Do we want to render these animations in the eye hue, or use the rgb from the animations themselves
  _iVars.renderInEyeHue = config.get( kKey_RenderInEyeHue, true ).asBool();

  // load our get-in animation
  ASSERT_NAMED_EVENT( config.isMember(kKey_AnimGetIn), debugString.c_str(), "must specify get-in animation (%s)", kKey_AnimGetIn );
  const std::string& getInAnimString = JsonTools::ParseString( config, kKey_AnimGetIn, debugString );
  _iVars.getInAnimation = AnimationTriggerFromString( getInAnimString );

  // load all of our reaction animations
  ASSERT_NAMED_EVENT( config.isMember( kKey_Animations ), debugString.c_str(), "missing required field (%s)", kKey_Animations );
  for ( const auto& reactionConfig : config[kKey_Animations] )
  {
    _iVars.shakeReactions.emplace_back( reactionConfig );
  }

  ASSERT_NAMED_EVENT( !_iVars.shakeReactions.empty(), debugString.c_str(), "must specify at least one set of reactions" );

  // order our animations by ascending threshold so that we can easily jump between them as we shake
  std::sort( _iVars.shakeReactions.begin(), _iVars.shakeReactions.end(), []( const ShakeReaction& lhs, const ShakeReaction& rhs )
  {
    return ( lhs.threshold < rhs.threshold );
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToRobotShaken::GetReactionAnimation( EReactionAnimation type ) const
{
  return GetReactionAnimation( _dVars.currentLevel, type );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorReactToRobotShaken::GetReactionAnimation( ReactionLevel level, EReactionAnimation type ) const
{
  return _iVars.shakeReactions[level].animations[static_cast<uint8_t>(type)];
}

} // namespace Vector
} // namespace Anki
