#include "cozmoAnim/alexaWeatherParser.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

//#include <memory>
//#include <vector>
#include <iomanip>
#include <time.h>
//#include "util/console/consoleInterface.h"
//#include "osState/osState.h"
//#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
//#include "cozmoAnim/robotDataLoader.h"
#include "json/json.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Vector {
  
namespace {
  struct ForecastInfo {
    int high;
    int low;
    std::string description;
    std::tm date;
  };
}
  
std::tm GetCurrentDate()
{
  time_t current_time;
  struct tm  local_time;
  
  time ( &current_time );
  localtime_r(&current_time, &local_time);
  return local_time;
  
}
std::tm ParseFullDate( const std::string& dateStr )
{
  // e.g., "Monday, October 8, 2018"
  std::tm t;
  // tack on a time
  Util::EpochFromDateString(dateStr + " 12:00:00", "%a, %b %d, %Y %H:%m:%s", t);
//  std::istringstream ss(dateStr);
//  ss >> std::get_time(&t, "%a, %b %d, %Y");
  PRINT_NAMED_WARNING("WHATNOW", "Converted full date '%s' to '%s'", dateStr.c_str(), std::asctime(&t));
  return t;
}

std::tm ParseShortDay( const std::string& dayOfWeek, const std::string& monthAndDay )
{
  std::string input = dayOfWeek + " " + monthAndDay;
  // e.g., "Mon Oct 7"
  std::tm t;
  std::istringstream ss(input);
  ss >> std::get_time(&t, "%a %b %d");
  PRINT_NAMED_WARNING("WHATNOW", "Converted short date=%s", std::asctime(&t));
  return t;
}

bool ParseInt(const std::string& str, int& out)
{
  std::stringstream ss(str);
  int i;
  ss >> i;
  
  if( ss.fail() ) {
    return false;
  } else {
    out = i;
    return true;
  }
}

bool ParseTemp(const std::string& str, int& temp)
{
  std::string numeric;
  for( char c : str) {
    if( !std::isdigit(c) ) {
      break;
    }
    numeric += c;
  }
  if( !numeric.empty() ) {
    return ParseInt(numeric, temp);
  } else {
    return false;
  }
}
  
//  bool ParseForecastEntry( const Json::Value& json, ForecastInfo& info )
//  {
//    //"date" : "Oct 7",
//    //"day" : "Sun",
//    //"highTemperature" : "76Â°",
//    //"lowTemperature" : "60"Â°",
//    //"image" : {
//    //  "contentDescription" : "Partly cloudy",
//    if( !json["date"].isString() ) {
//      return false;
//    }
//    if( !json["day"].isString() ) {
//      return false;
//    }
//    if( !json["highTemperature"].isString() ) {
//      return false;
//    }
//    if( !json["lowTemperature"].isString() ) {
//      return false;
//    }
//    if( json["image"].isNull() ) {
//      return false;
//    }
//    if( !json["image"]["contentDescription"].isString() ) {
//      return false;
//    }
//
//
//    info.date = ParseShortDay(json["day"].asString(), json["date"].asString());
//    info.description = json["image"]["contentDescription"].asString();
//    if( !ParseTemp( json["highTemperature"].asString(), info.high ) ) {
//      return false;
//    }
//    if( !ParseTemp( json["lowTemperature"].asString(), info.low ) ) {
//      return false;
//    }
//
//    return true;
//  }
  
AlexaWeatherParser::AlexaWeatherParser(const AnimContext* context, const std::function<void(RobotInterface::AlexaWeather&&)>& callback)
  : _callback(callback)
{
  
}
  
