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
#include "coretech/messaging/shared/socketConstants.h"
#include "engine/cozmoContext.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"
#include <chrono>

#define LOG_CHANNEL "BehaviorComponentCloudServer"

namespace Anki {
namespace Vector {

namespace {
  const std::string kWebVizTab = "cloudintents";
}

BehaviorComponentCloudServer::BehaviorComponentCloudServer(const CozmoContext* context, CallbackFunc callback,
                                                           const std::string& name)
: _callback(std::move(callback))
, _shutdown(false)
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

  if (_listenThread.joinable())
  {
    _listenThread.join();
  }
}

void BehaviorComponentCloudServer::RunThread(std::string sockName)
{
  Anki::Util::SetThreadName(pthread_self(), "BehaviorServer");

  // Start UDP server
  _server.SetBindClients(false);
  _server.StartListening(AI_SERVER_BASE_PATH + sockName);

  char buf[4096];
  const int fd = _server.GetSocket();
  const int nfds = fd + 1;

  while (!_shutdown)
  {
    bool ready = false;

    while (!ready)
    {
      // Block until a message arrives, with timeout.  We use a timeout simply to prevent
      // this thread from hanging on shutdown of engine.  The timeout (100 ms) is longer
      // than the 40ms sleep we had when polling, and well within the approximately 12
      // seconds it takes for the thread to crash after engine starts shutting down.
      fd_set fdset;
      FD_ZERO(&fdset);
      FD_SET(fd, &fdset);
      timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      const int n = select(nfds, &fdset, nullptr, nullptr, &timeout);
      if (_shutdown)
      {
        LOG_INFO("BehaviorComponentCloudServer.RunThread", "Exiting thread due to shutdown");
        return;
      }
      if (n <= 0 && errno == EINTR) {
        LOG_WARNING("BehaviorComponentCloudServer.RunThread", "select interrupted (errno %d)", errno);
        return;
      }
      if (n == 0) { // Timeout
        continue;
      }
      if (n < 0) {
        LOG_WARNING("BehaviorComponentCloudServer.RunThread", "select error (errno %d)", errno);
        break;
      }
      if (!FD_ISSET(fd, &fdset)) {
        LOG_WARNING("BehaviorComponentCloudServer.RunThread", "socket not ready?");
        break;
      }
      ready = true;
    }

    const ssize_t received = _server.Recv(buf, sizeof(buf));
    if (_shutdown)
    {
      break;
    }
    if (received > 0)
    {
      // ignore an empty reconnect packet that LocalUdpServer might forward
      const bool isReconnect = (received == 1 && buf[0] == '\0');
      if (!isReconnect)
      {
        CloudMic::Message msg{(const uint8_t*)buf, (size_t)received};
        const bool isDebug = AddDebugResult(msg);
        if (!isDebug)
        {
          _callback(std::move(msg));
        }
      }
    }

  } // End while (!_shutdown)
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
