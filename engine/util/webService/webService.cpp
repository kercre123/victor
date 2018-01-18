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

#include <cassert>
#include <string>

#if USE_DAS
#include "DAS/DAS.h"
#endif

#include "civetweb.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
//#include "driveEngine/luaScriptService/luaScriptService.h"
#include "util/logging/logging.h"
#include "util/console/consoleSystem.h"
#include "util/console/consoleChannel.h"
#include "util/helpers/templateHelpers.h"
#include "util/string/stringUtils.h"

#include "json/json.h"

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
ConsoleVarsUI(struct mg_connection *conn, void *cbdata)
{
  std::string style;
  std::string script;
  std::string html;
  std::map<std::string, std::string> category_html;

  Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();

  // Variables

  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();

  for (Anki::Util::ConsoleSystem::VariableDatabase::const_iterator it = varDatabase.begin();
       it != varDatabase.end();
       ++it) {
    std::string label = it->second->GetID();
    std::string cat = it->second->GetCategory();
    size_t dot = cat.find(".");
    if (dot != std::string::npos) {
      cat = cat.substr(0, dot);
    }

    if (it->second->IsToggleable()) {
      category_html[cat] += "                <div>\n";
      category_html[cat] += "                    <label for=\""+label+"\">"+label+"</label>\n";
      category_html[cat] += "                    <input type=\"checkbox\" name=\""+label+"\" id=\""+label+"\" onclick=\"onCheckboxClickHandler(this)\">\n";
      category_html[cat] += "                </div>\n";
      category_html[cat] += "                <br>\n";
    }
    else {
      char sliderRange[100];
      char inputRange[100];

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
    std::string sig = it->second->GetSignature();
    size_t dot = cat.find(".");
    if (dot != std::string::npos) {
      cat = cat.substr(0, dot);
    }

    if (sig.empty()) {
      category_html[cat] += "                <div>\n";
      category_html[cat] += "                  <input type=\"submit\" value=\""+label+"\">\n";
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

  struct mg_context *ctx = mg_get_context(conn);
  Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));

  std::string page = that->getConsoleVarsTemplate();

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

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");
  mg_printf(conn, "%s", page.c_str());

  return 1;
}

static int
ConsoleVarsSet(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;
  std::string value;

  if (info->content_length > 0) {
    char buf[info->content_length+1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    key = buf;
  }
  else if (info->query_string) {
    key = info->query_string;
  }

  if (key.substr(0, 4) =="key=") {
    size_t amp = key.find('&');
    if (amp != std::string::npos) {
      value = key.substr(amp+7);
      key = key.substr(4, amp-4);
    }

    Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(key.c_str());
    if (consoleVar && consoleVar->ParseText(value.c_str() )) {
      // success
      PRINT_NAMED_INFO("WebService", "CONSOLE_VAR %s %s", key.c_str(), value.c_str());
    }

    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
              "close\r\n\r\n");
  }
  return 1;
}

static int
ConsoleVarsGet(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;

  if (info->query_string) {
    if (!strncmp(info->query_string, "key=", 4)) {
      key = std::string(info->query_string+4);
    }
  }

  if (!key.empty()) {
    Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(key.c_str());
    if (consoleVar) {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
                "close\r\n\r\n");
      mg_printf(conn, "%s<br>", consoleVar->ToString().c_str());
    }
  }

  return 1;
}

