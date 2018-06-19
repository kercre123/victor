/**
* File: settingsCommManager.cpp
*
* Author: Paul Terry
* Created: 6/8/18
*
 * Description: Communicates settings with App and Cloud; calls into SettingsManager
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/components/settingsCommManager.h"

#include "engine/robot.h"
#include "engine/components/settingsManager.h"

#include "util/console/consoleInterface.h"

// Log options
#define LOG_CHANNEL "SettingsCommManager"

namespace Anki {
namespace Cozmo {


namespace
{
  SettingsCommManager* s_SettingsCommManager = nullptr;

  static const char* kConsoleGroup = "RobotSettings";

#if REMOTE_CONSOLE_ENABLED

  void DebugSetMasterVolume(ConsoleFunctionContextRef context)
  {
    const std::string volumeSettingValue = ConsoleArg_Get_String(context, "volumeSettingValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.MasterVolume", volumeSettingValue);
  }
  CONSOLE_FUNC(DebugSetMasterVolume, kConsoleGroup, const char* volumeSettingValue);

  void DebugSetEyeColor(ConsoleFunctionContextRef context)
  {
    const std::string eyeColorValue = ConsoleArg_Get_String(context, "eyeColorValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.EyeColor", eyeColorValue);
  }
  CONSOLE_FUNC(DebugSetEyeColor, kConsoleGroup, const char* eyeColorValue);

#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
SettingsCommManager::SettingsCommManager()
: IDependencyManagedComponent(this, RobotComponentID::SettingsCommManager)
{
}


// TODO:  Does this need to be registered somehow such that a bunch of other things are initialized before it?
// (e.g. to ensure audio system is up, before we set volume settings in this InitDependent call.)
// Another strategy is to delay setting the settings until the first UpdateDependent call...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  s_SettingsCommManager = this;
  _settingsManager = &robot->GetComponent<SettingsManager>();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::UpdateDependent(const RobotCompMap& dependentComps)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::HandleRobotSettingChangeRequest(const std::string& settingKey, const std::string& settingValue)
{
  // Change the robot setting and apply the change
  const bool success = _settingsManager->SetRobotSetting(settingKey, settingValue);
  if (!success)
  {
    LOG_ERROR("SettingsCommManager.HandleRobotSettingChangeRequest",
              "Error setting key %s to value %s", settingKey.c_str(), settingValue.c_str());
  }

  // TODO Update the jdoc (maybe only if successful?)
  // TODO Send message to App to acknowledge the change (and whether successful?)
  // TODO Potentially send message to Cloud (only if successful?)
  return true;
}

// TODO:  Message handlers from App
// TODO:  Message handlers from Cloud

} // namespace Cozmo
} // namespace Anki
