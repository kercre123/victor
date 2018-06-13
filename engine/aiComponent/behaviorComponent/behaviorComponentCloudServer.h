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

#include "coretech/messaging/shared/LocalUdpServer.h"
#include "util/global/globalDefinitions.h"
#include "util/signals/signalHolder.h"
#include "webServerProcess/src/webService.h"
#include <json/json.h>
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace Anki {
namespace Cozmo {

namespace CloudMic {
class Message;
}

class CozmoContext;

class BehaviorComponentCloudServer : private Util::SignalHolder {
public:
  using CallbackFunc = std::function<void(CloudMic::Message)>;

  BehaviorComponentCloudServer(const CozmoContext* context, CallbackFunc callback, const std::string& name, int sleepMs = 40);
  ~BehaviorComponentCloudServer();

private:
  void RunThread(std::string);

  CallbackFunc _callback;
  std::thread _listenThread;
  LocalUdpServer _server;
  std::atomic_bool _shutdown;
  const int _sleepMs;

  #define SEND_CLOUD_DEV_RESULTS ANKI_DEV_CHEATS
  #if SEND_CLOUD_DEV_RESULTS
  const CozmoContext* _context;
  std::vector<Json::Value> _devResults;

  using WebService = WebService::WebService;
  void OnClientInit(const WebService::SendToClientFunc& sendFunc);
  #endif
  bool AddDebugResult(const CloudMic::Message& msg);
};

}
}

#endif

