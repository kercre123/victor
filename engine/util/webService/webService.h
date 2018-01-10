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
#ifndef __driveEngine_webService_webService_H__
#define __driveEngine_webService_webService_H__

#include <string>
#include <vector>

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

class Vehicle;

namespace WebService {

class WebService
{
public:
  
  WebService();
  ~WebService();

  void Start(Anki::Util::Data::DataPlatform* platform);
  void Stop();
  
  // send data to any client subscribed to service
  void SendToWebSockets(const std::string& service, const Json::Value& data);

  const std::string& getConsoleVarsTemplate();
  const std::string& getLuaTemplate();
  const std::string& getMetaGameJsonTemplate();

private:
  
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
  
  // carpool (firmware)
  static int HandleFirmwareUpload(struct mg_connection* conn, void* cbdata); // called upon page visit
  std::string OnFlashFirmwareFile(const std::vector<std::string>& carUUIDs, uint8_t* data, int len); // return is empty or err str
  std::vector<Vehicle*> _vehicles;
  
  struct mg_context* _ctx;
  
  std::vector<WebSocketConnectionData> _webSocketConnections;

  std::string _consoleVarsUIHTMLTemplate;
  std::string _metaGameJsonHTMLTemplate;
  std::string _luaHTMLTemplate;

};

} // namespace WebService
  
} // namespace Cozmo
} // namespace Anki

namespace Anki {
namespace Cozmo {

ANKI_EXPORT void StartWebService();

} // namespace Cozmo
} // namespace Anki

#endif // defined(__driveEngine_webService_webService_H__)
