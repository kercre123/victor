#ifndef ANIMPROCESS_COZMO_ALEXAALERTSMANAGER_H
#define ANIMPROCESS_COZMO_ALEXAALERTSMANAGER_H

#include "util/helpers/noncopyable.h"
//#include <chrono>
#include <unordered_map>
#include <map>
#include <functional>
#include <string>

namespace Json { class Value; }

namespace Anki {
namespace Vector {

  namespace RobotInterface {
    struct AlexaAlerts;
  }
  
  class AnimContext;
  namespace RobotInterface {
    struct AlexaAlerts;
  }
  
class AlexaAlertsManager : private Util::noncopyable
{
public:
  AlexaAlertsManager(const AnimContext* context, const std::function<void(RobotInterface::AlexaAlerts)>& callback);
  
  bool ProcessAlert( const std::string& name, const std::string& json );
  
  void CancelAlerts(const std::vector<int>& alertIDs);
  
private:
  
  void SaveToDisk() const;
  void LoadAlertJson(const Json::Value& json);
  void SendAlertsToEngine() const;
  
  enum class Type : uint8_t {
    Invalid=0,
    Alarm,
    Timer,
    // we dont do reminders
  };
  struct AlertInfo {
    Type type;
    //struct tm time;
    int engineToken;
    int timeSinceEpoch;
  };
  std::unordered_map<std::string, AlertInfo> _alerts;
  std::map<int, std::string> _engineTokenToAlexaToken;
  
  std::function<void(RobotInterface::AlexaAlerts)> _callback;
  
  static int _nextToken;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAALERTSMANAGER_H
