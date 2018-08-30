/**
 * File: wifiWatcher.h
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

#pragma once

#include "anki-wifi/wifi.h"
#include "ev++.h"

namespace Anki {
namespace Switchboard {

class WifiWatcher {
public:
  WifiWatcher(struct ev_loop* loop);
  ~WifiWatcher();

  void ConnectIfNoWifi();

private:
  static void WatcherTick(struct ev_loop* loop, struct ev_timer* w, int revents);

  struct ev_loop* _loop;

  struct ev_WifiTimerStruct {
    ev_timer timer;
    WifiWatcher* self;
  } _timer;

  const uint8_t kWifiTick_s = 15;
};

} // Switchboard
} // Anki