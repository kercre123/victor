/**
 * File: wifiWatcher.cpp
 *
 * Author: paluri
 * Created: 8/28/2018
 *
 * Description: A watchdog to ensure robot is connected to robot (if possible) and to
 * try to connect if it is not.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "log.h"
#include "wifiWatcher.h"

namespace Anki {
namespace Switchboard {

WifiWatcher::WifiWatcher(struct ev_loop* loop)
: _loop(loop) {
  ev_timer_init(&_timer.timer, WatcherTick, 0, kWifiTick_s);
  ev_timer_start(_loop, &_timer.timer);
}

WifiWatcher::~WifiWatcher() {
  ev_timer_stop(_loop, &_timer.timer);
}

void WifiWatcher::ConnectIfNoWifi() {
  Anki::Wifi::WiFiState wifiState = Anki::Wifi::GetWiFiState();

  if((wifiState.connState == Anki::Wifi::WiFiConnState::CONNECTED) ||
    (wifiState.connState == Anki::Wifi::WiFiConnState::ONLINE)) {
    // robot is on wifi, so carry on!
    return;
  }

  Log::Write("WifiWatcher: detected no wifi. Scanning for networks...");

  std::vector<Anki::Wifi::WiFiScanResult> results;
  Anki::Wifi::WifiScanErrorCode code = Anki::Wifi::ScanForWiFiAccessPoints(results);

  if(code != Anki::Wifi::WifiScanErrorCode::SUCCESS) {
    // can't scan for wifi
    return;
  }

  for(const Anki::Wifi::WiFiScanResult& result : results) {
    if(result.provisioned) {
      Anki::Wifi::ConnectWifiResult connectResult = 
        Anki::Wifi::ConnectWiFiBySsid(result.ssid, 
                                      "", // don't need pw since already provisioned
                                      (uint8_t)result.auth,
                                      result.hidden,
                                      nullptr,
                                      nullptr);

      if(connectResult == Anki::Wifi::ConnectWifiResult::CONNECT_SUCCESS) {
        Log::Write("WifiWatcher: Switchboard autoconnected to wifi successfully.");
      } else {
        Log::Write("WifiWatcher: Switchboard failed to autoconnect.");
      }
      break;
    }
  }
}

void WifiWatcher::WatcherTick(struct ev_loop* loop, struct ev_timer* w, int revents) {
  struct ev_WifiTimerStruct* timerStruct = (struct ev_WifiTimerStruct*)w;
  timerStruct->self->ConnectIfNoWifi();
}

} // Switchboard
} // Anki