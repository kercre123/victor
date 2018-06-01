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

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIlluminationDetected.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

ConditionIlluminationDetected::ConditionIlluminationDetected( const Json::Value& config )
: IBEICondition( config )
{
  std::string targetStateString = JsonTools::ParseString( config, "targetState", 
                                                          "ConditionIlluminationDetected.Constructor" );
  if( !EnumFromString( targetStateString, _params.targetState ) )
  {
    PRINT_NAMED_ERROR( "ConditionIlluminationDetected.Constructor.InvalidState",
                      "Target state %s is not a valid IlluminationState", targetStateString.c_str() );
    // TODO: Bail or invalidate object somehow?
  }

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

  _matchState = MatchState::WaitingForMatch;
  _matchStartTime = 0;
  _matchedEvents = 0;
}

bool ConditionIlluminationDetected::AreConditionsMetInternal( BehaviorExternalInterface& bei ) const
{
  return MatchState::MatchConfirmed == _matchState;
}

void ConditionIlluminationDetected::HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei )
{
  switch( event.GetData().GetTag() )
  {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedIllumination:
    {
      HandleIllumination( event );
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR( "ConditionIlluminationDetected.HandleEvent.InvalidEvent", "" );
    }
  }
}

void ConditionIlluminationDetected::HandleIllumination( const EngineToGameEvent& event )
{
  const auto& msg = event.GetData().Get_RobotObservedIllumination();

  if( _params.ignoreUnknown && IlluminationState::Unknown == msg.state )
  {
    return;
  }

  switch( _matchState )
  {
    case MatchState::WaitingForMatch:
    {
      PRINT_NAMED_INFO("ConditionIlluminationDetected.HandleIllumination.Waiting", "%s",
                       EnumToString(_params.targetState) );
      if( msg.state == _params.targetState )
      {
        _matchState = MatchState::ConfirmingMatch;
        _matchStartTime = msg.timestamp;
        _matchedEvents = 0;
      }
      break;
    }
    case MatchState::ConfirmingMatch:
    {
      if( msg.state != _params.targetState )
      {
        _matchState = MatchState::WaitingForMatch;
      }
      else
      {
        PRINT_NAMED_INFO("ConditionIlluminationDetected.HandleIllumination.Confirming", "%s",
                         EnumToString(_params.targetState) );
        if( msg.timestamp < _matchStartTime )
        {
          PRINT_NAMED_ERROR("ConditionIlluminationDetected.HandleIllumination.TimeError",
                            "Time has gone backwards!");
          _matchState = MatchState::WaitingForMatch;
          break;
        }

        ++_matchedEvents;
        bool enoughTime = Util::IsFltGT( (msg.timestamp - _matchStartTime) / 1000.f, _params.confirmationTime_s );
        bool enoughEvents = _matchedEvents >= _params.confirmationMinNum;
        if( enoughTime && enoughEvents )
        {
          _matchState = MatchState::MatchConfirmed;
        }
      }
      break;
    }
    case MatchState::MatchConfirmed:
    {
      PRINT_NAMED_INFO("ConditionIlluminationDetected.HandleIllumination.Matched", "%s",
                       EnumToString(_params.targetState) );
      if( msg.state != _params.targetState )
      {
        _matchState = MatchState::WaitingForMatch;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("ConditionIlluminationDetected.HandleIllumination.InvalidState", "");
      _matchState = MatchState::WaitingForMatch;
    }
  }
}

}
}