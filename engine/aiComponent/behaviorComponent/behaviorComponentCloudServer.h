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

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorComponentCloudServer_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorComponentCloudServer_H__

#include "coretech/messaging/shared/UdpServer.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace Anki {
namespace Cozmo {

class BehaviorComponentCloudServer {
public:
  using CallbackFunc = std::function<void(std::string)>;

  BehaviorComponentCloudServer(CallbackFunc callback, short port, int sleepMs = 40);
  ~BehaviorComponentCloudServer();

private:
  void RunThread();

  CallbackFunc _callback;
  std::thread _listenThread;
  UdpServer _server;
  std::atomic_bool _shutdown;
  const short _port;
  const int _sleepMs;
};

}
}

#endif

