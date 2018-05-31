/**
 * File: conditionPersonDetected.cpp
 *
 * Author: Lorenzo Riano
 * Created: 5/31/18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionPersonDetected.h"

Anki::Cozmo::ConditionPersonDetected::ConditionPersonDetected(const Json::Value& config)
    : IBEICondition(config)
{

}

Anki::Cozmo::ConditionPersonDetected::~ConditionPersonDetected()
{

}

void
Anki::Cozmo::ConditionPersonDetected::InitInternal(Anki::Cozmo::BehaviorExternalInterface& behaviorExternalInterface)
{
  IBEICondition::InitInternal(behaviorExternalInterface);
}

bool Anki::Cozmo::ConditionPersonDetected::AreConditionsMetInternal(
    Anki::Cozmo::BehaviorExternalInterface& behaviorExternalInterface) const
{
  return false;
}

void
Anki::Cozmo::ConditionPersonDetected::GetRequiredVisionModes(std::set<Anki::Cozmo::VisionModeRequest>& requests) const
{
  IBEICondition::GetRequiredVisionModes(requests);
  //TODO we need person detection mode here
//    requiredVisionModes.insert( {VisionMode::DetectingMotion, EVisionUpdateFrequency::High} );
}

void Anki::Cozmo::ConditionPersonDetected::HandleEvent(const Anki::Cozmo::EngineToGameEvent& event,
                                                       Anki::Cozmo::BehaviorExternalInterface& bei)
{
  IBEIConditionEventHandler::HandleEvent(event, bei);
}


