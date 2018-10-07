#include "cozmoAnim/alexaAlertManager.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include <memory>
#include <vector>
#include <iomanip>
#include "util/console/consoleInterface.h"
#include "osState/osState.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/robotDataLoader.h"
#include "json/json.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kSaveFile = "/data/data/com.anki.victor/persistent/alexa/vectorAlerts.json";
}
  
  
int AlexaAlertsManager::_nextToken = 1; // start at 1 since 0 is the stop char when sending clad
  
AlexaAlertsManager::AlexaAlertsManager(const AnimContext* context, const std::function<void(RobotInterface::AlexaAlerts)>& callback)
  : _callback(callback)
{
  // load existing
  if( Util::FileUtils::FileExists( kSaveFile ) ) {
    const auto* platform = context->GetDataPlatform();
    Json::Value alertJson;
    const bool success = platform->readAsJson(kSaveFile, alertJson);
    if( success ) {
      LoadAlertJson(alertJson);
    }
  }
}
  
bool AlexaAlertsManager::ProcessAlert( const std::string& name, const std::string& json )
{
  Json::Reader reader;
  Json::Value jsonAlert;
  bool success = reader.parse(json, jsonAlert);
  if( !success ) {
    return false;
  }
  if( name == "SetAlert" ) {
    if( !jsonAlert["type"].isString() || !jsonAlert["scheduledTime"].isString() || !jsonAlert["token"].isString() ) {
      return false;
    }
    if( jsonAlert["type"].asString() == "REMINDER" ) {
      return false; // we don't do reminders (they contain reminder text as metadata(!) but alexa's voice sounds better than vector's)
    }
    
//    if( jsonAlert["type"].asString() == "ALARM" ) {
//      // disabled for now, since the time zone is wrong, so we probably wouldnt use this for demo
//      return false;
//    }
    
    const std::string& inputTime = jsonAlert["scheduledTime"].asString();
    // e.g., "2018-10-06T02:54:16+0000"
    std::tm t;
    std::istringstream ss(inputTime);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S+0000");
    PRINT_NAMED_WARNING("WHATNOW", "Converted scheduledTime=%s", std::asctime(&t));
    
    const bool isAlarm = jsonAlert["type"].asString() == "ALARM";
    time_t scheduledTime = mktime(&t);
    int engineToken = _nextToken++;
    const std::string& token = jsonAlert["token"].asString();
    
    auto& alert = _alerts[token];
    alert.type = isAlarm ? Type::Alarm : Type::Timer;
    alert.engineToken = engineToken;
    alert.timeSinceEpoch = scheduledTime;
    
    _engineTokenToAlexaToken[engineToken] = token;
    
    SaveToDisk();
    SendAlertsToEngine();
    
    return true;
  } else if( name == "DeleteAlert" ) {
    if( !jsonAlert["token"].isString() ) {
      return false;
    }
    auto it = _alerts.find( jsonAlert["token"].asString() );
    if( it == _alerts.end() ) {
      return false;
    }
    const auto& info = it->second;
    _engineTokenToAlexaToken.erase(info.engineToken);
    _alerts.erase(it);
    SaveToDisk();
    SendAlertsToEngine();
    return true;
  }
  return false;
}
  
void AlexaAlertsManager::SaveToDisk() const
{
  Json::Value toSave = Json::arrayValue;
  
  std::stringstream ss("Current alerts: ");
  for( const auto& entry : _alerts ) {
    Json::Value jsonEntry;
    jsonEntry["token"] = entry.first;
    jsonEntry["timeSinceEpoch"] = entry.second.timeSinceEpoch;
    jsonEntry["isAlarm"] = entry.second.type == Type::Alarm;
    jsonEntry["engineToken"] = entry.second.engineToken;
    ss << "id=" << (int)entry.second.engineToken << " time=" << entry.second.timeSinceEpoch << " isAlarm=" << std::boolalpha << (entry.second.type == Type::Alarm) << ",";
    toSave.append(jsonEntry);
  }
  PRINT_NAMED_WARNING("WHATNOW", "AlexaAlertsManager: %s", ss.str().c_str());
  
  Util::FileUtils::WriteFile( kSaveFile, toSave.toStyledString() );
  
}

  
void AlexaAlertsManager::LoadAlertJson(const Json::Value& json)
{
  _alerts.clear();
  _engineTokenToAlexaToken.clear();
  
  
  // array of "token", "engineToken", "scheduledTime" (int, not same as alexa directive format) and "isAlarm"
  for( const auto& entry : json ) {
    auto token = entry["token"].asString();
    auto engineToken = entry["engineToken"].asInt();
    
    // todo: dont add if dupe (why are there dupes?)
    
    if( _nextToken <= engineToken ) {
      _nextToken = engineToken + 1;
    }
    
    auto& alert = _alerts[token];
    alert.type = entry["isAlarm"].asBool() ? Type::Alarm : Type::Timer;
    alert.engineToken = engineToken;
    alert.timeSinceEpoch = entry["timeSinceEpoch"].asInt();
    
    _engineTokenToAlexaToken[engineToken] = token;
  }
  PRINT_NAMED_WARNING("WHATNOW", "loading alert json %zu", _alerts.size());
  if( !_alerts.empty() ) {
    SendAlertsToEngine();
  }
}
  
void AlexaAlertsManager::SendAlertsToEngine() const
{
  if( _callback ) {
    RobotInterface::AlexaAlerts msg;
    static_assert(sizeof(msg.alertIDs) / sizeof(msg.alertIDs[0]) == 8, "");
    for( int i=0; i<8; ++i ) {
      msg.alertIDs[i] = 0;
    }
    std::stringstream ss("Loaded ");
    int i=0;
    for( const auto& p : _engineTokenToAlexaToken ) {
      auto it = _alerts.find(p.second);
      if( ANKI_VERIFY(it != _alerts.end(), "WHATNOW", "token %s not found (%d)", p.second.c_str(), p.first) ) {
        auto& alert = it->second;
        msg.alertIDs[i] = alert.engineToken;
        msg.isAlarm[i] = alert.type == Type::Alarm;
        msg.alertTimes[i] = alert.timeSinceEpoch;
        ss << "id=" << (int)msg.alertIDs[i] << " t=" << msg.alertTimes[i] << " isAlarm=" << std::boolalpha << msg.isAlarm[i] << ", ";
        ++i;
      } else {
        std::stringstream ss2;
        for( const auto& alert : _alerts) {
          ss2 << alert.first << ": " << alert.second.engineToken << ", ";
        }
        PRINT_NAMED_WARNING("WHATNOW", "_alerts=%s", ss2.str().c_str());
      }
      
    }
    PRINT_NAMED_WARNING("WHATNOW", "Sending engine %s", ss.str().c_str());
    _callback(msg);
  }
}
  
void AlexaAlertsManager::CancelAlerts(const std::vector<int>& alertIDs)
{
  bool needsSave = false;
  for( int alertID : alertIDs ) {
    auto it = _engineTokenToAlexaToken.find(alertID);
    if( it != _engineTokenToAlexaToken.end() ) {
      const auto& token = it->second;
      _alerts.erase(token);
      needsSave = true;
    } else {
      ANKI_VERIFY( false && "engine token not found", "", "");
    }
  }
  
  if( needsSave ) {
    SaveToDisk();
  }
}
  
} // namespace Vector
} // namespace Anki
