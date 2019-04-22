#pragma once

#include "ev++.h"
#include "signals/simpleSignal.hpp"

namespace Anki {
namespace Switchboard {

class Blesh {
public:
  using DataSignal = Signal::Signal<void (const std::vector<uint8_t>& data)>;

  Blesh(struct ev_loop* loop);
  ~Blesh();

  bool Connect();
  bool Disconnect();

  bool SendData(const std::vector<uint8_t>& data);

  DataSignal& OnReceiveData() { return _receiveDataSignal; }  

private:
  struct ev_loop* _loop;
  bool _connected = false;

  static void WatcherCallback(EV_P_ ev_io *w, int revents);

  struct ev_BleshStruct {
    ev_io watcher;
    uint32_t fd;
    Blesh* blesh;
  } _sshWatcher;

  DataSignal _receiveDataSignal;
};

} // Switchboard
} // Anki