bool AlexaWeatherParser::Parse(const std::string& json )
{
  Json::Reader reader;
  Json::Value jsonWeather;
  #define FAIL(d) PRINT_NAMED_WARNING("WHATNOW","Failed because %d", d);
  bool success = reader.parse(json, jsonWeather);
  if( !success ) {
    FAIL(-1);
    return false;
  }
  
  
  if( jsonWeather["title"].isNull() ) {
    FAIL(0);
    return false;
  }
  if( !jsonWeather["title"]["subTitle"].isString() ) {
    FAIL(1);
    return false;
  }
//  if( !jsonWeather["weatherForecast"].isArray() ) {
//    return false;
//  }
  
  const std::tm requestedDate = ParseFullDate(  jsonWeather["title"]["subTitle"].asString() );
  
  const std::tm localDate = GetCurrentDate();
  
  if( !jsonWeather["currentWeather"].isString() ) {
    FAIL(2);
    return false;
  }
  if( jsonWeather["highTemperature"].isNull() ) {
    FAIL(3);
    return false;
  }
  if( jsonWeather["lowTemperature"].isNull() ) {
    FAIL(4);
    return false;
  }
  if( !jsonWeather["highTemperature"]["value"].isString() ) {
    
    FAIL(5);
    return false;
  }
  if( !jsonWeather["lowTemperature"]["value"].isString() ) {
    FAIL(6);
    return false;
  }
  if( jsonWeather["currentWeatherIcon"].isNull() ) {
    FAIL(7);
    return false;
  }
  if( !jsonWeather["currentWeatherIcon"]["contentDescription"].isString() ) {
    FAIL(8);
    return false;
  }
  
  int currentTemp=0;
  int highTemp=0;
  int lowTemp=0;
  if( !ParseTemp(jsonWeather["currentWeather"].asString(), currentTemp) ) {
    FAIL(9);
    return false;
  }
  if( !ParseTemp(jsonWeather["highTemperature"]["value"].asString(), highTemp) ) {
    FAIL(10);
    return false;
  }
  if( !ParseTemp(jsonWeather["lowTemperature"]["value"].asString(), lowTemp) ) {
    FAIL(11);
    return false;
  }
  // there's also "description", which looks to be a long accuweather string, e.g.:
  //     Periods of sun, a shower or thunderstorm; humid; watch for rough surf and rip currents
  std::string description = jsonWeather["currentWeatherIcon"]["contentDescription"].asString();
  int temp = 0;
  int hour = 0;
  PRINT_NAMED_WARNING("WHATNOW", "localDateyday = %d, requested=%d. current=%d, high=%d, low=%d", localDate.tm_mday, requestedDate.tm_mday, currentTemp, highTemp, lowTemp);
  if( localDate.tm_mday == requestedDate.tm_mday ) {
    // requested todays weather
    temp = currentTemp;
    hour = localDate.tm_hour; // current time
  } else {
    // requested forecast. average high and low
    temp = (lowTemp + highTemp) / 2;
    hour = 12; // noon
  }
  
  PRINT_NAMED_WARNING("WHATNOW", "Parsed weather temp=%d, hour=%d, desc=%s (sending=%d)", temp, requestedDate.tm_hour, description.c_str(), (int)(bool)_callback);
  if( _callback ) {
    RobotInterface::AlexaWeather msg;
    msg.temperature = temp;
    msg.hour = hour;
    const int len = std::min(description.length(), (size_t)256);
    memcpy( msg.condition, description.c_str(), len );
    msg.condition_length = len;
    _callback(std::move(msg));
  }
  
//  int cnt=0;
//  for( const auto& entry : jsonWeather["weatherForecast"] ) {
//    ForecastInfo forecast;
//    if( !ParseForecastEntry( entry, forecast ) ) {
//      return false;
//    }
//    if( cnt == 0 ) {
//      if( forecast.date.tm_yday > requestedDate.tm_yday ) {
//        // first entry is after current weather: requested current weather
//      } else if( forecast.date.tm_yday < requestedDate.tm_yday )
//    }
//    if( forecast.date.tm_yday == requestedDate.tm_yday ) {
//
//    }
//    ++cnt;
//  }
  
  return true;
}
  
} // namespace Vector
} // namespace Anki
