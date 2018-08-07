/**
 * File: conditionIlluminationDetected.h
 * 
 * Author: Humphrey Hu
 * Created: June 01 2018
 *
 * Description: Condition that checks the observed scene illumination for a particular state
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

const char* kTriggerStatesKey = "triggerStates";

ConditionIlluminationDetected::ConditionIlluminationDetected( const Json::Value& config )
: IBEICondition( config )
{
  // Parse trigger states
  std::vector<std::string> stateStrs;
  IlluminationState state;
  std::string infoStr = "[";
  if( JsonTools::GetVectorOptional( config, kTriggerStatesKey, stateStrs ) )
  {
    for( auto iter = stateStrs.begin(); iter != stateStrs.end(); ++iter )
    {
      if( !EnumFromString( *iter, state ) )
      {
        PRINT_NAMED_ERROR( "ConditionIlluminationDetected.Constuctor.InvalidState",
                          "Target state %s is not a valid IlluminationState", iter->c_str() );
      }
      else
      {
        _params.triggerStates.push_back( state );
        infoStr += *iter + ", ";
      }
    }
    infoStr += "]";
  }
  else
  {
    PRINT_NAMED_ERROR( "ConditionIlluminationDetected.Constructor.NoStates",
                       "No states specified" );
  }

  // Parse other parameters
  _params.confirmationTime_s = JsonTools::ParseFloat( config, "confirmationTime",
                                                      "ConditionIlluminationDetected.Constructor" );
  _params.confirmationMinNum = JsonTools::ParseUInt32( config, "confirmationMinNum",
                                                       "ConditionIlluminationDetected.Constructor" );
  _params.ignoreUnknown = JsonTools::ParseBool( config, "ignoreUnknown",
                                                "ConditionalIlluminationDetected.Constructor" );
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

  _variables.matchState = MatchState::WaitingForStart;
  _variables.matchStartTime = 0;
  _variables.matchedEvents = 0;
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

  if( _params.ignoreUnknown && IlluminationState::Unknown == obsState )
  {
    return;
  }

  switch( _variables.matchState )
  {
    case MatchState::WaitingForStart:
    {
      if( !IsTriggerState( obsState ) ) { break; }

      _variables.matchState = MatchState::ConfirmingMatch;
      _variables.matchStartTime = currTime;
      _variables.matchedEvents = 0;
      // Fall through to next state immediately
    }
    case MatchState::ConfirmingMatch:
    {
      if( !IsTriggerState( obsState ) )
      {
        Reset();
        break;
      }
   
      ++_variables.matchedEvents;
      if( !IsTimePassed( currTime, _params.confirmationTime_s ) || 
          _variables.matchedEvents < _params.confirmationMinNum )
      {
        break;
      }

      _variables.matchState = MatchState::MatchConfirmed;
      _variables.matchStartTime = currTime;
      // Fall through to next state immediately
    }
    case MatchState::MatchConfirmed:
    {
      if( !IsTriggerState( obsState ) )
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

  return Util::IsFltGT( f32(t - _variables.matchStartTime) / 1000.0f, dur );
}

bool ConditionIlluminationDetected::IsTriggerState( const IlluminationState& state ) const
{
  for( auto iter = _params.triggerStates.begin(); iter != _params.triggerStates.end(); ++iter )
  {
    if( *iter == state ) { return true; }
  }
  return false;
}

void ConditionIlluminationDetected::Reset()
{
  _variables.matchState = MatchState::WaitingForStart;
  _variables.matchedEvents = 0;
}

} // end namespace Vector
} // end namespace Anki
