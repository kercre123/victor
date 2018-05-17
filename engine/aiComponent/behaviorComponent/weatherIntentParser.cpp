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
  textToSay = weatherIntent.speakableLocationString;
  return !weatherIntent.speakableLocationString.empty();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::IsFahrenheit(const UserIntent_WeatherResponse& weatherIntent) 
{ 
  return weatherIntent.temperatureUnit == "F" || 
         weatherIntent.temperatureUnit == "f";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionType WeatherIntentParser::GetCondition(const UserIntent_WeatherResponse& weatherIntent) 
{

  std::string str;
  std::transform(weatherIntent.condition.begin(), weatherIntent.condition.end(), 
                 std::back_inserter(str), [](const char c) { return std::tolower(c); });

  if(str == "cloudy"){
    return WeatherConditionType::Cloudy;
  }else if(str == "cold"){
    return WeatherConditionType::Cold;
  }else if(str == "rain"){
    return WeatherConditionType::Rain;
  }else if(str == "snow"){
    return WeatherConditionType::Snow;
  }else if(str == "stars"){
    return WeatherConditionType::Stars;
  }else if(str == "sunny"){
    return WeatherConditionType::Sunny;
  }else if(str == "windy"){
    return WeatherConditionType::Windy;
  }else{
    PRINT_NAMED_ERROR("WeatherIntentParser.GetCondition.NoConditionMatch",
                      "No weather condition found for %s",
                      str.c_str());
  }

  return WeatherConditionType::Count;
}

} // namespace Cozmo
} // namespace Anki
