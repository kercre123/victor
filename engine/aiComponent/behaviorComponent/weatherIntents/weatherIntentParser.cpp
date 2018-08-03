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

#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include <exception>
#include <iomanip>

namespace Anki {
namespace Cozmo {

namespace {
const std::string kWeatherLocationPrepend = "Right now in ";
std::string kCityWhereItAlwaysRainsMutable = "Seattle";

#if REMOTE_CONSOLE_ENABLED
void SetRainyCity(ConsoleFunctionContextRef context)
{
  kCityWhereItAlwaysRainsMutable = ConsoleArg_Get_String(context, "rainyCity");
}

CONSOLE_FUNC(SetRainyCity, "WeatherHack", const char* rainyCity);
#endif

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherIntentParser::WeatherIntentParser(const RobotDataLoader::WeatherResponseMap* weatherResponseMap,
                                         const Json::Value& conditionRemaps)
: _weatherResponseMap(weatherResponseMap)
, _conditionRemaps(conditionRemaps)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::IsForecast(const UserIntent_WeatherResponse& weatherIntent) const
{
  return (weatherIntent.isForecast == "true") ||
         (weatherIntent.isForecast == "True");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::ShouldSayText(const UserIntent_WeatherResponse& weatherIntent,
                                        std::string& textToSay) const
{
  textToSay = kWeatherLocationPrepend + weatherIntent.speakableLocationString;
  return !weatherIntent.speakableLocationString.empty();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::IsFahrenheit(const UserIntent_WeatherResponse& weatherIntent) const
{
  return weatherIntent.temperatureUnit == "F" ||
         weatherIntent.temperatureUnit == "f";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeatherConditionType WeatherIntentParser::GetCondition(const UserIntent_WeatherResponse& weatherIntent,
                                                       bool isForPRDemo) const
{
  if(isForPRDemo && (weatherIntent.speakableLocationString == kCityWhereItAlwaysRainsMutable)){
    return WeatherConditionType::Rain;
  }

  std::string str;
  std::transform(weatherIntent.condition.begin(), weatherIntent.condition.end(),
                 std::back_inserter(str), [](const char c) { return std::tolower(c); });

  auto iter = _weatherResponseMap->find(str);
  if(iter != _weatherResponseMap->end()){
    return _conditionRemaps.GetRemappedCondition(*this, weatherIntent, iter->second);
  }else{
    PRINT_NAMED_ERROR("WeatherIntentParser.GetCondition.NoConditionMatch",
                      "No weather condition found for %s",
                      str.c_str());
    return WeatherConditionType::Count;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
tm WeatherIntentParser::GetLocalDateTime(const UserIntent_WeatherResponse& weatherIntent) const
{
  tm localTime;
  strptime(weatherIntent.localDateTime.c_str(), "%Y-%m-%dT%H:%M%S-", &localTime);
  return localTime;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WeatherIntentParser::GetTemperature(const UserIntent_WeatherResponse& weatherIntent,
                                         int& outTemp) const
{
  if(weatherIntent.temperature.empty()){
    return false;
  }
  try{
    outTemp = std::stoi(weatherIntent.temperature);
  }catch(const std::invalid_argument& ia){
    return false;
  }
  return true;
}


} // namespace Cozmo
} // namespace Anki
