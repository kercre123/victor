/***********************************************************************************************************************
 *
 *  BehaviorReactToMicDirection
 *  Cozmo
 *
 *  Created by Jarrod Hatfield on 12/06/17.
 *
 *  Description
 *  + This behavior will listen for directional audio and will have Victor face/interact with it in response
 *
 **********************************************************************************************************************/


// #include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMicDirection.h"
#include "behaviorReactToMicDirection.h"
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
  using MicDirectionConfidence    = MicDirectionHistory::DirectionConfidence;
  using MicDirectionIndex         = MicDirectionHistory::DirectionIndex;
}

namespace {

  const std::string kKeyResponseList        = "reaction_list";
  const std::string kKeyDefaultReaction     = "Default";
  const std::string kKeyReactionAnimation   = "anim";
  const std::string kKeyReactionDirection   = "dir";
}

const AnimationTrigger kInvalidAnimationTrigger = AnimationTrigger::Count;
const BehaviorReactToMicDirection::DynamicStateReaction BehaviorReactToMicDirection::kFallbackReaction
{
  {
    { kInvalidAnimationTrigger, ((float)EClockDirection::TwelveOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::OneOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::TwoOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::ThreeOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::FourOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::FiveOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::SixOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::SevenOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::EightOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::NineOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::TenOClock * kRadsPerDirection) },
    { kInvalidAnimationTrigger, ((float)EClockDirection::ElevenOClock * kRadsPerDirection) },
  }
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::DynamicVariables::DynamicVariables() :
  reactionDirection( EClockDirection::Invalid )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::BehaviorReactToMicDirection( const Json::Value& config ) :
  ICozmoBehavior( config )
{
  // load in our reaction animations from json ...
  const Json::Value& allResponses( config[kKeyResponseList] );
  ASSERT_NAMED_EVENT( ( !allResponses.isNull() ), "BehaviorReactToMicDirection.Init",
                      "Config is missing list of response animations (key == reaction_list)" );

  PRINT_NAMED_INFO("TESTING", "Created BehaviorReactToMicDirection");

  // initialize our default data to our fallback data ...
  std::copy( std::begin( kFallbackReaction.directionalResponseList ),
             std::end( kFallbackReaction.directionalResponseList ),
             std::begin( _instanceVars.defaultReaction.directionalResponseList ) );

  LoadDynamicReactionStateData( allResponses );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::~BehaviorReactToMicDirection()
{
  for ( uint state = 0; state < EDynamicReactionState::Num; ++state )
  {
    Util::SafeDelete( _instanceVars.reactions[state] );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::LoadDynamicReactionStateData( const Json::Value& config )
{
  // load our default values ...
  {
    const Json::Value& reactionElement( config[kKeyDefaultReaction] );
    if ( !reactionElement.isNull() )
    {
      LoadDynamicStateReactions( reactionElement, _instanceVars.defaultReaction );
    }
  }

  // now load in all of our custom state reaction data ...
  {
    for ( uint state = 0; state < EDynamicReactionState::Num; ++state )
    {
      // default everything to null, which tells the behavior to fallback to the default values
      _instanceVars.reactions[state] = nullptr;

      const std::string stateName = ConvertReactionStateToString( static_cast<EDynamicReactionState>(state) );
      const Json::Value& reactionElement( config[stateName] );
      if ( !reactionElement.isNull() )
      {
        DynamicStateReaction* newReaction = new DynamicStateReaction();
        LoadDynamicStateReactions( reactionElement, *newReaction );

        _instanceVars.reactions[state] = newReaction;
      }
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::LoadDynamicStateReactions( const Json::Value& config, DynamicStateReaction& reactionState )
{
  for ( uint dir = 0; dir < EClockDirection::NumDirections; ++dir )
  {
    const std::string directionString = ConvertClockDirectionToString( static_cast<EClockDirection>(dir) );
    const Json::Value& responseConfig( config[directionString] );
    ASSERT_NAMED_EVENT( !responseConfig.isNull(), "BehaviorReactToMicDirection.Init",
                       "Direction missing from list of reactions [%s]", directionString.c_str() );

    // default our values
    reactionState.directionalResponseList[dir] = _instanceVars.defaultReaction.directionalResponseList[dir];
    LoadDirectionResponse( responseConfig, reactionState.directionalResponseList[dir] );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::LoadDirectionResponse( const Json::Value& config, DirectionResponse& outResponse )
{
  // all json values are optional ...

  JsonTools::GetValueOptional( config, kKeyReactionDirection, outResponse.facing );

  std::string animString;
  if ( JsonTools::GetValueOptional( config, kKeyReactionAnimation, animString ) )
  {
    if ( !EnumFromString( animString, outResponse.animation ) )
    {
      PRINT_NAMED_ERROR( "BehaviorReactToMicDirection.LoadDirectionResponse",
                         "Invalid AnimationTrigger value supplied in the config [%s]",
                         animString.c_str() );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.behaviorAlwaysDelegates               = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::EClockDirection BehaviorReactToMicDirection::ConvertMicDirectionToClockDirection( MicDirectionHistory::DirectionIndex dir ) const
{
  ASSERT_NAMED_EVENT( dir < static_cast<MicDirectionHistory::DirectionIndex>(EClockDirection::NumDirections),
                     "BehaviorReactToMicDirection.ConvertMicDirectionToClockDirection",
                     "Attempting to convert an invalid mic direction" );

  return static_cast<EClockDirection>(dir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorReactToMicDirection::ConvertReactionStateToString( EDynamicReactionState state ) const
{
  switch ( state )
  {
    case EDynamicReactionState::OnCharger:
      return "OnCharger";
    case EDynamicReactionState::OnSurface:
      return "OnSurface";
    case EDynamicReactionState::OffTreads:
      return "OffTreads";
    case EDynamicReactionState::Num:
      PRINT_NAMED_ERROR( "BehaviorReactToMicDirection.ConvertReactionStateToString", "Invalid conversion of EDynamicReactionState::Num to string" );
      break;
  }

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorReactToMicDirection::ConvertClockDirectionToString( EClockDirection dir ) const
{
  switch ( dir )
  {
    case EClockDirection::TwelveOClock:
      return "TwelveOClock";
    case EClockDirection::OneOClock:
      return "OneOClock";
    case EClockDirection::TwoOClock:
      return "TwoOClock";
    case EClockDirection::ThreeOClock:
      return "ThreeOClock";
    case EClockDirection::FourOClock:
      return "FourOClock";
    case EClockDirection::FiveOClock:
      return "FiveOClock";
    case EClockDirection::SixOClock:
      return "SixOClock";
    case EClockDirection::SevenOClock:
      return "SevenOClock";
    case EClockDirection::EightOClock:
      return "EightOClock";
    case EClockDirection::NineOClock:
      return "NineOClock";
    case EClockDirection::TenOClock:
      return "TenOClock";
    case EClockDirection::ElevenOClock:
      return "ElevenOClock";
    case EClockDirection::Invalid:
      PRINT_NAMED_ERROR( "BehaviorReactToMicDirection.ConvertClockDirectionToString", "Invalid conversion of EClockDirection::Invalid to string" );
    break;
  }

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::SetReactDirection( MicDirectionHistory::DirectionIndex dir )
{
  _dynamicVars.reactionDirection = ConvertMicDirectionToClockDirection( dir );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::ClearReactDirection()
{
  _dynamicVars.reactionDirection = EClockDirection::Invalid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToMicDirection::WantsToBeActivatedBehavior() const
{
  // we heard a sound that we want to focus on, so let's activate our behavior so that we can respond to this
  // setting reactionDirection will cause us to activate
  return ( EClockDirection::Invalid != _dynamicVars.reactionDirection );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::OnBehaviorActivated()
{
  RespondToSound();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::OnBehaviorDeactivated()
{
  OnResponseComplete();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::BehaviorUpdate()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BehaviorReactToMicDirection::DirectionResponse& BehaviorReactToMicDirection::GetResponseData( EClockDirection dir ) const
{  
  const BEIRobotInfo& robotInfo = GetBEI().GetRobotInfo();

  // we need to update what set of "sound response data" to use depending on Victor's status.
  // victor can resond differently depeding on his current state (on/off charger, picked up, etc)
  EDynamicReactionState currentState = EDynamicReactionState::OnSurface;
  if ( robotInfo.IsOnCharger() )
  {
    currentState = EDynamicReactionState::OnCharger;
  }
  else if ( robotInfo.IsPickedUp() )
  {
    currentState = EDynamicReactionState::OffTreads;
  }

  // if we have a reaction for this speciic state, use it, else fall back to the default response
  const DynamicStateReaction* result = _instanceVars.reactions[currentState];
  return ( nullptr != result ) ? result->directionalResponseList[dir] :
                                 _instanceVars.defaultReaction.directionalResponseList[dir];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::RespondToSound()
{
  // normally we would play an animation, but currently we don't have animation ready, so for now we'll just turn to sound
  if ( EClockDirection::Invalid != _dynamicVars.reactionDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToMicDirection", "Responding to sound from direction [%u]", _dynamicVars.reactionDirection );

    // if an animation was specified, use it, else procedurally turn to the facing direction
    const DirectionResponse& response = GetResponseData( _dynamicVars.reactionDirection );
    if ( kInvalidAnimationTrigger != response.animation )
    {
      DelegateIfInControl( new TriggerAnimationAction( response.animation ) );
    }
    else
    {
      DelegateIfInControl( new TurnInPlaceAction( response.facing, false ) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::OnResponseComplete()
{
  // reset any response we may have been carrying out and start cooldowns
  ClearReactDirection();
}

}
}

