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

#include "webService.h"

#if USE_DAS
#include "DAS/DAS.h"
#endif

#include "civetweb.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "util/logging/logging.h"
#include "util/console/consoleSystem.h"
#include "util/console/consoleChannel.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/ankiDefines.h"
#include "util/helpers/templateHelpers.h"
#include "util/string/stringUtils.h"

#include "osState/osState.h"

#if defined(ANKI_PLATFORM_ANDROID)
#include <android/log.h>
#endif

#include <cassert>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>

using namespace Anki::Cozmo;


// Used websockets codes, see websocket RFC pg 29
// http://tools.ietf.org/html/rfc6455#section-5.2
enum {
  WebSocketsTypeText            = 0x1,
  WebSocketsTypeCloseConnection = 0x8
};

class ExternalOnlyConsoleChannel : public Anki::Util::IConsoleChannel
{
public:

  ExternalOnlyConsoleChannel(char* outText, uint32_t outTextLength)
    : _outText(outText)
    , _outTextLength(outTextLength)
    , _outTextPos(0)
  {
    assert((_outText!=nullptr) && (_outTextLength > 0));
    
    _tempBuffer = new char[kTempBufferSize];
  }
  
  virtual ~ExternalOnlyConsoleChannel()
  {
    if (_outText != nullptr)
    {
      // insure out buffer is null terminated
      
      if (_outTextPos < _outTextLength)
      {
        _outText[_outTextPos] = 0;
      }
      else
      {
        _outText[_outTextLength - 1] = 0;
      }
    }
    
    Anki::Util::SafeDeleteArray( _tempBuffer );
  }
  
  bool IsOpen() override { return true; }
  
  int WriteData(uint8_t *buffer, int len) override
  {
    assert(0); // not implemented (and doesn't seem to ever be called?)
    return len;
  }
  
  int WriteLogv(const char *format, va_list args) override
  {
    // Print to a temporary buffer first so we can use that for any required logs
    const int printRetVal = vsnprintf(_tempBuffer, kTempBufferSize, format, args);
    
    if (printRetVal > 0)
    {
      if ((_outText != nullptr) && (_outTextLength > _outTextPos))
      {
        const uint32_t remainingRoom = _outTextLength - _outTextPos;
        // new line is implicit in all log calls
        const int outPrintRetVal = snprintf(&_outText[_outTextPos], remainingRoom, "%s\n", _tempBuffer);
        // note outPrintRetVal can be >remainingRoom (snprintf returns size required, not size used)
        // so this can make _outTextPos > _outTextLength, but this is ok as we only use it when < _outTextLength
        _outTextPos += (outPrintRetVal > 0) ? outPrintRetVal : 0;
      }
    }
    
    return printRetVal;
  }
  
  int WriteLog(const char *format, ...) override
  {
    va_list ap;
    va_start(ap, format);
    int result = WriteLogv(format, ap);
    va_end(ap);
    return result;
  }
  
  bool Flush() override
  {
    // already flushed
    return true;
  }
  
  void SetTTYLoggingEnabled(bool newVal) override
  {
  }
  
  bool IsTTYLoggingEnabled() const override
  {
    return true;
  }
  
  const char* GetChannelName() const override { return nullptr; }
  void SetChannelName(const char* newName) override {}
  
private:
  
  static const size_t kTempBufferSize = 1024;
  
  char*     _tempBuffer;
  char*     _outText;
  uint32_t  _outTextLength;
  uint32_t  _outTextPos;
};

static int
LogMessage(const struct mg_connection *conn, const char *message)
{
  PRINT_NAMED_INFO("WebService", "%s", message);
  return 1;
}

static int
LogHandler(struct mg_connection *conn, void *cbdata)
{
#if USE_DAS
  // Stop rolling over logs so they are viewable
  // (otherwise, they get uploaded and then deleted pretty quickly)
  DASDisableNetwork(DASDisableNetworkReason_LogRollover);
#endif

  // pretend we didn't handle it and pass onto the default handler
  return 0;
}


