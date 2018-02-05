/**
 * File: webService.h
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
#ifndef WEB_SERVICE_H
#define WEB_SERVICE_H

#include <string>
#include <vector>
#include <mutex>

#include "util/export/export.h"

struct mg_context; // copied from civetweb.h
struct mg_connection;

namespace Json {
  class Value;
}

namespace Anki {

namespace Util {
namespace Data {
  class DataPlatform;
} // namespace Data
} // namespace Util

namespace Cozmo {

namespace WebService {

class WebService
{
public:
  
  WebService();
  ~WebService();

  void Start(Anki::Util::Data::DataPlatform* platform, const char* portNumStr);
  void Update();
  void Stop();
  
  // send data to any client subscribed to service
  void SendToWebSockets(const std::string& service, const Json::Value& data);

  const std::string& getConsoleVarsTemplate();
  const std::string& getLuaTemplate();
  const std::string& getMetaGameJsonTemplate();

  enum RequestType
  {
    RT_ConsoleVarsUI,
    RT_ConsoleVarGet,
    RT_ConsoleVarSet,
    RT_ConsoleVarList,
    RT_ConsoleFuncCall,
  };

  struct Request
  {
    Request(RequestType rt, const std::string& param1, const std::string& param2);
    RequestType _requestType;
    std::string _param1;
    std::string _param2;
    std::string _result;
    bool        _resultReady; // Result is ready for use by the webservice thread
    bool        _done;        // Result has been used and now it's OK for main thread to delete this item
  };

  void AddRequest(Request* requestPtr);
  std::mutex _requestMutex;

private:

  void GenerateConsoleVarsUI(std::string& page, const std::string& category);

  struct WebSocketConnectionData {
    struct mg_connection* conn = nullptr;
    std::string serviceName;
    bool subscribed = false;
  };
  
  void SendWebSocketError(struct mg_connection* conn, const std::string& str) const;
  
  // called by civetweb
  static void HandleWebSocketsReady(struct mg_connection* conn, void* cbparams);
  static int HandleWebSocketsConnect(const struct mg_connection* conn, void* cbparams);
  static int HandleWebSocketsData(struct mg_connection* conn, int bits, char* data, size_t dataLen, void* cbparams);
  static void HandleWebSocketsClose(const struct mg_connection* conn, void* cbparams);
  
  // called by the above handlers
  void OnOpenWebSocket(struct mg_connection* conn);
  void OnReceiveWebSocket(struct mg_connection* conn, const Json::Value& data);
  void OnCloseWebSocket(const struct mg_connection* conn);
  
  static void SendToWebSocket(struct mg_connection* conn, const Json::Value& data);
  
  // lua handlers
  void HandleLuaGet(struct mg_connection* conn, const Json::Value& data);
  void HandleLuaSet(struct mg_connection* conn, const Json::Value& data);
  void HandleLuaRevert(struct mg_connection* conn, const Json::Value& data);
  void HandleLuaPlay(struct mg_connection* conn, const Json::Value& data);
  void HandleLuaStop(struct mg_connection* conn, const Json::Value& data);
  
  // todo: OTA update somehow?

  struct mg_context* _ctx;
  
  std::vector<WebSocketConnectionData> _webSocketConnections;

  std::string _consoleVarsUIHTMLTemplate;
  std::string _metaGameJsonHTMLTemplate;
  std::string _luaHTMLTemplate;

  std::vector<Request*> _requests;
};

} // namespace WebService
  
} // namespace Cozmo
} // namespace Anki

#endif // defined(WEB_SERVICE_H)
