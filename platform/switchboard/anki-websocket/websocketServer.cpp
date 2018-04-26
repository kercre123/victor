/**
 * File: webService.cpp
 *
 * Author: richard; adapted for Victor by Paul Terry 01/08/2018
 * Created: 4/17/2017
 *
 * Description: Provides interface to civetweb, an embedded web server
 *
 *
 * Copyright: Anki, Inc. 2017-2018
 *
 **/

#include "websocketServer.h"

#include "switchboardd/log.h"

#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace Anki {
namespace Switchboard {

// Used websockets codes, see websocket RFC pg 29
// http://tools.ietf.org/html/rfc6455#section-5.2
enum {
  WebSocketsTypeText            = 0x1,
  WebSocketsTypeBinary          = 0x2,
  WebSocketsTypeCloseConnection = 0x8,
  WebSocketsTypePing            = 0x9,
  WebSocketsTypePong            = 0xA
};

WebsocketServer::WebsocketServer(std::shared_ptr<EngineMessagingClient> emc)
: _ctx(nullptr),
  _engineMessagingClient(emc)
{
  mg_init_library(0);
}

WebsocketServer::~WebsocketServer()
{
  if (_ctx) {
    mg_stop(_ctx);
  }
  _ctx = nullptr;
  mg_exit_library();
}

void WebsocketServer::Start() {
  const char *options[] = {
    "listening_ports",
    "80", // TODO: change this to "443s" when we 
          // have a TLS cert so we can handle
          // secure connections
    "num_threads",
    "1",
    0
  };
  struct mg_callbacks* callbacks = nullptr;

  _ctx = mg_start(/*&*/callbacks, this, options);

  mg_set_websocket_handler(_ctx,
                           "/",
                           HandleConnect,
                           HandleReady,
                           HandleData,
                           HandleClose,
                           0);
  _websocketMessageHandler = std::make_unique<WebsocketMessageHandler>(_engineMessagingClient);
  _onSendHandle = _websocketMessageHandler->OnSendEvent()
    .ScopedSubscribe(std::bind(&WebsocketServer::Send, this, std::placeholders::_1, std::placeholders::_2));
}

int WebsocketServer::HandleConnect(const struct mg_connection* conn, void* cbparams)
{
  return 0; // proceed with connection
}

void WebsocketServer::HandleReady(struct mg_connection* conn, void* cbparams)
{
  struct mg_context* ctx = mg_get_context(conn);
  WebsocketServer* that = static_cast<WebsocketServer*>(mg_get_user_data(ctx));
  that->OpenSocket( conn );
}

int WebsocketServer::HandleData(struct mg_connection* conn, int bits, char* data, size_t dataLen, void* cbparams)
{
  int ret = 1; // keep open

  // lower 4 bits
  const int opcode = bits & 0xF;

  // see websocket RFC §5.2 http://tools.ietf.org/html/rfc6455
  switch (opcode)
  {
    case WebSocketsTypeBinary:
    {
      struct mg_context* ctx = mg_get_context(conn);
      WebsocketServer* that = static_cast<WebsocketServer*>(mg_get_user_data(ctx));

      that->ReceiveSocket( conn, data, dataLen );
    }
      break;

    case WebSocketsTypeCloseConnection:
    {
      // agree to close connection, but don't do anything here until the close event fires
      ret = 0;
    }
      break;

    default:
      break;
  }

  return ret;
}

void WebsocketServer::HandleClose(const struct mg_connection* conn, void* cbparams)
{
  struct mg_context* ctx = mg_get_context(conn);
  WebsocketServer* that = static_cast<WebsocketServer*>(mg_get_user_data(ctx));
  that->CloseSocket( conn );
}

bool WebsocketServer::Send(uint8_t* data, size_t dataLen)
{
  if (_conn) {
    mg_websocket_write(_conn, WebSocketsTypeBinary, reinterpret_cast<char*>(data), dataLen);
    return true;
  }
  return false;
}

void WebsocketServer::OpenSocket(struct mg_connection* conn)
{
  if (_conn == nullptr) {
    _conn = conn;
  }
}

void WebsocketServer::ReceiveSocket(struct mg_connection* conn, char* data, size_t dataLen)//const Json::Value& data)
{
  _websocketMessageHandler->Receive(reinterpret_cast<unsigned char*>(data), dataLen);
}

void WebsocketServer::CloseSocket(const struct mg_connection* conn)
{
  if (_conn == conn) {
    _conn = nullptr;
  }
}

} // namespace Switchboard
} // namespace Anki
