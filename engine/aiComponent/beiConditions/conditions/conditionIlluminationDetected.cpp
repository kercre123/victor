/**
 * File: conditionIlluminationDetected.h
 * 
 * Author: Humphrey Hu
 * Created: June 01 2018
 *
 * Description: Condition that checks the observed scene illumination entering and/or leaving from
 *              specified states
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionIlluminationDetected.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Vector {

namespace {

const char* kPreTriggerStatesKey = "preTriggerStates";
const char* kPostTriggerStatesKey = "postTriggerStates";
const char* kPreConfirmationTimeKey = "preConfirmationTime";
const char* kPostConfirmationTimeKey = "postConfirmationTime";
const char* kPreConfirmationMinNumKey = "preConfirmationMinNum";
const char* kPostConfirmationMinNumKey = "postConfirmationMinNum";

} // namespace 


ConditionIlluminationDetected::ConditionIlluminationDetected( const Json::Value& config )
: IBEICondition( config )
{
  _params.preConfirmationTime_s = 0.0;
  _params.preConfirmationMinNum = 0;
  if( ParseTriggerStates( config, kPreTriggerStatesKey, _params.preStates ) )
  {
    _params.preConfirmationTime_s = JsonTools::ParseFloat( config, kPreConfirmationTimeKey,
                                                           "ConditionIlluminationDetected.Constructor" );
    _params.preConfirmationMinNum = JsonTools::ParseUInt32( config, kPreConfirmationMinNumKey,
                                                            "ConditionIlluminationDetected.Constructor" );
  }

  _params.postConfirmationTime_s = 0.0;
  _params.postConfirmationMinNum = 0;
  if( ParseTriggerStates( config, kPostTriggerStatesKey, _params.postStates ) )
  {
    _params.postConfirmationTime_s = JsonTools::ParseFloat( config, kPostConfirmationTimeKey,
                                                          "ConditionIlluminationDetected.Constructor" );
    _params.postConfirmationMinNum = JsonTools::ParseUInt32( config, kPostConfirmationMinNumKey,
                                                           "ConditionIlluminationDetected.Constructor" );
  }

  _params.matchHoldTime_s = JsonTools::ParseFloat( config, "matchHoldTime",
                                                   "ConditionIlluminationDetect.Constructor" );
}

ConditionIlluminationDetected::~ConditionIlluminationDetected() {}

void ConditionIlluminationDetected::GetRequiredVisionModes(std::set<VisionModeRequest>& request) const
{
  request.insert({ VisionMode::DetectingIllumination, EVisionUpdateFrequency::High });
}

void ConditionIlluminationDetected::InitInternal( BehaviorExternalInterface& bei )
{
  _messageHelper.reset( new BEIConditionMessageHelper( this, bei ) );
  _messageHelper->SubscribeToTags( {EngineToGameTag::RobotObservedIllumination} );

  Reset();
}

bool ConditionIlluminationDetected::AreConditionsMetInternal( BehaviorExternalInterface& bei ) const
{
  return MatchState::MatchConfirmed == _variables.matchState;
}

void ConditionIlluminationDetected::HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei )
{
  switch( event.GetData().GetTag() )
  {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedIllumination:
    {
      const auto& msg = event.GetData().Get_RobotObservedIllumination();  
      TickStateMachine( msg.timestamp, msg.state );
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR( "ConditionIlluminationDetected.HandleEvent.InvalidEvent", "" );
    }
  }
}

void ConditionIlluminationDetected::TickStateMachine( const RobotTimeStamp_t& currTime, const IlluminationState& obsState )
{

  switch( _variables.matchState )
  {
    case MatchState::WaitingForPre:
    {
      if( !IsTriggerState( _params.preStates, obsState ) ) 
      {
        Reset();
        break;
      }

      _variables.matchState = MatchState::ConfirmingPre;
      _variables.matchStartTime = currTime;
      _variables.matchedEvents = 0;
      // Fall through to next state immediately
    }
    case MatchState::ConfirmingPre:
    {
      if( !IsTriggerState( _params.preStates, obsState ) )
      {
        Reset();
        break;
      }
   
      ++_variables.matchedEvents;
      if( !IsTimePassed( currTime, _params.preConfirmationTime_s ) || 
          _variables.matchedEvents < _params.preConfirmationMinNum )
      {
        break;
      }
      _variables.matchState = MatchState::WaitingForPost;
      // Fall through to next state immediately

    }
    case MatchState::WaitingForPost:
    {
      if( !IsTriggerState( _params.postStates, obsState ) )
      {
        Reset();
        break;
      }

      _variables.matchState = MatchState::ConfirmingPost;
      _variables.matchStartTime = currTime;
      _variables.matchedEvents = 0;
      // Fall through to next state immediately
    }
    case MatchState::ConfirmingPost:
    {
      if( !IsTriggerState( _params.postStates, obsState ) )
      {
        Reset();
        break;
      }
   
      ++_variables.matchedEvents;
      if( !IsTimePassed( currTime, _params.postConfirmationTime_s ) || 
          _variables.matchedEvents < _params.postConfirmationMinNum )
      {
        break;
      }

      _variables.matchState = MatchState::MatchConfirmed;
      _variables.matchStartTime = currTime;
      // Fall through to next state immediately
    }
    case MatchState::MatchConfirmed:
    {
      if( !IsTriggerState( _params.postStates, obsState ) ||
          IsTimePassed( currTime, _params.postConfirmationTime_s ) )
      {
        Reset();
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("ConditionIlluminationDetected.HandleIllumination.InvalidState", "");
      Reset();
    }
  }
}

bool ConditionIlluminationDetected::IsTimePassed( const RobotTimeStamp_t& t, const f32& dur ) const
{
  if( t < _variables.matchStartTime )
  {
    PRINT_NAMED_ERROR("ConditionIlluminationDetected.HandleIllumination.TimeError",
                      "Time has gone backwards!");
    return false;
  }

  return Util::IsFltGT(Util::MilliSecToSec(f32(t - _variables.matchStartTime)), dur );

}

bool ConditionIlluminationDetected::IsTriggerState( const std::vector<IlluminationState>& triggers,
                                                    const IlluminationState& state ) const
{
  if( triggers.empty() ) {
    return true;
  }

  for( auto iter = triggers.begin(); iter != triggers.end(); ++iter )
  {
    if( *iter == state ) {
      return true;
    }
  }
  return false;
}

void ConditionIlluminationDetected::Reset()
{
  _variables.matchState = MatchState::WaitingForPre;
  _variables.matchedEvents = 0;
}

bool ConditionIlluminationDetected::ParseTriggerStates( const Json::Value& config,
                                                        const char* key,
                                                        std::vector<IlluminationState>& triggers )
{
  triggers.clear();

  // NOTE If no triggers specified, any state will work
  std::vector<std::string> stateStrs;
  if( !JsonTools::GetVectorOptional( config, key, stateStrs ) ) {
    return false;
  }

  IlluminationState state;
  for( auto iter = stateStrs.begin(); iter != stateStrs.end(); ++iter )
  {
    if( !EnumFromString( *iter, state ) )
    {
      PRINT_NAMED_ERROR( "ConditionIlluminationDetected.ParseTriggerStates.InvalidState",
                        "Target state %s is not a valid IlluminationState", iter->c_str() );
    }
    else
    {
      triggers.push_back( state );
    }
  }
  return !triggers.empty();
}

} // end namespace Vector
} // end namespace Anki
