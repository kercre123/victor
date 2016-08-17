/**
* File: webotsCtrlDevLog
*
* Author: Lee Crippen
* Created: 6/21/2016
*
* Description: Webots controller for loading and displaying Cozmo dev logs
*
* Copyright: Anki, inc. 2016
*
*/
#include "webotsCtrlDevLog.h"
#include "anki/cozmo/basestation/debug/devLogProcessor.h"
#include "anki/messaging/shared/UdpClient.h"
#include "clad/types/vizTypes.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/math/numericCast.h"
#include <webots/Supervisor.hpp>

#include <functional>


namespace Anki {
namespace Cozmo {
  
static constexpr auto kDevLogStepTime_ms = 10;
static const char* kLogsDirectoryFieldName = "logsDirectory";
  
WebotsDevLogController::WebotsDevLogController(int32_t stepTime_ms)
: _stepTime_ms(stepTime_ms)
, _supervisor(new webots::Supervisor())
, _devLogProcessor()
, _vizConnection(new UdpClient())
{
  _supervisor->keyboardEnable(stepTime_ms);
  _vizConnection->Connect("127.0.0.1", Util::EnumToUnderlying(VizConstants::VIZ_SERVER_PORT));
}

WebotsDevLogController::~WebotsDevLogController()
{
  if (_vizConnection)
  {
    _vizConnection->Disconnect();
  }
}

int32_t WebotsDevLogController::Update()
{
  UpdateKeyboard();
  
  if (_devLogProcessor)
  {
    // Once we no longer have log data left clear the processor
    if (!_devLogProcessor->AdvanceTime(_stepTime_ms))
    {
      _devLogProcessor.reset();
    }
  }
  
  if (_supervisor->step(_stepTime_ms) == -1)
  {
    PRINT_NAMED_INFO("WebotsDevLogController.Update.StepFailed", "");
    return -1;
  }
  return 0;
}
  
void WebotsDevLogController::UpdateKeyboard()
{
  if (!UpdatePressedKeys())
  {
    return;
  }
  
  for(auto key : _lastKeysPressed)
  {
    // Extract modifier key(s)
    //int modifier_key = key & ~webots::Supervisor::KEYBOARD_KEY;
    
    // Set key to its modifier-less self
    key &= webots::Supervisor::KEYBOARD_KEY;
  
    if ('L' == key)
    {
      const webots::Node* selfNode = _supervisor->getSelf();
      ASSERT_NAMED(nullptr != selfNode, "WebotsDevLogController.UpdateKeyboard.SelfNodeMissing");
      if (nullptr != selfNode)
      {
        webots::Field* logNameField = selfNode->getField(kLogsDirectoryFieldName);
        if (nullptr == logNameField)
        {
          PRINT_NAMED_ERROR("WebotsDevLogController.UpdateKeyboard.MissingDataField", "Name: %s", kLogsDirectoryFieldName);
        }
        else
        {
          InitDevLogProcessor(logNameField->getSFString());
        }
      }
    }
  }
}
  
bool WebotsDevLogController::UpdatePressedKeys()
{
  std::set<int> currentKeysPressed;
  int key = _supervisor->keyboardGetKey();
  while(key != 0)
  {
    currentKeysPressed.insert(key);
    key = _supervisor->keyboardGetKey();
  }
  
  // If exact same keys were pressed last tic, do nothing.
  if (_lastKeysPressed == currentKeysPressed)
  {
    return false;
  }
  
  _lastKeysPressed = currentKeysPressed;
  return true;
}
  
void WebotsDevLogController::InitDevLogProcessor(const std::string& directoryPath)
{
  // We only init the dev log processor when we don't have one and we've been given a valid path.
  // It would be nice to handle loading a new log after having run one already, but the VizController is stateful
  // and we don't yet have a way to clear it before going through another log
  if (_devLogProcessor)
  {
    PRINT_NAMED_INFO("WebotsDevLogController.InitDevLogProcessor", "DevLogProcessor already exists. Ignoring.");
    return;
  }
  
  if (directoryPath.empty() || !Util::FileUtils::DirectoryExists(directoryPath))
  {
    PRINT_NAMED_INFO("WebotsDevLogController.InitDevLogProcessor", "Input directory %s not found.", directoryPath.c_str());
    return;
  }
  
  PRINT_NAMED_INFO("WebotsDevLogController.InitDevLogProcessor", "Loading directory %s", directoryPath.c_str());
  _devLogProcessor.reset(new DevLogProcessor(directoryPath));
  _devLogProcessor->SetVizMessageCallback(std::bind(&WebotsDevLogController::HandleVizData, this, std::placeholders::_1));
  _devLogProcessor->SetPrintCallback(std::bind(&WebotsDevLogController::HandlePrintLines, this, std::placeholders::_1));
}
  
void WebotsDevLogController::HandleVizData(const DevLogReader::LogData& logData)
{
  if (_vizConnection && _vizConnection->IsConnected())
  {
    _vizConnection->Send(reinterpret_cast<const char*>(logData._data.data()), Util::numeric_cast<int>(logData._data.size()));
  }
}
  
void WebotsDevLogController::HandlePrintLines(const DevLogReader::LogData& logData)
{
  std::cout << reinterpret_cast<const char*>(logData._data.data());
}

} // namespace Cozmo
} // namespace Anki


// =======================================================================


int main(int argc, char **argv)
{
  // Note: we don't allow logFiltering in DevLog like we do in the other controllers because this
  // controller is meant to show all logs.

  Anki::Util::PrintfLoggerProvider loggerProvider;
  loggerProvider.SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
  loggerProvider.SetMinToStderrLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_WARN);  
  Anki::Util::gLoggerProvider = &loggerProvider;

  Anki::Cozmo::WebotsDevLogController webotsCtrlDevLog(Anki::Cozmo::kDevLogStepTime_ms);

  while (webotsCtrlDevLog.Update() == 0) { }

  return 0;
}
