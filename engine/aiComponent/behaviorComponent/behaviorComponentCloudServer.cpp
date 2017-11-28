/**
 * File: behaviorComponentCloudServer
 *
 * Author: baustin
 * Created: 10/31/17
 *
 * Description: Provides a server endpoint for cloud process to connect to
 *              and send messages
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudServer.h"

namespace Anki {
namespace Cozmo {

BehaviorComponentCloudServer::BehaviorComponentCloudServer(CallbackFunc callback, const short port, const int sleepMs)
: _callback(std::move(callback))
, _shutdown(false)
, _port(port)
, _sleepMs(sleepMs)
{
  _listenThread = std::thread([this] { RunThread(); });
}

BehaviorComponentCloudServer::~BehaviorComponentCloudServer()
{
  _shutdown = true;
  _listenThread.join();
}

void BehaviorComponentCloudServer::RunThread()
{
  // Start UDP server
  _server.StartListening(_port);
  char buf[512];
  while (!_shutdown) {
    const ssize_t received = _server.Recv(buf, sizeof(buf));
    if (received > 0) {
      std::string msg{buf, buf+received};
      _callback(std::move(msg));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepMs));
  }
}

}
}
