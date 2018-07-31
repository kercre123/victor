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


#define DEBUG_MIC_REACTION_VERBOSE 0 // add some verbose debugging if trying to track down issues

namespace Anki {
namespace Cozmo {

namespace {

  const char* kKeyResponseList        = "reaction_list";
  const char* kKeyDefaultReaction     = "Default";
  const char* kKeyEnabled             = "enabled";
  const char* kKeyReactionAnimation   = "anim";
  const char* kKeyReactionDirection   = "dir";

  constexpr AnimationTrigger kInvalidAnimationTrigger = AnimationTrigger::Count;
  constexpr float kDegreesPerDirection                = 360.0f / (float)kNumMicDirections;
  constexpr float kMinDegreesToTurn                   = 5.0f;
}

const BehaviorReactToMicDirection::DynamicStateReaction BehaviorReactToMicDirection::kFallbackReaction
{
  {
    { kInvalidAnimationTrigger, -((float)EClockDirection::TwelveOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::OneOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::TwoOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::ThreeOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::FourOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::FiveOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::SixOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::SevenOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::EightOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::NineOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::TenOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, -((float)EClockDirection::ElevenOClock * kDegreesPerDirection) },
    { kInvalidAnimationTrigger, 0 },
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
             std::begin( _iVars.defaultReaction.directionalResponseList ) );

  LoadDynamicReactionStateData( allResponses );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::~BehaviorReactToMicDirection()
{
  for ( uint state = 0; state < EDynamicReactionState::Num; ++state )
  {
    Util::SafeDelete( _iVars.reactions[state] );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kKeyResponseList,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::LoadDynamicReactionStateData( const Json::Value& config )
{
  // load our default values ...
  {
    const Json::Value& reactionElement( config[kKeyDefaultReaction] );
    if ( !reactionElement.isNull() )
    {
      LoadDynamicStateReactions( reactionElement, _iVars.defaultReaction );
    }
  }

  // now load in all of our custom state reaction data ...
  {
    for ( uint state = 0; state < EDynamicReactionState::Num; ++state )
    {
      // default everything to null, which tells the behavior to fallback to the default values
      _iVars.reactionEnabled[state] = true;
      _iVars.reactions[state] = nullptr;

      const std::string stateName = ConvertReactionStateToString( static_cast<EDynamicReactionState>(state) );
      const Json::Value& reactionElement( config[stateName] );
      if ( !reactionElement.isNull() )
      {
        JsonTools::GetValueOptional( reactionElement, kKeyEnabled, _iVars.reactionEnabled[state] );
        if ( _iVars.reactionEnabled[state] )
        {
          DynamicStateReaction* newReaction = new DynamicStateReaction();
          LoadDynamicStateReactions( reactionElement, *newReaction );

          _iVars.reactions[state] = newReaction;
        }
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
    reactionState.directionalResponseList[dir] = _iVars.defaultReaction.directionalResponseList[dir];
    LoadDirectionResponse( responseConfig, reactionState.directionalResponseList[dir] );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::LoadDirectionResponse( const Json::Value& config, DirectionResponse& outResponse )
{
  // all json values are optional ...
  {
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

  // if the user specifically disables this direction, "zero" it else
  // note: if the direction is purposely left blank, the "default" value will be used
  bool enabled = true;
  if ( JsonTools::GetValueOptional( config, kKeyEnabled, enabled ) && !enabled)
  {
    outResponse.animation = kInvalidAnimationTrigger;
    outResponse.facing = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::EClockDirection BehaviorReactToMicDirection::ConvertMicDirectionToClockDirection( MicDirectionIndex dir ) const
{
  if ( dir < static_cast<MicDirectionIndex>(EClockDirection::NumDirections) )
  {
    return static_cast<EClockDirection>(dir);
  }
  else
  {
    // the only direction we'll accept that isn't a clock direction is the
    // "unknown" direction
    ASSERT_NAMED_EVENT( dir == kMicDirectionUnknown,
                       "BehaviorReactToMicDirection.ConvertMicDirectionToClockDirection",
                       "Attempting to convert an invalid mic direction" );

    return EClockDirection::Invalid;
  }
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
    case EClockDirection::Ambient:
      return "Ambient";
    case EClockDirection::Invalid:
      PRINT_NAMED_ERROR( "BehaviorReactToMicDirection.ConvertClockDirectionToString", "Invalid conversion of EClockDirection::Invalid to string" );
    break;
  }

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::SetReactDirection( MicDirectionIndex dir )
{
  _dVars.reactionDirection = ConvertMicDirectionToClockDirection( dir );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::ClearReactDirection()
{
  _dVars.reactionDirection = EClockDirection::Invalid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToMicDirection::WantsToBeActivatedBehavior() const
{
  // we heard a sound that we want to focus on, so let's activate our behavior so that we can respond to this
  // setting reactionDirection will cause us to activate
  if ( EClockDirection::Invalid != _dVars.reactionDirection )
  {
    const EDynamicReactionState currentState = GetCurrentReactionState();
    return _iVars.reactionEnabled[currentState];
  }

  return false;
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
  // we need to update what set of "sound response data" to use depending on Victor's status.
  // victor can resond differently depeding on his current state (on/off charger, picked up, etc)
  const EDynamicReactionState currentState = GetCurrentReactionState();

  // if we have a reaction for this speciic state, use it, else fall back to the default response
  const DynamicStateReaction* result = _iVars.reactions[currentState];
  return ( nullptr != result ) ? result->directionalResponseList[dir] :
                                 _iVars.defaultReaction.directionalResponseList[dir];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMicDirection::EDynamicReactionState BehaviorReactToMicDirection::GetCurrentReactionState() const
{
  const BEIRobotInfo& robotInfo = GetBEI().GetRobotInfo();

  // we need to update what set of "sound response data" to use depending on Victor's status.
  // victor can resond differently depeding on his current state (on/off charger, picked up, etc)
  EDynamicReactionState currentState = EDynamicReactionState::OnSurface;
  if ( robotInfo.IsOnChargerPlatform() )
  {
    currentState = EDynamicReactionState::OnCharger;
  }
  else if ( robotInfo.IsPickedUp() )
  {
    currentState = EDynamicReactionState::OffTreads;
  }

  return currentState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::RespondToSound()
{
  // normally we would play an animation, but currently we don't have animation ready, so for now we'll just turn to sound
  if ( EClockDirection::Invalid != _dVars.reactionDirection )
  {
    PRINT_CH_DEBUG( "MicData", "BehaviorReactToMicDirection.Reacting", "Responding to sound from direction [%u]", _dVars.reactionDirection );

    // if an animation was specified, use it, else procedurally turn to the facing direction
    const DirectionResponse& response = GetResponseData( _dVars.reactionDirection );
    if ( kInvalidAnimationTrigger != response.animation )
    {
      DelegateIfInControl( new TriggerLiftSafeAnimationAction( response.animation ) );

      #if DEBUG_MIC_REACTION_VERBOSE
      {
        PRINT_CH_DEBUG( "MicData", "BehaviorReactToMicDirection.Debug", "Playing reaction anim [%s]",
                        AnimationTriggerToString(response.animation) );
      }
      #endif
    }
    else
    {
      if ( std::abs( response.facing ) >= kMinDegreesToTurn )
      {
        // convert degrees to radians
        const Radians rads = DEG_TO_RAD( response.facing );
        DelegateIfInControl( new TurnInPlaceAction( rads.ToFloat(), false ) );

        #if DEBUG_MIC_REACTION_VERBOSE
        {
          PRINT_CH_DEBUG( "MicData", "BehaviorReactToMicDirection.Debug", "Playing reaction turn to [%d] degrees",
                          (int)RAD_TO_DEG(rads.ToFloat()) );
        }
        #endif
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMicDirection::OnResponseComplete()
{
  // reset any response we may have been carrying out and start cooldowns
  _dVars = DynamicVariables();
}

}
}

