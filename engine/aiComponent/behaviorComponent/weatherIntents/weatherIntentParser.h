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

#ifndef __Engine_AiComponent_BehaviorComponent_WeatherIntentParser_H__
#define __Engine_AiComponent_BehaviorComponent_WeatherIntentParser_H__

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/behaviorComponent/weatherConditionTypes.h"

#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherConditionRemaps.h"
#include "engine/robotDataLoader.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Vector {

  
class WeatherIntentParser : private Util::noncopyable
{
public:
  WeatherIntentParser(const RobotDataLoader::WeatherResponseMap* weatherResponseMap,
                      const Json::Value& conditionRemaps);

  bool IsForecast(const UserIntent_WeatherResponse& weatherIntent) const;
  bool ShouldSayText(const UserIntent_WeatherResponse& weatherIntent, 
                     std::string& textToSay) const;
  bool IsFahrenheit(const UserIntent_WeatherResponse& weatherIntent) const;
  WeatherConditionType GetCondition(const UserIntent_WeatherResponse& weatherIntent,
                                    bool isForPRDemo = false) const;
  tm GetLocalDateTime(const UserIntent_WeatherResponse& weatherIntent) const;
  bool GetTemperature(const UserIntent_WeatherResponse& weatherIntent,
                      int& outTemp) const;


private:
  class ConditionRemaps{
  public:
    ConditionRemaps(const Json::Value& conditionRemaps);
  
  private:
    
  };

  const RobotDataLoader::WeatherResponseMap* _weatherResponseMap;
  const WeatherConditionRemaps _conditionRemaps;


}; // class WeatherIntentParser

} // namespace Vector
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_WeatherIntentParser_H__
