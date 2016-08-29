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
#include "clad/types/imageTypes.h"
#include "clad/types/vizTypes.h"
#include "clad/vizInterface/messageViz.h"
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
static const char* kSaveImagesFieldName = "saveImages";
  
WebotsDevLogController::WebotsDevLogController(int32_t stepTime_ms)
: _stepTime_ms(stepTime_ms)
, _supervisor(new webots::Supervisor())
, _devLogProcessor()
, _vizConnection(new UdpClient())
, _selfNode(_supervisor->getSelf())
, _savingImages(false)
{
  ASSERT_NAMED(nullptr != _selfNode, "WebotsDevLogController.Constructor.SelfNodeMissing");
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
    PRINT_NAMED_ERROR("WebotsDevLogController.Update.StepFailed", "");
    return -1;
  }
  return 0;
}
  
std::string WebotsDevLogController::GetDirectoryPath() const
{
  std::string dirPath;
  
  webots::Field* logNameField = _selfNode->getField(kLogsDirectoryFieldName);
  if (nullptr == logNameField)
  {
    PRINT_NAMED_ERROR("WebotsDevLogController.GetDirectoryPath.MissingDataField",
                      "Name: %s", kLogsDirectoryFieldName);
  }
  else
  {
    dirPath = logNameField->getSFString();
  }
  
  return dirPath;
}

void WebotsDevLogController::EnableSaveImagesIfChecked()
{
  const webots::Field* saveImagesField = _selfNode->getField(kSaveImagesFieldName);
  if (nullptr == saveImagesField)
  {
    PRINT_NAMED_ERROR("WebotsDevLogController.ToggleImageSaving.MissingSaveImagesField",
                      "Name: %s", kSaveImagesFieldName);
  }
  else
  {
    EnableSaveImages(saveImagesField->getSFBool());
  }
}
  
void WebotsDevLogController::EnableSaveImages(bool enable)
{
  if(enable == _savingImages)
  {
    // Nothing to do, already in correct mode
    return;
  }
  
  ImageSendMode mode = ImageSendMode::Off;
  if(enable)
  {
    mode = ImageSendMode::Stream;
    _savingImages = true;
  }
  else
  {
    _savingImages = false;
  }

  // Save images to "savedVizImages" in log directory
  std::string path = Util::FileUtils::FullFilePath({_devLogProcessor->GetDirectoryName(), "savedImages"});
  
  VizInterface::MessageViz message(VizInterface::SaveImages(mode, path));
  
  const size_t MAX_MESSAGE_SIZE{(size_t)VizConstants::MaxMessageSize};
  uint8_t buffer[MAX_MESSAGE_SIZE]{0};
  
  const size_t numWritten = (uint32_t)message.Pack(buffer, MAX_MESSAGE_SIZE);
  
  if (_vizConnection->Send((const char*)buffer, (int)numWritten) <= 0) {
    PRINT_NAMED_WARNING("VizManager.SendMessage.Fail", "Send vizMsgID %s of size %zd failed\n", VizInterface::MessageVizTagToString(message.GetTag()), numWritten);
  }
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
  
    switch(key)
    {
      case(int)'I':
      {
        // Toggle save state:
        EnableSaveImages(!_savingImages);
        
        // Make field in object tree match new state (EnableSaveImages changes _savingImages)
        webots::Field* saveImagesField = _selfNode->getField(kSaveImagesFieldName);
        if (nullptr == saveImagesField)
        {
          PRINT_NAMED_ERROR("WebotsDevLogController.ToggleImageSaving.MissingSaveImagesField",
                            "Name: %s", kSaveImagesFieldName);
        }
        else
        {
          saveImagesField->setSFBool(_savingImages);
        }
        
        break;
      }
        
      case (int)'L':
      {
        std::string dirPath = GetDirectoryPath();
        if(!dirPath.empty())
        {
          InitDevLogProcessor(dirPath);
        }
        break;
      }
     
    } // switch(key)

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
  
  // Initialize saveImages to on if box is already checked
  EnableSaveImagesIfChecked();
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

  // If log directory is already specified when we start, just go ahead and use it,
  // without needing to press 'L' key
  std::string dirPath = webotsCtrlDevLog.GetDirectoryPath();
  if(!dirPath.empty())
  {
    webotsCtrlDevLog.Update(); // Tick once first 
    webotsCtrlDevLog.InitDevLogProcessor(dirPath);
  }
  
  while (webotsCtrlDevLog.Update() == 0) { }

  return 0;
}