static int
ConsoleVarsList(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);

  std::string key;

  if (info->query_string) {
    if (!strncmp(info->query_string, "key=", 4)) {
      key = std::string(info->query_string + 4);
    }
  }

  const Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
  const Anki::Util::ConsoleSystem::VariableDatabase& varDatabase = consoleSystem.GetVariableDatabase();

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  for (Anki::Util::ConsoleSystem::VariableDatabase::const_iterator it = varDatabase.begin();
       it != varDatabase.end();
       ++it) {
    std::string label = it->second->GetID();
    if (key.length() == 0 || Anki::Util::StringCaseInsensitiveEquals(label.substr(0, key.length()), key)) {
      mg_printf(conn, "%s<br>\n", label.c_str());
    }
  }

  return 1;
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

  if (func.substr(0,5) == "func=") {
    size_t amp = func.find('&');
    if (amp == std::string::npos) {
      func = func.substr(5);
    }
    else {
      args = func.substr(amp+6);
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

    // call func

    Anki::Util::IConsoleFunction* consoleFunc = Anki::Util::ConsoleSystem::Instance().FindFunction(func.c_str());
    if (consoleFunc)
    {
      char outText[255+1];
      uint32_t outTextLength = sizeof(outText);

      ExternalOnlyConsoleChannel consoleChannel(outText, outTextLength);

      Anki::Util::ConsoleSystem& consoleSystem = Anki::Util::ConsoleSystem::Instance();
      bool success = consoleSystem.ParseConsoleFunctionCall(consoleFunc, args.c_str(), consoleChannel);
      if (success) {
        PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s success", func.c_str(), args.c_str());
      }
      else {
        PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s failed %s", func.c_str(), args.c_str(), outText);
      }
    }
    else
    {
      PRINT_NAMED_INFO("WebService", "CONSOLE_FUNC %s %s not found", func.c_str(), args.c_str());
    }
  }

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");
  return 1;
}

#if 0 //////// temp

static int
JsonUpload(struct mg_connection *conn, void *cbdata)
{
  const struct mg_request_info* info = mg_get_request_info(conn);

  std::stringstream ssPayload;
  if (info->content_length > 0) {
    char buf[info->content_length + 1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    ssPayload << buf;
  }

  std::string jsonName;
  std::string jsonContents;

  // this is fragile and browser-specific, but it's dev only, so just use chrome and look away
  bool reading = false;
  int blobCnt = 0;
  for( std::string line; std::getline(ssPayload, line); ) {
    // if line starts with "Content-", start parsing. If it starts with "------", end
    if( line.substr(0,8) == "Content-" ) {
      reading = true;
    } else if( reading && (line.substr(0,6) == "------") ) {
      reading = false;
      ++blobCnt;
    } else if( reading && !line.empty() ) {
      if( blobCnt == 0 ) {
        jsonName += line;
      } else if( blobCnt == 1 ) {
        jsonContents += line + "\n";
      }
    }
  }

  jsonName.erase(std::remove(jsonName.begin(), jsonName.end(), '\r'), jsonName.end());

  bool success = false;
  if( !jsonName.empty() && !jsonContents.empty() ) {
    const auto& metagameComponent = Anki::DriveEngine::RushHour::getInstance()->GetMetaGameComponent();
    success = metagameComponent.ReloadFromString(jsonName, jsonContents);
  }

  if( success ) {
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: "
              "text/html\r\nConnection: close\r\n\r\n");
  } else {
    mg_printf(conn,
              "HTTP/1.1 400 Bad Request\r\nContent-Type: "
              "text/html\r\nConnection: close\r\n\r\n");
  }

  return 1;
}

static int
sku(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
  mg_printf(conn, "%s", STRINGIFY(ANKI_BUILD_SKUi));
#undef STRINGIFY_HELPER
#undef STRINGIFY
  return 1;
}

static int
appinfo(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  const Anki::DriveEngine::AppInfo* appInfo = Anki::DriveEngine::RushHour::getRefInstance().GetRushHourAppInfo()->GetAppInfo();
  std::string appString = "Version: " + appInfo->GetVersion() + " Display Version: " + appInfo->GetDisplayVersion() + " OS: " + appInfo->GetOSName() + " AppPlatform: " + appInfo->GetAppPlatform();

  mg_printf(conn, "%s", appString.c_str());
  return 1;
}


#endif  ///// temp

static int
dasinfo(struct mg_connection *conn, void *cbdata)
{
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

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
  std::string dasString = "DAS: #undefined";
#endif

  mg_printf(conn, "%s", dasString.c_str());
  return 1;
}

#if 0 //////  temp