static int
ProcessRequest(struct mg_connection *conn, Anki::Cozmo::WebService::WebService::RequestType requestType,
               const std::string& param1, const std::string& param2, const std::string& param3 = "", bool waitAndSendResponse = true)
{
  WebService::WebService::Request* requestPtr = new WebService::WebService::Request(requestType, param1, param2, param3);

  struct mg_context *ctx = mg_get_context(conn);
  WebService::WebService* that = static_cast<WebService::WebService*>(mg_get_user_data(ctx));

  that->AddRequest(requestPtr);

  if( waitAndSendResponse ) {
    
    // Now wait until the main thread processes the request
    using namespace std::chrono;
    static const double kTimeoutDuration_s = 10.0;
    const auto startTime = steady_clock::now();
    bool timedOut = false;
    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      const auto now = steady_clock::now();
      const auto elapsed_s = duration_cast<seconds>(now - startTime).count();
      if (elapsed_s > kTimeoutDuration_s)
      {
        timedOut = true;
        break;
      }
    } while (!requestPtr->_resultReady);

    // We check if result is there because we just slept and it may have come in
    // just before the timeout
    if (timedOut && !requestPtr->_resultReady)
    {
      std::lock_guard<std::mutex> lock(that->_requestMutex);
      requestPtr->_result = "Timed out after " + std::to_string(kTimeoutDuration_s) + " seconds";
    }

    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
              "close\r\n\r\n");
    mg_printf(conn, "%s\n", requestPtr->_result.c_str());
  
    // Now mark the request as done so the main thread can delete it.
    // if you pass !waitAndSendResponse, you need to manually set this flag
    requestPtr->_done = true;
  }

  return 1;
}

static int
ConsoleVarsUI(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);
  std::string category = ((info->query_string) ? info->query_string : "");

  const int returnCode = ProcessRequest(conn, WebService::WebService::RequestType::RT_ConsoleVarsUI, category, "");

  return returnCode;
}

