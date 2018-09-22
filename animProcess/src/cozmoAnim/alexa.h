#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include "util/logging/logging.h"

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Bluetooth/SQLiteBluetoothStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/SQLiteSettingStorage.h>

#include <memory>
#include <vector>
#include <iostream>

namespace Anki {
namespace Vector {

inline bool AlexaTest()
{
  using namespace alexaClientSDK;
  std::vector<std::shared_ptr<std::istream>> configJsonStreams;
  
  // for (auto configFile : configFiles) {
  //   if (configFile.empty()) {
  //     alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Config filename is empty!");
  //     return false;
  //   }
  
  //   auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
  //   if (!configInFile->good()) {
  //     ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
  //     alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file " + configFile);
  //     return false;
  //   }
  
  //   configJsonStreams.push_back(configInFile);
  // }
  PRINT_NAMED_WARNING("WHATNOW", "calling");
  
  #define LX(event) avsCommon::utils::logger::LogEntry(__FILE__, event)
  
  if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize(configJsonStreams)) {
    //ACSDK_CRITICAL(LX("Failed to initialize SDK!"));
    PRINT_NAMED_WARNING("WHATNOW", "failed to init");
    return false;
  }
  PRINT_NAMED_WARNING("WHATNOW", "worked");
  return true;
}


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
