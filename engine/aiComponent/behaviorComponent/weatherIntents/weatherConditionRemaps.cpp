/**
* File: weatherConditionRemaps.cpp
*
* Author: Kevin Karol
* Date:   7/23/2018
*
* Description: Weather conditions can be re-mapped based on additional data in the
* weather response
*
* Copyright: Anki, Inc. 2018
**/


#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherConditionRemaps.h"

#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kInitialTypesKey    = "InitialCladTypes";
const char* kRemapConditionsKey = "RemapConditions";
const char* kOutputConditionKey = "OutputCondition";

// remap conditions
const char* kConditionTypeKey = "conditionType";
const char* kOperatorKey      = "operator";
const char* kAndOperator      = "and";

const char* kTimeCondition   = "time";
const char* kLocalTimeBeforeKey = "LocalTimeBefore";
const char* kLocalTimeAfterKey  = "LocalTimeAfter";

const char* kTemperatureCondition = "temperature";
const char* kTempBelowFarKey = "TemperatureBelowFarenheit";
const char* kTempAboveFarKey = "TemperatureAboveFarenheit";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionRemaps::WeatherConditionRemaps(const Json::Value& conditionRemaps)
{
  for(const auto& config: conditionRemaps){
    RemapEntry entry;
    entry.remappedType = WeatherConditionTypeFromString(config[kOutputConditionKey].asString());

    const std::string debugName = "WeatherConditionRemaps.ConditionParsing";
    for(const auto& conditionSpec : config[kRemapConditionsKey]){
      if(conditionSpec[kConditionTypeKey] == kTimeCondition){
        if(conditionSpec.isMember(kLocalTimeBeforeKey)){
          entry.localTimeBefore = std::make_unique<tm>();
          strptime(conditionSpec[kLocalTimeBeforeKey].asString().c_str(), "%H:%M", entry.localTimeBefore.get());
        }
        if(conditionSpec.isMember(kLocalTimeAfterKey)){
          entry.localTimeAfter = std::make_unique<tm>();
          strptime(conditionSpec[kLocalTimeAfterKey].asString().c_str(), "%H:%M", entry.localTimeAfter.get());
        }
      }else if(conditionSpec[kConditionTypeKey] == kTemperatureCondition){
        if(conditionSpec.isMember(kTempBelowFarKey)){
          entry.temperatureBelow = JsonTools::ParseInt32(conditionSpec, kTempBelowFarKey, debugName);
        }
        if(conditionSpec.isMember(kTempAboveFarKey)){
          entry.temperatureAbove = JsonTools::ParseInt32(conditionSpec, kTempAboveFarKey, debugName);
        }
      }

      // If there's more than one condition specified we need to know what operator to apply to their results (and/or)
      const auto& names = conditionSpec.getMemberNames();
      if(names.size() > 2){
        if(ANKI_VERIFY(conditionSpec.isMember(kOperatorKey), 
                       "WeatherConditionRemaps.Constructor.MissingRemapOpreator",
                       "Multiple conditions specified, but no operator found in config")){
          entry.allSpecifiedConditionsMustBeMet = (conditionSpec[kOperatorKey] == kAndOperator);
        }
      }
    }


    // Introduce an instance into the map for every initial clad type
    for(const auto& initialType : config[kInitialTypesKey]){
      auto type = WeatherConditionTypeFromString(initialType.asString());
      entry.initialType = type;
      auto iter = _remaps.find(type);
      if(iter == _remaps.end()){
        std::vector<RemapEntry> container;
        container.push_back(entry);
        auto pair = std::make_pair<WeatherConditionType, std::vector<RemapEntry>>(std::move(type), std::move(container));
        _remaps.emplace(pair);
      }else{
        iter->second.push_back(entry);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionType WeatherConditionRemaps::GetRemappedCondition(const WeatherIntentParser& parser,
                                                                  const UserIntent_WeatherResponse& weatherIntent,
                                                                  const WeatherConditionType initialCondition) const
{
  const auto iter = _remaps.find(initialCondition);
  if(iter != _remaps.end()){
    const tm localDatetime = parser.GetLocalDateTime(weatherIntent);

    int trueTemperature = 0;
    parser.GetTemperature(weatherIntent, trueTemperature);

    for(const auto& entry : iter->second){
      const bool shouldConsiderTemperature = (entry.temperatureBelow != kInvalidTemp) || (entry.temperatureAbove != kInvalidTemp);

      const bool isBelowConfigTemp = ((entry.temperatureBelow != kInvalidTemp) && (entry.temperatureBelow > trueTemperature));
      const bool isAboveConfigTemp = ((entry.temperatureAbove != kInvalidTemp) && (entry.temperatureAbove < trueTemperature));

      const bool inTemperatureRange = entry.allSpecifiedConditionsMustBeMet ? 
                                        isBelowConfigTemp && isAboveConfigTemp : 
                                        isBelowConfigTemp || isAboveConfigTemp;

      const bool shouldConsiderTime = (entry.localTimeBefore != nullptr) || (entry.localTimeAfter != nullptr);
      const bool inTimeBefore = (entry.localTimeBefore != nullptr) &&
                                  (entry.localTimeBefore->tm_hour >= localDatetime.tm_hour) &&
                                    ((entry.localTimeBefore->tm_hour > localDatetime.tm_hour) ||
                                     (entry.localTimeBefore->tm_min  > localDatetime.tm_min));
      const bool inTimeAfter = (entry.localTimeAfter != nullptr) &&
                                 (entry.localTimeAfter->tm_hour <= localDatetime.tm_hour) &&
                                   ((entry.localTimeAfter->tm_hour < localDatetime.tm_hour) ||
                                    (entry.localTimeAfter->tm_min < localDatetime.tm_min));

      
      const bool inTimeRange = entry.allSpecifiedConditionsMustBeMet ? 
                                 inTimeBefore && inTimeAfter : 
                                 inTimeBefore || inTimeAfter;     


      if((!shouldConsiderTemperature || inTemperatureRange) &&
         (!shouldConsiderTime || inTimeRange)){
        return entry.remappedType;
      }
    }
  }

  return initialCondition;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionRemaps::RemapEntry::RemapEntry(const RemapEntry& other)
{
  initialType      = other.initialType;
  remappedType     = other.remappedType;
  temperatureBelow = other.temperatureBelow;
  temperatureAbove = other.temperatureAbove;
  localTimeBefore  = (other.localTimeBefore == nullptr) ? nullptr : std::make_unique<tm>(*other.localTimeBefore.get());
  localTimeAfter   = (other.localTimeAfter == nullptr)  ? nullptr : std::make_unique<tm>(*other.localTimeAfter.get());
}


} // namespace Cozmo
} // namespace Anki
