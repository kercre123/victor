/**
 * File: websocketServer.h
 *
 * Author: richard; adapted for Victor by Paul Terry 01/08/2018; adapted for switchboard by Shawn Blakesley 04/11/2018
 * Created: 4/17/2017
 *
 * Description: Provides interface to civetweb, an embedded web server
 *
 *
 * Copyright: Anki, Inc. 2017-2018
 *
 **/
#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "civetweb/include/civetweb.h"

#include "libev/libev.h"

#include <string>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

#include "signals/simpleSignal.hpp"
#include "switchboardd/websocketMessageHandler.h"
#include "switchboardd/engineMessagingClient.h"

struct mg_context; // copied from civetweb.h
struct mg_connection;

namespace Anki {

namespace Switchboard {

class WebsocketServer
{
public:
  explicit WebsocketServer(std::shared_ptr<EngineMessagingClient> emc);
  ~WebsocketServer();

  using WebSocketMessageSignal = Signal::Signal<void (uint8_t*, size_t)>;

  WebSocketMessageSignal& OnReceiveEvent() {
    return _receiveSignal;
  }
  void Start();
private:
  // no connection in send because maybe we'll just talk to the most recent connection?
  bool Send(uint8_t* data, size_t dataLen);

  // called by civetweb
  static void HandleReady(struct mg_connection* conn, void* cbparams);
  static int HandleConnect(const struct mg_connection* conn, void* cbparams);
  static int HandleData(struct mg_connection* conn, int bits, char* data, size_t dataLen, void* cbparams);
  static void HandleClose(const struct mg_connection* conn, void* cbparams);

  void ConnectSocket(struct mg_connection* conn);
  void OpenSocket(struct mg_connection* conn);
  void ReceiveSocket(struct mg_connection* conn, char* data, size_t dataLen);
  void CloseSocket(const struct mg_connection* conn);

  struct mg_context* _ctx;

  // TODO: Use this to reject new connections if we're already connected.
  struct mg_connection* _conn;

  Signal::SmartHandle _onSendHandle;

  WebSocketMessageSignal _receiveSignal;
  std::unique_ptr<WebsocketMessageHandler> _websocketMessageHandler;
  std::shared_ptr<EngineMessagingClient> _engineMessagingClient;
};
  
} // namespace Switchboard
} // namespace Anki

#endif // defined(WEBSOCKET_SERVER_H)
