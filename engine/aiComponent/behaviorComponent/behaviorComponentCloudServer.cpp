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

#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/cloud/mic.h"
#include "engine/cozmoContext.h"
#include "util/threading/threadPriority.h"
#include <chrono>

namespace Anki {
namespace Cozmo {

namespace {
  const std::string kWebVizTab = "cloudintents";
}

BehaviorComponentCloudServer::BehaviorComponentCloudServer(const CozmoContext* context, CallbackFunc callback,
                                                           const std::string& name, const int sleepMs)
: _callback(std::move(callback))
, _shutdown(false)
, _sleepMs(sleepMs)
#if SEND_CLOUD_DEV_RESULTS
, _context(context)
#endif
{
  _listenThread = std::thread([this, name] { RunThread(std::move(name)); });

  #if SEND_CLOUD_DEV_RESULTS
  AddSignalHandle(_context->GetWebService()->OnWebVizSubscribed(kWebVizTab).ScopedSubscribe(
    [this] (const auto& sendFunc) { OnClientInit(sendFunc); }));
  #endif
}

BehaviorComponentCloudServer::~BehaviorComponentCloudServer()
{
  _shutdown = true;
  _listenThread.join();
}

void BehaviorComponentCloudServer::RunThread(std::string sockName)
{
  Anki::Util::SetThreadName(pthread_self(), "BehaviorServer");
  // Start UDP server
  _server.SetBindClients(false);
  _server.StartListening(LOCAL_SOCKET_PATH + sockName);
  char buf[512];
  while (!_shutdown) {
    const ssize_t received = _server.Recv(buf, sizeof(buf));
    // ignore an empty reconnect packet that LocalUdpServer might forward
    const bool isReconnect = (received == 1 && buf[0] == '\0');
    if (received > 0 && !isReconnect) {
      CloudMic::Message msg{(const uint8_t*)buf, (size_t)received};
      const bool isDebug = AddDebugResult(msg);
      if (!isDebug) {
        _callback(std::move(msg));
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepMs));
  }
}

bool BehaviorComponentCloudServer::AddDebugResult(const CloudMic::Message& msg)
{
  #if SEND_CLOUD_DEV_RESULTS
  Json::Value value = msg.GetJSON();

  auto epochTime = std::chrono::system_clock::now().time_since_epoch();
  value["time"] = std::chrono::duration_cast<std::chrono::seconds>(epochTime).count();

  // don't let result buffer grow infinitely
  if (_devResults.size() > 8 && _devResults.size() == _devResults.capacity()) {
    for (int i = 0; i < _devResults.size() - 1; i++) {
      _devResults[i] = std::move(_devResults[i+1]);
    }
    _devResults[_devResults.size()-1] = std::move(value);
  }
  else {
    _devResults.emplace_back(std::move(value));
  }

  _context->GetWebService()->SendToWebViz(kWebVizTab, _devResults.back());
  return _devResults.back()["type"] == "debugFile";
  #else
  return false;
  #endif
}

#if SEND_CLOUD_DEV_RESULTS
void BehaviorComponentCloudServer::OnClientInit(const WebService::SendToClientFunc& sendFunc)
{
  Json::Value value{Json::arrayValue};
  for (const auto& val : _devResults) {
    value.append(val);
  }
  sendFunc(value);
}
#endif

}
}