static int
ConsoleVarSet(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string query;

  if (info->content_length > 0) {
    char buf[info->content_length+1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    query = buf;
  }
  else if (info->query_string) {
    query = info->query_string;
  }

  const int returnCode = ProcessRequest(conn, WebService::WebService::RequestType::RT_ConsoleVarSet, query, "");

  return returnCode;
}

static int
ConsoleVarGet(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;

  if (info->query_string) {
    if (!strncmp(info->query_string, "key=", 4)) {
      key = std::string(info->query_string+4);
    }
  }

  const int returnCode = ProcessRequest(conn, WebService::WebService::RequestType::RT_ConsoleVarGet, key, "");

  return returnCode;
}

static int
ConsoleVarList(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;

  if (info->query_string) {
    if (!strncmp(info->query_string, "key=", 4)) {
      key = std::string(info->query_string + 4);
    }
  }
  
  const int returnCode = ProcessRequest(conn, WebService::WebService::RequestType::RT_ConsoleVarList, key, "");

  return returnCode;
}

static int
ConsoleFuncList(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;

  if (info->query_string) {
    if (!strncmp(info->query_string, "key=", 4)) {
      key = std::string(info->query_string + 4);
    }
  }

  const int returnCode = ProcessRequest(conn, Anki::Cozmo::WebService::WebService::RequestType::RT_ConsoleFuncList, key, "");

  return returnCode;
}

static int
ConsoleFuncCall(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string func;
  std::string args;
  if (info->content_length > 0) {
    char buf[info->content_length+1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    func = buf;
  }
  else if (info->query_string) {
    func = info->query_string;
  }

  int returnCode = 1;

  if (func.substr(0,5) == "func=") {
    size_t amp = func.find('&');
    if (amp == std::string::npos) {
      func = func.substr(5);
    }
    else {
      args = func.substr(amp+6);  // skip over "args="
      func = func.substr(5, amp-5);

      // unescape '+' => ' '

      size_t plus = args.find("+");
      while (plus != std::string::npos) {
        args = args.replace(plus, 1, " ");
        plus = args.find("+", plus+1);
      }

      // unescape '\"' => '"'

      size_t quote = args.find("\\\"");
      while (quote != std::string::npos) {
        args = args.replace(quote, 2, "\"");
        quote = args.find("\\\"", quote+2);
      }
    }

    returnCode = ProcessRequest(conn, WebService::WebService::RequestType::RT_ConsoleFuncCall, func, args);
  }

  return returnCode;
}

static int
ProcessRequestFromQueryString(struct mg_connection *conn, void *cbdata, WebService::WebService::RequestType type)
{
  const mg_request_info* info = mg_get_request_info(conn);
  std::string request;
  if (info->content_length > 0) {
    char buf[info->content_length+1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    request = buf;
  }
  else if (info->query_string) {
    request = info->query_string;
  }
  int returnCode = ProcessRequest(conn, type, request, "");
  
  return returnCode;
}

static int
TempAppToEngine(struct mg_connection *conn, void *cbdata)
{
  return ProcessRequestFromQueryString( conn,
                                        cbdata,
                                        Anki::Cozmo::WebService::WebService::RequestType::RT_TempAppToEngine );
}

static int
TempEngineToApp(struct mg_connection *conn, void *cbdata)
{
  return ProcessRequestFromQueryString( conn,
                                        cbdata,
                                        Anki::Cozmo::WebService::WebService::RequestType::RT_TempEngineToApp );
}


WebService::WebService::Request::Request(RequestType rt,
                                         const std::string& param1,
                                         const std::string& param2,
                                         const std::string& param3)
{
  _requestType = rt;
  _param1 = param1;
  _param2 = param2;
  _param3 = param3;
  _result = "";
  _resultReady = false;
  _done = false;
}
Anki::Cozmo::WebService::WebService::Request::Request(RequestType rt, const std::string& param1, const std::string& param2)
  : Request(rt, param1, param2, "")
{
  
}

static int
dasinfo(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

// NOTE:  For some reason, android builds of the webserver are not getting USE_DAS defined properly
#if USE_DAS
  std::string dasString = "DAS: " + std::string(DASGetLogDir()) + " DASDisableNetworkReason:";
  int disabled = DASNetworkingDisabled;
  if (disabled & DASDisableNetworkReason_Simulator) {
    dasString += " Simulator";
  }
  if (disabled & DASDisableNetworkReason_UserOptOut) {
    dasString += " UserOptOut";
  }
  if (disabled & DASDisableNetworkReason_Shutdown) {
    dasString += " Shutdown";
  }
  if (disabled & DASDisableNetworkReason_LogRollover) {
    dasString += " LogRollover";
  }
//  if (disabled & DASDisableNetworkReason_Debug) {
//    dasString += " Debug";
//  }
#else
  std::string dasString = "DAS: #undefined for this platform";
#endif

  mg_printf(conn, "%s", dasString.c_str());
  return 1;
}


static int GetInitialConfig(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  struct mg_context *ctx = mg_get_context(conn);
  WebService::WebService* that = static_cast<WebService::WebService*>(mg_get_user_data(ctx));

  const std::string title0    = that->GetConfig()["title0"].asString();
  const std::string title1    = that->GetConfig()["title1"].asString();
  const std::string startPage = that->GetConfig()["startPage"].asString();
#ifdef SIMULATOR
  const std::string webotsSim = "true";
#else
  const std::string webotsSim = "false";
#endif

  mg_printf(conn, "%s\n%s\n%s\n%s\n", title0.c_str(), title1.c_str(),
            startPage.c_str(), webotsSim.c_str());
  return 1;
}


static int GetMainRobotInfo(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  const auto& osState = OSState::getInstance();
  const std::string robotID  = std::to_string(osState->GetRobotID());
  const std::string serialNo = osState->GetSerialNumberAsString();
  const std::string ip       = osState->GetIPAddress();

  const std::string buildConfig =
#if defined(DEBUG)
  "DEBUG";
#elif defined(RELEASE)
  "RELEASE";
#elif defined(PROFILE)
  "PROFILE";
#elif defined(SHIPPING)
  "SHIPPING";
#else
  "UNKNOWN";
#endif

#ifdef SIMULATOR

  const std::string procVersion = "n/a (webots)";
  const std::string procCmdLine = "n/a (webots)";

#else

  // This is a one-time read of info that won't change during the run,
  // so we don't keep any file streams open
  std::ifstream fs;

  fs.open("/proc/version", std::ifstream::in);
  std::string procVersion;
  std::getline(fs, procVersion);
  fs.close();

  fs.open("/proc/cmdline", std::ifstream::in);
  std::string procCmdLine;
  std::getline(fs, procCmdLine);
  fs.close();

#endif

  mg_printf(conn, "%s\n%s\n%s\n%s\n%s\n%s\n",
            robotID.c_str(), serialNo.c_str(), ip.c_str(),
            buildConfig.c_str(),
            procVersion.c_str(), procCmdLine.c_str());
  return 1;
}


static int GetPerfStats(struct mg_connection *conn, void *cbdata)
{
  using namespace std::chrono;
  const auto startTime = steady_clock::now();

  static const int kNumStats = 3;
  bool active[kNumStats];

  const mg_request_info* info = mg_get_request_info(conn);
  const std::string boolsString = info->query_string ? info->query_string : "";
  int i = 0;
  for ( ; i < kNumStats && boolsString[i]; i++)
  {
    active[i] = (boolsString[i] == '1');
  }
  // If the string wasn't long enough, make the rest of the flags false
  for ( ; i < kNumStats; i++)
  {
    active[i] = false;
  }

#ifdef SIMULATOR

  // On webots simulator, most if not all of these don't apply
  const std::string stat_cpuFreq = "n/a";
  const std::string stat_temperature = "n/a";
  const std::string stat_batteryVoltage = "n/a";

#else

  const auto& osState = OSState::getInstance();

  std::string stat_cpuFreq;
  if (active[0]) {
    // Update cpu freq
    const uint32_t cpuFreq_kHz = osState->UpdateCPUFreq_kHz();
    stat_cpuFreq = std::to_string(cpuFreq_kHz);
  }

  std::string stat_temperature;
  if (active[1]) {
    // Update temperature reading (Celsius)
    const uint32_t cpuTemp_C = osState->UpdateTemperature_C();
    stat_temperature = std::to_string(cpuTemp_C);
  }

  std::string stat_batteryVoltage;
  if (active[2]) {
    // NOTE: Can no longer (if not now then soon) access battery voltage via OSState.
    //       Engine's BatteryComponent could possibly send updates to webserver.
    // // Battery voltage
    // const uint32_t batteryVoltage_uV = osState->UpdateBatteryVoltage_uV();
    // const float batteryVoltage_V = batteryVoltage_uV * 0.000001f;
    // std::stringstream ss;
    // ss << std::fixed << std::setprecision(6) << batteryVoltage_V;
    // stat_batteryVoltage = ss.str();
  }

#endif

  const auto now = steady_clock::now();
  const auto elapsed_us = duration_cast<microseconds>(now - startTime).count();
  PRINT_NAMED_INFO("WebService", "GetPerfStats took %lld microseconds to read", elapsed_us);

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  mg_printf(conn, "%s\n%s\n%s\n",
            stat_cpuFreq.c_str(),
            stat_temperature.c_str(),
            stat_batteryVoltage.c_str());

  return 1;
}


namespace Anki {
namespace Cozmo {
namespace WebService {

WebService::WebService()
: _ctx(nullptr)
{
}

WebService::~WebService()
{
  Stop();
}

void WebService::Start(Anki::Util::Data::DataPlatform* platform, const Json::Value& config)
{
  if (platform == nullptr) {
    return;
  }
  if (_ctx != nullptr) {
    return;
  }

  _config = config;

  const std::string portNumString = _config["port"].asString();

  const std::string webserverPath = platform->pathToResource(Util::Data::Scope::Resources, "webserver");

  std::string rewrite;
  rewrite += "/persistent=" + platform->pathToResource(Util::Data::Scope::Persistent, "");
  rewrite += ",";
  rewrite += "/resources=" + platform->pathToResource(Util::Data::Scope::Resources, "");
  rewrite += ",";
  rewrite += "/cache=" + platform->pathToResource(Util::Data::Scope::Cache, "");
  rewrite += ",";
  rewrite += "/currentgamelog=" + platform->pathToResource(Util::Data::Scope::CurrentGameLog, "");
#if USE_DAS
  rewrite += ",";
  rewrite += "/daslog=" + std::string(DASGetLogDir());
#endif

//  const std::string& passwordFile = platform->pathToResource(Util::Data::Scope::Resources, "webserver/htpasswd");

  const char *options[] = {
    "document_root",
    webserverPath.c_str(),
    "listening_ports",
    portNumString.c_str(),
    "num_threads",
    "4",
    "url_rewrite_patterns",
    rewrite.c_str(),
//    "put_delete_auth_file",
//    passwordFile.c_str(),
//    "authentication_domain",
//    "anki.com",
    "websocket_timeout_ms",
    "3600000", // 1 hour
//#if defined(SHIPPING)
//    "global_auth_file",
//    passwordFile.c_str(),
//#endif
    0
  };

  struct mg_callbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.log_message = LogMessage;

  _ctx = mg_start(&callbacks, this, options);
  
  mg_set_websocket_handler(_ctx,
                           "/socket",
                           HandleWebSocketsConnect,
                           HandleWebSocketsReady,
                           HandleWebSocketsData,
                           HandleWebSocketsClose,
                           0);

  mg_set_request_handler(_ctx, "/daslog", LogHandler, 0);
  mg_set_request_handler(_ctx, "/consolevars", ConsoleVarsUI, 0);

  mg_set_request_handler(_ctx, "/consolevarset", ConsoleVarSet, 0);
  mg_set_request_handler(_ctx, "/consolevarget", ConsoleVarGet, 0);
  mg_set_request_handler(_ctx, "/consolevarlist", ConsoleVarList, 0);
  mg_set_request_handler(_ctx, "/consolefunclist", ConsoleFuncList, 0);
  mg_set_request_handler(_ctx, "/consolefunccall", ConsoleFuncCall, 0);

  mg_set_request_handler(_ctx, "/dasinfo", dasinfo, 0);
  mg_set_request_handler(_ctx, "/getinitialconfig", GetInitialConfig, 0);
  mg_set_request_handler(_ctx, "/getmainrobotinfo", GetMainRobotInfo, 0);
  mg_set_request_handler(_ctx, "/getperfstats", GetPerfStats, 0);

  // todo (VIC-1398): remove
  if( ANKI_DEV_CHEATS ) { 
    mg_set_request_handler(_ctx, "/sendAppMessage", TempAppToEngine, 0);
    mg_set_request_handler(_ctx, "/getAppMessages", TempEngineToApp, 0);
  }

  const std::string& consoleVarsTemplate = platform->pathToResource(Util::Data::Scope::Resources, "webserver/consolevarsui.html");
  _consoleVarsUIHTMLTemplate = Anki::Util::StringFromContentsOfFile(consoleVarsTemplate);

  _requests.clear();
}


// This is called from the main thread
void WebService::Update()
{
  std::lock_guard<std::mutex> lock(_requestMutex);

  // First pass:  Delete any completely-finished requests from the list (and delete the requests themselves)
  size_t destIndex = 0;
  for (size_t srcIndex = 0; srcIndex < _requests.size(); srcIndex++)
  {
    Request* requestPtr = _requests[srcIndex];
    if (requestPtr->_done)
    {
      delete requestPtr;
    }
    else
    {
      if (srcIndex != destIndex)
      {
        _requests[destIndex] = _requests[srcIndex];
      }
      destIndex++;
    }
  }
  _requests.resize(destIndex);

  Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();

  // Second pass:  Process any requests that haven't been processed yet
  for (const auto requestPtr : _requests)
  {
    if (!requestPtr->_resultReady)
    {
      switch (requestPtr->_requestType)
      {
        case RT_ConsoleVarsUI:
          {
            GenerateConsoleVarsUI(requestPtr->_result, requestPtr->_param1);
          }
          break;
        case RT_ConsoleVarGet:
          {
            const std::string& key = requestPtr->_param1;

            if (!key.empty()) {
              const Anki::Util::IConsoleVariable* consoleVar = consoleSystem.FindVariable(key.c_str());
              if (consoleVar) {
                requestPtr->_result = consoleVar->ToString() + "<br>";
              }
              else {
                requestPtr->_result = "Variable not found<br>";
              }
            }
            else {
              requestPtr->_result = "Key required (name of variable)<br>";
            }
          }
          break;
        case RT_ConsoleVarSet:
          {
            std::string key = requestPtr->_param1;

            while (!key.empty()) {
              if (key.substr(0, 4) == "key=") {
                std::string value;
                std::string remainder;

                size_t amp = key.find('&');
                if (amp != std::string::npos) {
                  value = key.substr(amp+7);
                  key = key.substr(4, amp-4);
                  size_t amp = value.find('&');
                  if (amp != std::string::npos) {
                    remainder = value.substr(amp+1);
                    value = value.substr(0, amp);
                  }
                }

                Anki::Util::IConsoleVariable* consoleVar = consoleSystem.FindVariable(key.c_str());
                if (consoleVar) {
                  if (consoleVar->ParseText(value.c_str() )) {
                    // success
                    PRINT_NAMED_INFO("WebService", "CONSOLE_VAR %s %s", key.c_str(), value.c_str());
                    requestPtr->_result += consoleVar->ToString() + "<br>";
                  }
                  else {
                    requestPtr->_result += "Error setting variable "+key+"="+value+"<br>";
                  }
                }
                else {
                  requestPtr->_result += "Variable not found "+key+"<br>";
                }

                key = remainder;
              } else {
                break;
              }
            }
          }
          break;

        case RT_ConsoleVarList:
          {
            const std::string& key = requestPtr->_param1;
            const auto keyLen = key.length();

            const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
            for (Anki::Util::ConsoleSystem::VariableDatabase::const_iterator it = varDatabase.begin();
                 it != varDatabase.end(); ++it)
            {
              std::string label = it->second->GetID();
              if (keyLen == 0 || Anki::Util::StringCaseInsensitiveEquals(label.substr(0, keyLen), key))
              {
                requestPtr->_result += label.c_str();
                requestPtr->_result += "<br>\n";
              }
            }
          }
          break;

        case RT_ConsoleFuncList:
          {
            const std::string& key = requestPtr->_param1;
            const auto keyLen = key.length();

            const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
            for (Anki::Util::ConsoleSystem::FunctionDatabase::const_iterator it = funcDatabase.begin();
                 it != funcDatabase.end(); ++it)
            {
              std::string label = it->second->GetID();
              if (keyLen == 0 || Anki::Util::StringCaseInsensitiveEquals(label.substr(0, keyLen), key))
              {
                requestPtr->_result += label.c_str();
                requestPtr->_result += "<br>\n";
              }
            }
          }
          break;

        case RT_ConsoleFuncCall:
          {
            const std::string& func = requestPtr->_param1;
            const std::string& args = requestPtr->_param2;

            Anki::Util::IConsoleFunction* consoleFunc = consoleSystem.FindFunction(func.c_str());
            if (consoleFunc) {
              // 256KB to accommodate output of animation names
              char outText[256*1024+1] = {0};
              uint32_t outTextLength = sizeof(outText);

              ExternalOnlyConsoleChannel consoleChannel(outText, outTextLength);

              bool success = consoleSystem.ParseConsoleFunctionCall(consoleFunc, args.c_str(), consoleChannel);
              if (success) {
                PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s success", func.c_str(), args.c_str());
                requestPtr->_result += outText;
              }
              else {
                PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s failed %s", func.c_str(), args.c_str(), outText);
                requestPtr->_result += outText;
              }
            }
            else {
              PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s not found", func.c_str(), args.c_str());
            }
          }
          break;
        case RT_TempAppToEngine:
          {
            _appToEngineOnData.emit( requestPtr->_param1 );
          }
          break;
        case RT_TempEngineToApp:
          {
            requestPtr->_result = _appToEngineRequestData.emit();
          }
          break;
        case RT_WebsocketOnSubscribe:
        case RT_WebsocketOnData:
          {
            const auto& moduleName = requestPtr->_param1;
            const auto& idxStr = requestPtr->_param2;
            size_t idx = std::stoi( idxStr );
            
            auto sendToClient = [idx, moduleName, this](const Json::Value& toSend){
              // might crash if webservice is somehow destroyed after the subscriber, but only in dev
              if( (idx < _webSocketConnections.size())
                 && (_webSocketConnections[idx].subscribedModules.count( moduleName ) > 0) )
              {
                Json::Value payload;
                payload["module"] = moduleName;
                payload["data"] = toSend;
                SendToWebSocket( _webSocketConnections[idx].conn, payload );
              }
            };
            
            if( requestPtr->_requestType == RT_WebsocketOnSubscribe ) {
              auto signalIt = _webVizSubscribedSignals.find( moduleName );
              if( signalIt != _webVizSubscribedSignals.end() ) {
                signalIt->second.emit( sendToClient );
              }
            } else if( requestPtr->_requestType == RT_WebsocketOnData ) {
              const auto& dataStr = requestPtr->_param3;
              Json::Reader reader;
              Json::Value data;
              
              if( reader.parse(dataStr, data) ) {
                auto signalIt = _webVizDataSignals.find( moduleName );
                if( signalIt != _webVizDataSignals.end() ) {
                  signalIt->second.emit( data, sendToClient );
                }
              }
            }
            requestPtr->_done = true; // no one cares about the result, just cleanup immediately
          }
          break;
      }

      // Notify the requesting thread that the result is now ready
      requestPtr->_resultReady = true;
    }
  }
}

void WebService::Stop()
{
  if (_ctx) {
    mg_stop(_ctx);
  }
  _ctx = nullptr;
}


void WebService::AddRequest(Request* requestPtr)
{
  std::lock_guard<std::mutex> lock(_requestMutex);
  _requests.push_back(requestPtr);
}


void WebService::GenerateConsoleVarsUI(std::string& page, const std::string& category)
{
  std::string style;
  std::string script;
  std::string html;
  std::map<std::string, std::string> category_html;

  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();

  // Variables

  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();
  for (Anki::Util::ConsoleSystem::VariableDatabase::const_iterator it = varDatabase.begin();
       it != varDatabase.end();
       ++it) {
    std::string label = it->second->GetID();
    std::string cat = it->second->GetCategory();

    if (!category.empty() && category != cat) {
      continue;
    }

    size_t dot = cat.find(".");
    if (dot != std::string::npos) {
      cat = cat.substr(0, dot);
    }

    if (category_html.find(cat) == category_html.end()) {
      category_html[cat] = "";
    }

    if (it->second->IsToggleable()) {
      category_html[cat] += "                <div>\n";
      category_html[cat] += "                    <label for=\""+label+"\">"+label+"</label>\n";
      category_html[cat] += "                    <input type=\"checkbox\" name=\""+label+"\" id=\""+label+"\" onclick=\"onCheckboxClickHandler(this)\">\n";
      category_html[cat] += "                </div>\n";
      category_html[cat] += "                <br>\n";
    }
    else {
      char sliderRange[200];
      char inputRange[200];

      if (it->second->IsIntegerType()) {
        if (it->second->IsSignedType()) {
          snprintf(sliderRange, sizeof(sliderRange),
                   "data-value=\"%lld\" data-begin=\"%lld\" data-end=\"%lld\" data-scale=\"1\"",
                   it->second->GetAsInt64(),
                   it->second->GetMinAsInt64(),
                   it->second->GetMaxAsInt64());

          snprintf(inputRange, sizeof(inputRange),
                   "min=\"%lld\" max=\"%lld\"",
                   it->second->GetMinAsInt64(),
                   it->second->GetMaxAsInt64());
        }
        else {
          snprintf(sliderRange, sizeof(sliderRange),
                   "data-value=\"%llu\" data-begin=\"%llu\" data-end=\"%llu\" data-scale=\"1\"",
                   it->second->GetAsUInt64(),
                   it->second->GetMinAsUInt64(),
                   it->second->GetMaxAsUInt64());

          snprintf(inputRange, sizeof(inputRange),
                   "min=\"%llu\" max=\"%llu\"",
                   it->second->GetMinAsUInt64(),
                   it->second->GetMaxAsUInt64());
        }
      }
      else {
        snprintf(sliderRange, sizeof(sliderRange),
                 "data-value=\"%g\" data-begin=\"%g\" data-end=\"%g\" data-scale=\"100.0\"",
                 it->second->GetAsDouble(),
                 it->second->GetMinAsDouble(),
                 it->second->GetMaxAsDouble());

        snprintf(inputRange, sizeof(inputRange),
                 "min=\"%g\" max=\"%g\"",
                 it->second->GetMinAsDouble(),
                 it->second->GetMaxAsDouble());
      }

      category_html[cat] += "                <div>\n";
      category_html[cat] += "                  <label for=\""+label+"_amount\">"+label+":</label>\n";
      category_html[cat] += "                  <div id=\""+label+"\" class=\"slider\" "+sliderRange+" style=\"width: 100px; margin: 0.25em;\"></div>\n";
      category_html[cat] += "                  <input type=\"text\" id=\""+label+"_amount\" class=\"amount\" "+inputRange+" style=\"margin: 0.25em; border:1; font-weight:bold;\">\n";
      category_html[cat] += "                </div><br>\n";
    }
  }

  // Functions

  const Anki::Util::ConsoleSystem::FunctionDatabase& funcDatabase = consoleSystem.GetFunctionDatabase();
  for (Anki::Util::ConsoleSystem::FunctionDatabase::const_iterator it = funcDatabase.begin();
       it != funcDatabase.end();
       ++it) {
    std::string label = it->second->GetID();
    std::string cat = it->second->GetCategory();

    if (!category.empty() && category != cat) {
      continue;
    }

    std::string sig = it->second->GetSignature();
    size_t dot = cat.find(".");
    if (dot != std::string::npos) {
      cat = cat.substr(0, dot);
    }

    if (sig.empty()) {
      category_html[cat] += "                <div>\n";
      category_html[cat] += "                  <input type=\"submit\" value=\""+label+"\" class=\"function\">\n";
      category_html[cat] += "                </div><br>\n";
    }
    else {
      category_html[cat] += "                <div>\n";
      category_html[cat] += "                  <a id=\"tt\" title=\"("+sig+")\"><label for=\""+label+"_function\">"+label+":</label></a>\n";
      category_html[cat] += "                  <input type=\"text\" id=\""+label+"_args\" value=\"\" style=\"margin: 0.25em; border:1; font-weight:bold;\">\n";
      category_html[cat] += "                  <input type=\"submit\" id=\""+label+"_function\" value=\"Call\" class=\"function\">\n";
      category_html[cat] += "                </div><br>\n";
    }
  }

  for (std::map<std::string, std::string>::const_iterator it = category_html.begin();
       it != category_html.end();
       ++it) {
    html += "            <h3>"+it->first+"</h3>\n";
    html += "            <div>\n";
    html += it->second;
    html += "            </div>\n";
  }

  page = getConsoleVarsTemplate();

  std::string tmp;
  size_t pos;

  tmp = "/* -- generated style -- */";
  pos = page.find(tmp);
  if (pos != std::string::npos) {
    page = page.replace(pos, tmp.length(), style);
  }

  tmp = "// -- generated script --";
  pos = page.find(tmp);
  if (pos != std::string::npos) {
    page = page.replace(pos, tmp.length(), script);
  }

  tmp = "<!-- generated html -->";
  pos = page.find(tmp);
  if (pos != std::string::npos) {
    page = page.replace(pos, tmp.length(), html);
  }
}


void WebService::SendToWebSockets(const std::string& moduleName, const Json::Value& data) const
{
  Json::Value payload;
  payload["module"] = moduleName;
  payload["data"] = data;
  for( const auto& connData : _webSocketConnections ) {
    if( connData.subscribedModules.find( moduleName ) != connData.subscribedModules.end() ) {
      SendToWebSocket(connData.conn, payload);
    }
  }
}

int WebService::HandleWebSocketsConnect(const struct mg_connection* conn, void* cbparams)
{
  return 0; // proceed with connection
}
  
void WebService::HandleWebSocketsReady(struct mg_connection* conn, void* cbparams)
{
  struct mg_context* ctx = mg_get_context(conn);
  Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));
  DEV_ASSERT(that != nullptr, "Expecting valid webservice this pointer");
  that->OnOpenWebSocket( conn );
}

int WebService::HandleWebSocketsData(struct mg_connection* conn, int bits, char* data, size_t dataLen, void* cbparams)
{
  int ret = 1; // keep open
  
  // lower 4 bits
  const int opcode = bits & 0xF;

  // see websocket RFC §5.2 http://tools.ietf.org/html/rfc6455
  switch (opcode)
  {
    case WebSocketsTypeText:
    {
      if( (dataLen >= 2) && (data[0] == '{') ) {
        struct mg_context* ctx = mg_get_context(conn);
        Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));
        DEV_ASSERT(that != nullptr, "Expecting valid webservice this pointer");
        
        Json::Reader reader;
        Json::Value payload;
        bool success = reader.parse(data, payload);
        if( success ) {
          that->OnReceiveWebSocket( conn, payload );
        }
      }
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

void WebService::HandleWebSocketsClose(const struct mg_connection* conn, void* cbparams)
{
  struct mg_context* ctx = mg_get_context(conn);
  Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));
  DEV_ASSERT(that != nullptr, "Expecting valid webservice this pointer");
  that->OnCloseWebSocket( conn );
}

void WebService::SendToWebSocket(struct mg_connection* conn, const Json::Value& data)
{
  // todo: deal with threads if this is used outside dev
  
  std::stringstream ss;
  ss << data;
  std::string str = ss.str();
  mg_websocket_write(conn, WebSocketsTypeText, str.c_str(), str.size());
}

const std::string& WebService::getConsoleVarsTemplate()
{
  return _consoleVarsUIHTMLTemplate;
}
  
void WebService::OnOpenWebSocket(struct mg_connection* conn)
{
  ASSERT_NAMED(conn != nullptr, "Can't create connection to n");
  // add a connection to the list that applies to all services
  _webSocketConnections.push_back({});
  _webSocketConnections.back().conn = conn;
}
  
void WebService::OnReceiveWebSocket(struct mg_connection* conn, const Json::Value& data)
{
  // todo: deal with threads
  
  // find connection
  auto it = std::find_if( _webSocketConnections.begin(), _webSocketConnections.end(), [&conn](const auto& perConnData) {
    return perConnData.conn == conn;
  });
  
  if( it != _webSocketConnections.end() ) {
    if( !data["type"].isNull() && !data["module"].isNull() ) {
      const std::string& moduleName = data["module"].asString();
      size_t idx = it - _webSocketConnections.begin();
      
      if( data["type"].asString() == "subscribe" ) {
        it->subscribedModules.insert( moduleName );
        
        const bool waitAndSendResponse = false;
        ProcessRequest(conn,
                       Anki::Cozmo::WebService::WebService::RequestType::RT_WebsocketOnSubscribe,
                       moduleName,
                       std::to_string(idx),
                       "",
                       waitAndSendResponse);
      }
      if( data["type"].asString() == "unsubscribe" ) {
        it->subscribedModules.erase( moduleName );
      }
      if( (data["type"].asString() == "data") && !data["data"].isNull() ) {
        const bool waitAndSendResponse = false;
        std::stringstream ss;
        ss << data["data"];
        ProcessRequest(conn,
                       Anki::Cozmo::WebService::WebService::RequestType::RT_WebsocketOnData,
                       moduleName,
                       std::to_string(idx),
                       ss.str(),
                       waitAndSendResponse);
      }
      
    }
  } else {
    std::stringstream ss;
    ss << data;
    PRINT_NAMED_ERROR("Webservice.OnReceiveWebSocket","No connection for data %s", ss.str().c_str());
  }
}
  
void WebService::OnCloseWebSocket(const struct mg_connection* conn)
{
  // find connection
  auto it = std::find_if( _webSocketConnections.begin(), _webSocketConnections.end(), [&conn](const auto& perConnData) {
    return perConnData.conn == conn;
  });
  
  // erase it
  auto& data = *it;
  std::swap(data, _webSocketConnections.back());
  _webSocketConnections.pop_back();
}

} // namespace WebService
} // namespace Cozmo
} // namespace Anki

