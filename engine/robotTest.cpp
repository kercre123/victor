/**
 * File: robotTest
 *
 * Author: Paul Terry
 * Created: 04/05/2019
 *
 * Description: Engine-based test framework for physical robot
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/cozmoContext.h"
#include "engine/perfMetricEngine.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/robotTest.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/string/stringUtils.h"

#include "webServerProcess/src/webService.h"

#define LOG_CHANNEL "RobotTest"

namespace Anki {
namespace Vector {

const std::string RobotTest::kInactiveScriptName = "(NONE)";

#if ANKI_ROBOT_TEST_ENABLED

namespace {
  const char* kScriptCommandsKey = "scriptCommands";
  const char* kCommandKey = "command";
  const char* kParametersKey = "parameters";
}


static int RoboTestWebServerImpl(WebService::WebService::Request* request)
{
  auto* robotTest = static_cast<RobotTest*>(request->_cbdata);

  int returnCode = robotTest->ParseWebCommands(request->_param1);
  if (returnCode != 0)
  {
    // If there were no errors, attempt to execute the commands, and output
    // string messages/results so that they can be returned in the web request
    robotTest->ExecuteQueuedWebCommands(&request->_result);
  }

  return returnCode;
}

// Note that this can be called at any arbitrary time, from a webservice thread
static int RobotTestWebServerHandler(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);
  auto* robotTest = static_cast<RobotTest*>(cbdata);

  std::string commands;
  if (info->content_length > 0)
  {
    char buf[info->content_length + 1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    commands = buf;
  }
  else if (info->query_string)
  {
    commands = info->query_string;
  }

  auto ws = robotTest->GetWebService();
  const int returnCode = ws->ProcessRequestExternal(conn, cbdata, RoboTestWebServerImpl, commands);

  return returnCode;
}


RobotTest::RobotTest(const CozmoContext* context)
  : _context(context)
{
}

RobotTest::~RobotTest()
{
//  OnShutdown();
}

void RobotTest::Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService)
{
  _webService = webService;
  _webService->RegisterRequestHandler("/robottest", RobotTestWebServerHandler, this);

  _platform = dataPlatform;
  _uploadedScriptsPath = _platform->pathToResource(Util::Data::Scope::Cache,
                                                   "robotTestScripts");
  Util::FileUtils::CreateDirectory(_uploadedScriptsPath);

  // Find and load all scripts in the resources folder
  std::string scriptsPath = _platform->pathToResource(Util::Data::Scope::Resources,
                                                      "config/engine/robotTestFramework");
  static const bool kUseFullPath = true;
  static const bool kRecurse = true;
  auto fileList = Util::FileUtils::FilesInDirectory(scriptsPath, kUseFullPath, "json", kRecurse);
  for (const auto& scriptFilePath : fileList)
  {
    Json::Value scriptJson;
    const bool success = _platform->readAsJson(scriptFilePath, scriptJson);
    if (!success)
    {
      LOG_ERROR("RobotTest.Init.ScriptLoadError",
                "Robot test script file %s failed to parse as JSON",
                scriptFilePath.c_str());
    }
    else
    {
      const bool isValid = ValidateScript(scriptJson);
      if (!isValid)
      {
        LOG_ERROR("RobotTest.Init.ScriptValidationError",
                  "Robot test script file %s is valid JSON but has one or more errors",
                  scriptFilePath.c_str());
      }
      else
      {
        static const bool kMustHaveExtension = true;
        static const bool kRemoveExtension = true;
        const std::string name = Util::FileUtils::GetFileName(scriptFilePath, kMustHaveExtension, kRemoveExtension);
        ScriptsMap::const_iterator it = _scripts.find(name);
        if (it != _scripts.end())
        {
          LOG_ERROR("RobotTest.Init.DuplicateScriptName",
                    "Duplicate test script file name %s; ignoring script with duplicate name",
                    scriptFilePath.c_str());
        }
        else
        {
          RobotTestScript script;
          script._name = name;
          script._wasUploaded = false;
          script._scriptJson = scriptJson;
          _scripts[name] = script;
        }
      }
    }
  }
  LOG_INFO("RobotTest.Init",
           "Successfully loaded and validated %i robot test script files out of %i found in resources",
           static_cast<int>(_scripts.size()), static_cast<int>(fileList.size()));
}


// This is called near the start of the tick
void RobotTest::Update()
{
  ANKI_CPU_PROFILE("RobotTest::Update");

  while (_state == RobotTestState::RUNNING)
  {
    const bool commandCompleted = ExecuteScriptCommand(_nextScriptCommand);
    if (commandCompleted)
    {
      _curScriptCommandIndex++;
      FetchNextScriptCommand();
    }
    else
    {
      // If the command is not completed (e.g. waiting for a signal), we're done with this tick
      break;
    }
  }
}


// Parse commands out of the query string, and only if there are no errors,
// add them to the queue
int RobotTest::ParseWebCommands(std::string& queryString)
{
  queryString = Util::StringToLower(queryString);

  std::vector<RobotTestWebCommand> cmds;
  std::string current;

  while (!queryString.empty())
  {
    size_t amp = queryString.find('&');
    if (amp == std::string::npos)
    {
      current = queryString;
      queryString = "";
    }
    else
    {
      current = queryString.substr(0, amp);
      queryString = queryString.substr(amp + 1);
    }

    if (current == "stop")
    {
      RobotTestWebCommand cmd(WebCommandType::STOP);
      cmds.push_back(cmd);
    }
    else if (current == "status")
    {
      RobotTestWebCommand cmd(WebCommandType::STATUS);
      cmds.push_back(cmd);
    }
    else
    {
      // Commands that have arguments:
      static const std::string cmdKeywordRun("run=");
      
      if (current.substr(0, cmdKeywordRun.size()) == cmdKeywordRun)
      {
        std::string argumentValue = current.substr(cmdKeywordRun.size());
        RobotTestWebCommand cmd(WebCommandType::RUN, argumentValue);
        cmds.push_back(cmd);
      }
      else
      {
        LOG_ERROR("RobotTest.ParseWebCommands", "Error parsing robot test web command: %s", current.c_str());
        return 0;
      }
    }
  }

  // Now that there are no errors, add all parsed commands to queue
  for (auto& cmd : cmds)
  {
    _queuedWebCommands.push(cmd);
  }
  return 1;
}


void RobotTest::ExecuteQueuedWebCommands(std::string* resultStr)
{
  // Execute queued web commands
  while (!_queuedWebCommands.empty())
  {
    const RobotTestWebCommand cmd = _queuedWebCommands.front();
    _queuedWebCommands.pop();
    switch (cmd._webCommand)
    {
      case WebCommandType::RUN:
        ExecuteWebCommandRun(cmd._paramString, resultStr);
        break;
      case WebCommandType::STOP:
        ExecuteWebCommandStop(resultStr);
        break;
      case WebCommandType::STATUS:
        ExecuteWebCommandStatus(resultStr);
        break;
    }
  }
}


void RobotTest::ExecuteWebCommandRun(const std::string& scriptName, std::string* resultStr)
{
  const bool success = StartScript(scriptName);
  if (success)
  {
    *resultStr += "Started";
  }
  else
  {
    *resultStr += "Failed to start";
  }
  *resultStr += (" running script \"" + scriptName + "\"\n");
}


void RobotTest::ExecuteWebCommandStop(std::string* resultStr)
{
  StopScript();
  *resultStr += "No script running\n";
}


void RobotTest::ExecuteWebCommandStatus(std::string* resultStr)
{
  *resultStr += (_state == RobotTestState::INACTIVE ?
                 "Inactive" : "Running: " + _curScriptName);
  *resultStr += "\n";
}


bool RobotTest::ValidateScript(const Json::Value& scriptJson)
{
  bool valid = true;

  // todo ensure there is a "scriptCommands" list.
  // todo check for errors, e.g. all commands are valid. (call StringToScriptCommand)  report specific errors in log.  return false if errors found
  // todo If command is 'PerfMetric', could check 'parameters' is valid by calling PerfMetric::ParseCommand

  return valid;
}


bool RobotTest::StartScript(const std::string& scriptName)
{
  if (_state == RobotTestState::RUNNING)
  {
    StopScript();
  }
  const ScriptsMap::const_iterator it = _scripts.find(scriptName);
  if (it == _scripts.end())
  {
    LOG_INFO("RobotTest.StartScript", "Start requested for script %s but script not found",
             scriptName.c_str());
    return false;
  }

  LOG_INFO("RobotTest.StartScript", "Starting script %s", scriptName.c_str());
  _state = RobotTestState::RUNNING;
  _curScriptName = scriptName;
  _curScriptCommandsJson = &it->second._scriptJson[kScriptCommandsKey];
  _curScriptCommandIndex = 0;
  _waitTickCount = 0;
  _waitTime = 0.0f;
  _waitingForCloudIntent = false;
  FetchNextScriptCommand();
  return true;
}


void RobotTest::StopScript()
{
  if (_state != RobotTestState::RUNNING)
  {
    LOG_INFO("RobotTest.StopScript", "Stop command given but no script was running");
    return;
  }
  LOG_INFO("RobotTest.StopScript", "Stopping script %s", _curScriptName.c_str());
  _state = RobotTestState::INACTIVE;
  _curScriptName = kInactiveScriptName;
  _curScriptCommandsJson = nullptr;
}


void RobotTest::FetchNextScriptCommand()
{
  if (_curScriptCommandIndex >= _curScriptCommandsJson->size())
  {
    // Script had no instructions, or we're pointing beyond the end of the script
    _nextScriptCommand = ScriptCommandType::EXIT;
  }
  else
  {
    const auto& commandJson = (*_curScriptCommandsJson)[_curScriptCommandIndex];
    const auto& commandStr = commandJson[kCommandKey].asString();
    const bool success = StringToScriptCommand(commandStr, _nextScriptCommand);
    if (!success)
    {
      // This should not happen once we start validating scripts when they are loaded
      LOG_ERROR("RobotText.FetchNextScriptCommand", "Error fetching next script command");
    }
  }
}


bool RobotTest::StringToScriptCommand(const std::string& commandStr, ScriptCommandType& command)
{
  // todo: probably convert the ScriptCommandType to a CLAD enum, so we get the conversion code automatically
  static const std::unordered_map<std::string, RobotTest::ScriptCommandType> stringToEnumMap =
  {
    {"exit", RobotTest::ScriptCommandType::EXIT},
    {"perfMetric", RobotTest::ScriptCommandType::PERFMETRIC},
    {"cloudIntent", RobotTest::ScriptCommandType::CLOUD_INTENT},
    {"waitCloudIntent", RobotTest::ScriptCommandType::WAIT_CLOUD_INTENT},
    {"waitUntilEngineTickCount", RobotTest::ScriptCommandType::WAIT_UNTIL_ENGINE_TICK_COUNT},
    {"waitTicks", RobotTest::ScriptCommandType::WAIT_TICKS},
    {"waitSeconds", RobotTest::ScriptCommandType::WAIT_SECONDS},
  };

  auto it = stringToEnumMap.find(commandStr);
  if (it == stringToEnumMap.end())
  {
    return false;
  }

  command = it->second;
  return true;
}


bool RobotTest::ExecuteScriptCommand(ScriptCommandType command)
{
  bool commandCompleted = true;

  switch (command)
  {
    case ScriptCommandType::EXIT:
    {
      // If we've gone past the last instruction, or we've
      // reached an exit command, it's time to stop
      StopScript();
      commandCompleted = false;
    }
    break;

    case ScriptCommandType::PERFMETRIC:
    {
      const auto& paramsStr = (*_curScriptCommandsJson)[_curScriptCommandIndex][kParametersKey].asString();
      const auto perfMetric = _context->GetPerfMetric();
      const int success = perfMetric->ParseCommands(paramsStr);
      if (success)
      {
        perfMetric->ExecuteQueuedCommands();
      }
    }
    break;

    case ScriptCommandType::CLOUD_INTENT:
    {
      Robot* robot = _context->GetRobotManager()->GetRobot();
      auto& uic = robot->GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

      const auto& params = (*_curScriptCommandsJson)[_curScriptCommandIndex][kParametersKey];

      Json::StyledWriter writer;
      std::string stringifiedJSON;
      stringifiedJSON = writer.write(params);

      uic.SetCloudIntentPendingFromString(stringifiedJSON);

      _waitingForCloudIntent = true;
    }
    break;

    case ScriptCommandType::WAIT_CLOUD_INTENT:
    {
      if (_waitingForCloudIntent)
      {
        commandCompleted = false;
      }
    }
    break;

    case ScriptCommandType::WAIT_UNTIL_ENGINE_TICK_COUNT:
    {
      // todo: Wait until engine global tick count is beyond the number specified.  set commandCompleted to true if so.
      // note:  re-use _waitTickCount; just set to (target tick count - cur tick count).
    }
    break;

    case ScriptCommandType::WAIT_TICKS:
    {
      if (_waitTickCount <= 0)
      {
        _waitTickCount = (*_curScriptCommandsJson)[_curScriptCommandIndex][kParametersKey].asInt();
        commandCompleted = _waitTickCount <= 0 ? true : false;
      }
      else
      {
        if (--_waitTickCount > 0)
        {
          commandCompleted = false;
        }
      }
    }
    break;

    case ScriptCommandType::WAIT_SECONDS:
    {
      // todo: Wait for N seconds.  set commandCompleted to true when done.
    }
    break;
  }

  return commandCompleted;
}

#else  // ANKI_ROBOT_TEST_ENABLED

RobotTest::RobotTest(const CozmoContext* context) {}

RobotTest::~RobotTest() {}

void RobotTest::Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService)
{
}

void RobotTest::Update()
{
}

#endif  // (else) ANKI_ROBOT_TEST_ENABLED

} // namespace Vector
} // namespace Anki