static int
LuaScripts(struct mg_connection *conn, void *cbdata)
{
  struct mg_context *ctx = mg_get_context(conn);
  Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));
  
  const auto& luaScriptService = *Anki::DriveEngine::RushHour::getInstance()->GetLuaScriptService();
  if( luaScriptService.AreScriptsSupported() ) {
    std::string page = that->getLuaTemplate();

    std::string scriptsList;
    auto scriptNames = luaScriptService.GetScriptNames();
    std::sort(scriptNames.begin(), scriptNames.end());
    for( const auto& scriptName : scriptNames ) {
      std::string elapsedTimeStr;
      if( luaScriptService.IsPlaying(scriptName) ) {
        float elapsedTime = luaScriptService.GetElapsedTime(scriptName);
        elapsedTimeStr = "data-elapsed-time=\"" + std::to_string(elapsedTime) + "\" ";
      }
      // <option value="filename" data-elapsed-time="elapsedTime" >filename.lua</option>
      scriptsList += "  <option value=\"" + scriptName +  + "\" " + elapsedTimeStr + ">" + scriptName + ".lua</option>";
    }
    
    const std::string placeholder = "<!-- scripts list -->";
    Anki::Util::StringReplace(page, placeholder, scriptsList);

    mg_printf(conn,
              "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
              "close\r\n\r\n");
    mg_printf(conn, "%s", page.c_str());
  } else {
    mg_printf(conn,
              "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/html\r\nConnection: "
              "close\r\n\r\n");
    mg_printf(conn, "%s", "This device does not support scripts yet. You must access this page through a device hosting an active match.");
  }

  return 1;
}

static int
JsonUploadUI(struct mg_connection *conn, void *cbdata)
{
  struct mg_context *ctx = mg_get_context(conn);
  Anki::Cozmo::WebService::WebService* that = static_cast<Anki::Cozmo::WebService::WebService*>(mg_get_user_data(ctx));

  const auto& metagameComponent = Anki::DriveEngine::RushHour::getInstance()->GetMetaGameComponent();

  std::string page = that->getMetaGameJsonTemplate();

  std::string jsonList;
  auto jsonNames = metagameComponent.GetWebReloadableComponents();
  std::sort(jsonNames.begin(), jsonNames.end());
  for( const auto& jsonName : jsonNames ) {
    // <option value="filename">filename.json</option>
    jsonList += "  <option value=\"" + jsonName +  + "\">" + jsonName + ".json</option>";
  }
  
  const std::string placeholder = "<!-- json list -->";
  Anki::Util::StringReplace(page, placeholder, jsonList);

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");
  mg_printf(conn, "%s", page.c_str());

  return 1;
}

#endif  //////////

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

