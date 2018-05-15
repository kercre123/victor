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

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

  
class WeatherIntentParser : private Util::noncopyable
{
public:
  static bool IsForecast(const UserIntent_WeatherResponse& weatherIntent);
  static bool ShouldSayText(const UserIntent_WeatherResponse& weatherIntent, 
                            std::string& textToSay);
  static bool IsFahrenheit(const UserIntent_WeatherResponse& weatherIntent);
  static WeatherConditionType GetCondition(const UserIntent_WeatherResponse& weatherIntent);

}; // class WeatherIntentParser

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_WeatherIntentParser_H__
