/**
* File: weatherConditionRemaps.h
*
* Author: Kevin Karol
* Date:   7/23/2018
*
* Description: Weather conditions can be re-mapped based on additional data in the
* weather response
*
* Copyright: Anki, Inc. 2018
**/

#ifndef __Engine_AiComponent_BehaviorComponent_WeatherConditionRemaps_H__
#define __Engine_AiComponent_BehaviorComponent_WeatherConditionRemaps_H__

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/behaviorComponent/weatherConditionTypes.h"

#include "engine/robotDataLoader.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Vector {

// forward declaration
class WeatherIntentParser;

class WeatherConditionRemaps : private Util::noncopyable{
public:
  WeatherConditionRemaps(const Json::Value& conditionRemaps);

  WeatherConditionType GetRemappedCondition(const WeatherIntentParser& parser,
                                            const UserIntent_WeatherResponse& weatherIntent,
                                            const WeatherConditionType initialCondition) const;

private:
  static const int kInvalidTemp = std::numeric_limits<int>::max();
  struct RemapEntry{
    RemapEntry(){};
    RemapEntry(const RemapEntry& other);
    WeatherConditionType initialType;
    WeatherConditionType remappedType;

    // Temperature related remaps
    float temperatureBelowF = kInvalidTemp;
    float temperatureAboveF = kInvalidTemp;

    // Time related remaps
    std::unique_ptr<tm> localTimeBefore;
    std::unique_ptr<tm> localTimeAfter;
 

    // and vs or
    bool allSpecifiedConditionsMustBeMet = false;
  };

  std::unordered_map<WeatherConditionType, std::vector<RemapEntry>, Anki::Util::EnumHasher> _remaps;


};

} // namespace Vector
} // namespace Anki


#endif // __Engine_AiComponent_BehaviorComponent_WeatherConditionRemaps_H__