void WebService::Start(Anki::Util::Data::DataPlatform* platform)
{
  if (platform == nullptr) {
    return;
  }
  if (_ctx != nullptr) {
    return;
  }

  const std::string webserverPath = platform->pathToResource(Util::Data::Scope::Resources, "webserver");

  std::string rewrite;
  rewrite += "/output=" + platform->pathToResource(Util::Data::Scope::Persistent, "");
  rewrite += ",";
  rewrite += "/resources=" + platform->pathToResource(Util::Data::Scope::Resources, "");
  rewrite += ",";
  rewrite += "/cache=" + platform->pathToResource(Util::Data::Scope::Cache, "");
  rewrite += ",";
  rewrite += "/currentgamelog=" + platform->pathToResource(Util::Data::Scope::CurrentGameLog, "");
  rewrite += ",";
  rewrite += "/external=" + platform->pathToResource(Util::Data::Scope::External, "");
#if USE_DAS
  rewrite += ",";
  rewrite += "/daslog=" + std::string(DASGetLogDir());
#endif

  const std::string& passwordFile = platform->pathToResource(Util::Data::Scope::Resources, "webserver/htpasswd");

  const char *options[] = {
    "document_root",
    webserverPath.c_str(),
    "listening_ports",
    "8888",
    "num_threads",
    "4",
    "url_rewrite_patterns",
    rewrite.c_str(),
    "put_delete_auth_file",
    passwordFile.c_str(),
    "authentication_domain",
    "anki.com",
    "websocket_timeout_ms",
    "3600000", // 1 hour
#if defined(NDEBUG)
    "global_auth_file",
    passwordFile.c_str(),
#endif
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
//  mg_set_request_handler(_ctx, "/json", JsonUploadUI, 0);

  mg_set_request_handler(_ctx, "/consolevarset", ConsoleVarsSet, 0);
  mg_set_request_handler(_ctx, "/consolevarget", ConsoleVarsGet, 0);
  mg_set_request_handler(_ctx, "/consolevarlist", ConsoleVarsList, 0);
  mg_set_request_handler(_ctx, "/consolefunccall", ConsoleFuncCall, 0);
//  mg_set_request_handler(_ctx, "/jsonUpload", JsonUpload, 0);

//  mg_set_request_handler(_ctx, "/sku", sku, 0);
//  mg_set_request_handler(_ctx, "/appinfo", appinfo, 0);
  mg_set_request_handler(_ctx, "/dasinfo", dasinfo, 0);

//  mg_set_request_handler(_ctx, "/scripts", LuaScripts, 0); // landing page. other xfer through sockets

  const std::string& consoleVarsTemplate = platform->pathToResource(Util::Data::Scope::Resources, "webserver/consolevarsui.html");
  _consoleVarsUIHTMLTemplate = Anki::Util::StringFromContentsOfFile(consoleVarsTemplate);

  const std::string& luaTemplate = platform->pathToResource(Util::Data::Scope::Resources, "webserver/lua.html");
  _luaHTMLTemplate = Anki::Util::StringFromContentsOfFile(luaTemplate);
  
  const std::string& metaGameJsonTemplate = platform->pathToResource(Util::Data::Scope::Resources, "webserver/json.html");
  _metaGameJsonHTMLTemplate = Anki::Util::StringFromContentsOfFile(metaGameJsonTemplate);
}

void WebService::Update()
{
  //PRINT_NAMED_INFO("WebService", "Calling Update on WebService");
}

void WebService::Stop()
{
  if (_ctx)
  {
    mg_stop(_ctx);
  }
  _ctx = nullptr;
}

#if 0
void WebService::SendWebSocketError(struct mg_connection* conn, const std::string& str) const
{
  Json::Value err;
  err["type"] = "error";
  err["reason"] = str;
  SendToWebSocket(conn, err);
}

void WebService::SendToWebSockets(const std::string& service, const Json::Value& data)
{
  for( const auto& connData : _webSocketConnections ) {
    if( connData.subscribed && (connData.serviceName.empty() || (connData.serviceName == service)) ) {
      SendToWebSocket(connData.conn, data);
    }
  }
}
#endif

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

#if 0 // websockets used for OD game tuning...disabling for now
void WebService::SendToWebSocket(struct mg_connection* conn, const Json::Value& data)
{
  // todo: deal with threads if this is used outside dev
  
  std::stringstream ss;
  ss << data;
  std::string str = ss.str();
  mg_websocket_write(conn, WebSocketsTypeText, str.c_str(), str.size());
}
#endif

const std::string& WebService::getConsoleVarsTemplate()
{
  return _consoleVarsUIHTMLTemplate;
}
  
const std::string& WebService::getLuaTemplate()
{
  return _luaHTMLTemplate;
}
  
const std::string& WebService::getMetaGameJsonTemplate()
{
  return _metaGameJsonHTMLTemplate;
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
    if( !data["type"].isNull() ) {
      
      if( (data["type"].asString() == "subscribe") && !data["service"].isNull() ) {
        
        // first message after connection is "subscribe", which can be empty, meaning subscribe to all
        it->serviceName = data["service"].asString();
        it->subscribed = true;
        
//      } else if( data["type"].asString() == "luaget" ) {
//        HandleLuaGet(conn, data);
//      } else if( data["type"].asString() == "luaset" ) {
//        HandleLuaSet(conn, data);
//      } else if( data["type"].asString() == "luarevert" ) {
//        HandleLuaRevert(conn, data);
//      } else if( data["type"].asString() == "luaplay" ) {
//        HandleLuaPlay(conn, data);
//      } else if( data["type"].asString() == "luastop" ) {
//        HandleLuaStop(conn, data);
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


#if 0 ///////

void WebService::HandleLuaGet(struct mg_connection* conn, const Json::Value& data)
{
  if( !ANKI_DEVELOPER_CODE ) {
    // dont show anyone our scripts for now
    return;
  }

  bool success = false;
  std::string selectedScript;
  
  if( data["script"].isString() ) {
    selectedScript = data["script"].asString();
  }
  
  if( selectedScript == "NOTASCRIPTNAME" ) { // the default dropdown value...shouldnt happen
    selectedScript = "";
  }
  
  if( !selectedScript.empty() ) {
    std::string contents;
    float elapsedTime = -1.0f;
    
    const auto& luaScriptService = *Anki::DriveEngine::RushHour::getInstance()->GetLuaScriptService();
    contents = luaScriptService.GetScript( selectedScript );
    elapsedTime = luaScriptService.GetElapsedTime( selectedScript );
    assert( !contents.empty() );
    
    if( !contents.empty() ) {
      
      Json::Value retJson;
      retJson["type"] = "luascriptcontents";
      retJson["script"] = selectedScript;
      retJson["contents"] = contents;
      retJson["elapsedTime"] = elapsedTime;
      
      SendToWebSocket( conn, retJson );
      
      success = true;
    }
  }
  
  if( !success ) {
    SendWebSocketError(conn, "lua script " + selectedScript + " not found" );
  }
}
  
void WebService::HandleLuaSet(struct mg_connection* conn, const Json::Value& data)
{
  std::string scriptName;
  std::string scriptContents;
  
  if( data["scriptName"].isString() ) {
    scriptName = data["scriptName"].asString();
  }
  if( !data["contents"].isNull() ) {
    scriptContents = data["contents"].asString();
  }
  
  if( !scriptName.empty() && !scriptContents.empty() ) {
    
    auto& luaScriptService = *Anki::DriveEngine::RushHour::getRefInstance().GetLuaScriptService();
    luaScriptService.SetScript(scriptName, scriptContents);
    // response comes from LuaScriptService
  } else {
    SendWebSocketError( conn, "400 bad request (luaset)" );
  }
}
 
void WebService::HandleLuaRevert(struct mg_connection* conn, const Json::Value& data)
{
  if( data["scriptName"].isString() ) {
    std::string scriptName;
    scriptName = data["scriptName"].asString();

    auto& luaScriptService = *Anki::DriveEngine::RushHour::getRefInstance().GetLuaScriptService();
    luaScriptService.RevertScript(scriptName);

    Json::Value retJson;
    retJson["type"] = "luascriptcontents";
    retJson["script"] = scriptName;
    retJson["contents"] = luaScriptService.GetScript(scriptName);
    retJson["elapsedTime"] = luaScriptService.GetElapsedTime(scriptName);
    SendToWebSocket(conn, retJson);

  } else {
    SendWebSocketError( conn, "400 bad request (luarevert)" );
  }
}

void WebService::HandleLuaPlay(struct mg_connection* conn, const Json::Value& data)
{
  if( data["scriptName"].isString() ) {
    std::string scriptName;
    scriptName = data["scriptName"].asString();
  
    auto& luaScriptService = *Anki::DriveEngine::RushHour::getRefInstance().GetLuaScriptService();
    luaScriptService.CommandScript(scriptName, true);
    // response comes from LuaScriptService
  } else {
    SendWebSocketError( conn, "400 bad request (luaplay)" );
  }
}
 
void WebService::HandleLuaStop(struct mg_connection* conn, const Json::Value& data)
{
  if( data["scriptName"].isString() ) {
    std::string scriptName;
    scriptName = data["scriptName"].asString();
  
    auto& luaScriptService = *Anki::DriveEngine::RushHour::getRefInstance().GetLuaScriptService();
    luaScriptService.CommandScript(scriptName, false);
  } else {
    SendWebSocketError( conn, "400 bad request (luastop)" );
  }
}

#endif  /////

} // namespace WebService
} // namespace Cozmo
} // namespace Anki

