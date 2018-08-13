/**
 * File: conditionEngineErrorCodeReceived.cpp
 * 
 * Author: Arjun Menon
 * Created: August 08 2018
 *
 * Description: 
 * Condition that checks the image quality based on the given type expected
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionEngineErrorCodeReceived.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "util/logging/logging.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kEngineErrorCodeKey = "engineErrorCode";
}

ConditionEngineErrorCodeReceived::ConditionEngineErrorCodeReceived( const Json::Value& config )
: IBEICondition( config )
{
  _iConfig.engineErrorCodeToCheck = EngineErrorCodeFromString(JsonTools::ParseString(config, kEngineErrorCodeKey, "ConditionalEngineErrorCodeReceived.Constructor"));
}

ConditionEngineErrorCodeReceived::~ConditionEngineErrorCodeReceived()
{
}

ConditionEngineErrorCodeReceived::InstanceConfig::InstanceConfig()
{
  engineErrorCodeToCheck = EngineErrorCode::Count;
}

ConditionEngineErrorCodeReceived::DynamicVariables::DynamicVariables()
{
  engineErrorCodeMatches = false;
}

void ConditionEngineErrorCodeReceived::GetRequiredVisionModes(std::set<VisionModeRequest>& request) const
{
  request.insert({ VisionMode::CheckingQuality, EVisionUpdateFrequency::High });
}

void ConditionEngineErrorCodeReceived::InitInternal( BehaviorExternalInterface& bei )
{
  _messageHelper.reset( new BEIConditionMessageHelper( this, bei ) );
  _messageHelper->SubscribeToTags( {EngineToGameTag::EngineErrorCodeMessage} );

  _dVars = DynamicVariables();
}

bool ConditionEngineErrorCodeReceived::AreConditionsMetInternal(BehaviorExternalInterface& bei ) const
{
  return _dVars.engineErrorCodeMatches;
}


void ConditionEngineErrorCodeReceived::HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei )
{
  switch( event.GetData().GetTag() )
  {
    case ExternalInterface::MessageEngineToGameTag::EngineErrorCodeMessage:
    {
      const auto& msg = event.GetData().Get_EngineErrorCodeMessage();  
      _dVars.engineErrorCodeMatches = msg.errorCode == _iConfig.engineErrorCodeToCheck;
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR( "ConditionEngineErrorCodeReceived.HandleEvent.InvalidEvent", "" );
    }
  }
}

}
}
