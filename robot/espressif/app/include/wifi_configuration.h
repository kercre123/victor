/**
 * @file WiFi Configuration module for the Espressif
 * @author Daniel Casner
 * @date 2016-03-09
 * @copyright Anki, Inc. 2015
 **/


#ifndef COZMO_WIFI_CONFIGURATION_H_
#define COZMO_WIFI_CONFIGURATION_H_

#include "anki/types.h"
#include "clad/robotInterface/appConnectMessage.h"

namespace Anki {
  namespace Cozmo {
    namespace WiFiConfiguration {
      
      extern uint8_t sessionToken[16];
      
      // Sets up data structures, call before any other methods
      Result Init();
      
      // Turn the wifi off
      bool Off(const bool sleep);
      
      void SendRobotIpInfo(const u8 ifId);
      
      void ProcessConfigString (const RobotInterface::AppConnectConfigString& msg);
      void ProcessConfigFlags  (const RobotInterface::AppConnectConfigFlags&  msg);
      void ProcessConfigIPInfo (const RobotInterface::AppConnectConfigIPInfo& msg);
    }
  } // Cozmo
} // Anki

#endif
