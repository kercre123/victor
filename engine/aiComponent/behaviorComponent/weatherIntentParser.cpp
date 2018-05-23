/**
* File: weatherIntentParser.h
*
* Author: Kevin Karol
* Date:   5/10/2018
*
* Description: Translates the contents of a weather response user intent into terms that
* can be used for decision making/response internally
*
* Copyright: Anki, Inc. 2018
**/

#include "engine/aiComponent/behaviorComponent/weatherIntentParser.h"

#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include <iomanip>

namespace Anki {
namespace Cozmo {

namespace{
const std::string kWeatherLocationPrepend = "Right now in ";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::IsForecast(const UserIntent_WeatherResponse& weatherIntent) 
{ 
  return (weatherIntent.isForecast == "true") || 
         (weatherIntent.isForecast == "True");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::ShouldSayText(const UserIntent_WeatherResponse& weatherIntent, 
                                        std::string& textToSay) 
{ 
  textToSay = kWeatherLocationPrepend + weatherIntent.speakableLocationString;
  return !weatherIntent.speakableLocationString.empty();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::IsFahrenheit(const UserIntent_WeatherResponse& weatherIntent) 
{ 
  return weatherIntent.temperatureUnit == "F" || 
         weatherIntent.temperatureUnit == "f";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionType WeatherIntentParser::GetCondition(const RobotDataLoader::WeatherResponseMap* weatherResponseMap,
                                                       const UserIntent_WeatherResponse& weatherIntent) 
{

  std::string str;
  std::transform(weatherIntent.condition.begin(), weatherIntent.condition.end(), 
                 std::back_inserter(str), [](const char c) { return std::tolower(c); });

  auto iter = weatherResponseMap->find(str);
  if(iter != weatherResponseMap->end()){
    return iter->second;
  }else{
    PRINT_NAMED_ERROR("WeatherIntentParser.GetCondition.NoConditionMatch",
                      "No weather condition found for %s",
                      str.c_str());
    return WeatherConditionType::Count;
  }
}

} // namespace Cozmo
} // namespace Anki
