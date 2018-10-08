#ifndef ANIMPROCESS_COZMO_ALEXAWEATHERPARSER_H
#define ANIMPROCESS_COZMO_ALEXAWEATHERPARSER_H

#include "util/helpers/noncopyable.h"
//#include <chrono>
#include <unordered_map>
#include <map>
#include <functional>
#include <string>

namespace Json { class Value; }

namespace Anki {
namespace Vector {
  
  class AnimContext;
  namespace RobotInterface {
    struct AlexaWeather;
  }
  
class AlexaWeatherParser// : private Util::noncopyable
{
public:
  AlexaWeatherParser(const AnimContext* context, const std::function<void(RobotInterface::AlexaWeather&&)>& callback);
  
  bool Parse(const std::string& json);
  
private:
  
  std::function<void(RobotInterface::AlexaWeather&&)> _callback;
  
  static int _nextToken;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAWEATHERPARSER_H
