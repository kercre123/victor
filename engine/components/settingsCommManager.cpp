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
#include "engine/components/jdocsManager.h"
#include "engine/components/settingsManager.h"
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"

#include "util/console/consoleInterface.h"


#define LOG_CHANNEL "SettingsCommManager"

namespace Anki {
namespace Cozmo {


namespace
{
  SettingsCommManager* s_SettingsCommManager = nullptr;

#if REMOTE_CONSOLE_ENABLED

  static const char* kConsoleGroup = "RobotSettings";
  static const bool kUpdateSettingsJdoc = true;

  // NOTE: Need to keep kMasterVolumeLevels in sync with MasterVolume in robotSettings.clad
  constexpr const char* kMasterVolumeLevels = "Mute,Low,MediumLow,Medium,MediumHigh,High";
  CONSOLE_VAR_ENUM(u8, kMasterVolumeLevel, kConsoleGroup, 0, kMasterVolumeLevels);
  void DebugSetMasterVolume(ConsoleFunctionContextRef context)
  {
    const std::string& volumeSettingValue = MasterVolumeToString(static_cast<MasterVolume>(kMasterVolumeLevel));
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::master_volume,
                                                           Json::Value(volumeSettingValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetMasterVolume, kConsoleGroup);

  // NOTE: Need to keep kEyeColors in sync with EyeColor in robotSettings.clad
  constexpr const char* kEyeColors = "Teal,Orange,Yellow,LimeGreen,AzureBlue,Purple,MatrixGreen";
  CONSOLE_VAR_ENUM(u8, kEyeColor, kConsoleGroup, 0, kEyeColors);
  void DebugSetEyeColor(ConsoleFunctionContextRef context)
  {
    const std::string& eyeColorValue = EyeColorToString(static_cast<EyeColor>(kEyeColor));
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::eye_color,
                                                           Json::Value(eyeColorValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetEyeColor, kConsoleGroup);

  void DebugSetLocale(ConsoleFunctionContextRef context)
  {
    const std::string& localeValue = ConsoleArg_Get_String(context, "localeValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::locale,
                                                           Json::Value(localeValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetLocale, kConsoleGroup, const char* localeValue);

  void DebugSetTimeZone(ConsoleFunctionContextRef context)
  {
    const std::string& timeZoneValue = ConsoleArg_Get_String(context, "timeZoneValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::time_zone,
                                                           Json::Value(timeZoneValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetTimeZone, kConsoleGroup, const char* timeZoneValue);

  void DebugSetDefaultLocation(ConsoleFunctionContextRef context)
  {
    const std::string& defaultLocationValue = ConsoleArg_Get_String(context, "defaultLocationValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::default_location,
                                                           Json::Value(defaultLocationValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetDefaultLocation, kConsoleGroup, const char* defaultLocationValue);

  void DebugToggle24HourClock(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(RobotSetting::clock_24_hour);
  }
  CONSOLE_FUNC(DebugToggle24HourClock, kConsoleGroup);

  void DebugToggleTempIsFahrenheit(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(RobotSetting::temp_is_fahrenheit);
  }
  CONSOLE_FUNC(DebugToggleTempIsFahrenheit, kConsoleGroup);

  void DebugToggleDistIsMetric(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(RobotSetting::dist_is_metric);
  }
  CONSOLE_FUNC(DebugToggleDistIsMetric, kConsoleGroup);

  // For PR demo, this extra console var is used to initialize the 'locale' menu,
  // which is not one-to-one with locale...
  CONSOLE_VAR(s32, kDebugDemoLocaleIndex, kConsoleGroup, 0);

  // This is really a convenience function for the PR demo; also, otherwise we'd have to
  // implement bool console vars for the bool settings and then poll them for changes
  void DebugDemoSetLocaleIndex(ConsoleFunctionContextRef context)
  {
    const int localeIndex = ConsoleArg_Get_Int(context, "localeIndex");
    LOG_INFO("SettingsCommManager.DebugDemoSetLocaleIndex", "Demo Locale index set to %i", localeIndex);

    static const size_t kNumLocales = 4;
    // Note below: the last item is for Canada but we use en-US for locale
    static const std::string locales[kNumLocales] = {"en-US", "en-GB", "en-AU", "en-US"};
    const std::string localeValue = locales[localeIndex];
    LOG_INFO("SettingsCommManager.DebugDemoSetLocaleIndex", "Demo Locale set to %s", localeValue.c_str());
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::locale,
                                                           Json::Value(localeValue));

    static const bool isFahrenheitFlags[kNumLocales] = {true, false, false, false};
    const bool isFahrenheit = isFahrenheitFlags[localeIndex];
    s_SettingsCommManager->HandleRobotSettingChangeRequest(RobotSetting::temp_is_fahrenheit,
                                                           Json::Value(isFahrenheit));
    kDebugDemoLocaleIndex = localeIndex;
  }
  CONSOLE_FUNC(DebugDemoSetLocaleIndex, kConsoleGroup, int localeIndex);

#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsCommManager::SettingsCommManager()
: IDependencyManagedComponent(this, RobotComponentID::SettingsCommManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  s_SettingsCommManager = this;
  _settingsManager = &robot->GetComponent<SettingsManager>();
  _jdocsManager = &robot->GetComponent<JdocsManager>();
  _gatewayInterface = robot->GetGatewayInterface();
  auto* gi = _gatewayInterface;
  if (gi != nullptr)
  {
    auto commonCallback = std::bind(&SettingsCommManager::HandleEvents, this, std::placeholders::_1);
    // Subscribe to desired simple events
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPullSettingsRequest,   commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPushSettingsRequest,   commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kUpdateSettingsRequest, commonCallback));
  }

#if REMOTE_CONSOLE_ENABLED
  // HACK:  Fill in a special debug console var used in the PR demo (related to locale and temperature units)
  const auto& localeSetting = _settingsManager->GetRobotSettingAsString(RobotSetting::locale);
  const auto& isFahrenheitSetting = _settingsManager->GetRobotSettingAsBool(RobotSetting::temp_is_fahrenheit);
  if (localeSetting == "en-US")
  {
    // Set US or Canada based on fahrenheit setting
    kDebugDemoLocaleIndex = isFahrenheitSetting ? 0 : 3;
  }
  else if (localeSetting == "en-GB")
  {
    kDebugDemoLocaleIndex = 1;
  }
  else if (localeSetting == "en-AU")
  {
    kDebugDemoLocaleIndex = 2;
  }
  else
  {
    LOG_WARNING("SettingsCommManager.InitDependent.SetSpecialLocaleIndexForDemo",
                "Unsupported locale setting %s", localeSetting.c_str());
  }
#endif

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::UpdateDependent(const RobotCompMap& dependentComps)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::HandleRobotSettingChangeRequest(const RobotSetting robotSetting,
                                                          const Json::Value& settingJson,
                                                          const bool updateSettingsJdoc)
{
  // Change the robot setting and apply the change
  const bool success = _settingsManager->SetRobotSetting(robotSetting, settingJson, updateSettingsJdoc);
  if (!success)
  {
    LOG_ERROR("SettingsCommManager.HandleRobotSettingChangeRequest",
              "Error setting key %s to value %s", EnumToString(robotSetting), settingJson.asString().c_str());
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::ToggleRobotSettingHelper(const RobotSetting robotSetting)
{
#if REMOTE_CONSOLE_ENABLED
  const bool curSetting = _settingsManager->GetRobotSettingAsBool(robotSetting);
  return HandleRobotSettingChangeRequest(robotSetting, Json::Value(!curSetting), kUpdateSettingsJdoc);
#endif
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::RefreshConsoleVars()
{
#if REMOTE_CONSOLE_ENABLED
  const auto& masterVolumeValue = _settingsManager->GetRobotSettingAsString(RobotSetting::master_volume);
  MasterVolume masterVolume;
  if (EnumFromString(masterVolumeValue, masterVolume))
  {
    kMasterVolumeLevel = static_cast<u8>(masterVolume);
  }
  const auto& eyeColorValue = _settingsManager->GetRobotSettingAsString(RobotSetting::eye_color);
  EyeColor eyeColor;
  if (EnumFromString(eyeColorValue, eyeColor))
  {
    kEyeColor = static_cast<u8>(eyeColor);
  }
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  switch(event.GetData().GetTag())
  {
    case external_interface::GatewayWrapperTag::kPullSettingsRequest:
      OnRequestPullSettings(event.GetData().pull_settings_request());
      break;
    case external_interface::GatewayWrapperTag::kPushSettingsRequest:
      OnRequestPushSettings(event.GetData().push_settings_request());
      break;
    case external_interface::GatewayWrapperTag::kUpdateSettingsRequest:
      OnRequestUpdateSettings(event.GetData().update_settings_request());
      break;
    default:
      LOG_ERROR("SettingsCommManager.HandleEvents",
                "HandleEvents called for unknown message");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestPullSettings(const external_interface::PullSettingsRequest& pullSettingsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestPullSettings", "Pull settings request");
  auto* pullSettingsResp = new external_interface::PullSettingsResponse();
  auto* jdoc = pullSettingsResp->add_doc();
  _jdocsManager->GetJdoc(external_interface::JdocType::ROBOT_SETTINGS, *jdoc);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pullSettingsResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestPushSettings(const external_interface::PushSettingsRequest& pushSettingsRequest)
{
  // TODO
  LOG_INFO("SettingsCommManager.OnRequestPushSettings", "Push settings request");

  auto* pushSettingsResp = new external_interface::PushSettingsResponse();

  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pushSettingsResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestUpdateSettings(const external_interface::UpdateSettingsRequest& updateSettingsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestUpdateSettings", "Update settings request");
  const auto& settings = updateSettingsRequest.settings();
  bool updateSettingsJdoc = false;

  if (settings.oneof_clock_24_hour_case() == external_interface::RobotSettingsConfig::OneofClock24HourCase::kClock24Hour)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::clock_24_hour,
                                                          Json::Value(settings.clock_24_hour()));
  }
  if (settings.oneof_eye_color_case() == external_interface::RobotSettingsConfig::OneofEyeColorCase::kEyeColor)
  {
    // Cast the protobuf3 enum to our CLAD enum, then call into CLAD to get the string version
    // todo:  replace the clad enum completely, when we get protobuf versions of enum-to-string, string-to-enum
    const std::string& eyeColor = EnumToString(static_cast<EyeColor>(settings.eye_color()));
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::eye_color,
                                                          Json::Value(eyeColor));
  }
  if (settings.oneof_default_location_case() == external_interface::RobotSettingsConfig::OneofDefaultLocationCase::kDefaultLocation)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::default_location,
                                                          Json::Value(settings.default_location()));
  }
  if (settings.oneof_dist_is_metric_case() == external_interface::RobotSettingsConfig::OneofDistIsMetricCase::kDistIsMetric)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::dist_is_metric,
                                                          Json::Value(settings.dist_is_metric()));
  }
  if (settings.oneof_locale_case() ==  external_interface::RobotSettingsConfig::OneofLocaleCase::kLocale)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::locale,
                                                          Json::Value(settings.locale()));
  }
  if (settings.oneof_master_volume_case() == external_interface::RobotSettingsConfig::OneofMasterVolumeCase::kMasterVolume)
  {
    // Cast the protobuf3 enum to our CLAD enum, then call into CLAD to get the string version
    // todo:  replace the clad enum completely, when we get protobuf versions of enum-to-string, string-to-enum
    const std::string masterVolume = EnumToString(static_cast<MasterVolume>(settings.master_volume()));
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::master_volume,
                                                          Json::Value(masterVolume));
  }
  if (settings.oneof_temp_is_fahrenheit_case() == external_interface::RobotSettingsConfig::OneofTempIsFahrenheitCase::kTempIsFahrenheit)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest(RobotSetting::temp_is_fahrenheit,
                                                          Json::Value(settings.temp_is_fahrenheit()));
  }
  if (settings.oneof_time_zone_case() == external_interface::RobotSettingsConfig::OneofTimeZoneCase::kTimeZone)
  {
    updateSettingsJdoc |= HandleRobotSettingChangeRequest((RobotSetting::time_zone),
                                                          Json::Value(settings.time_zone()));
  }

  // The request can handle multiple settings changes, but we only update the jdoc once, for efficiency
  if (updateSettingsJdoc)
  {
    _settingsManager->UpdateSettingsJdoc();
  }

  auto* response = new external_interface::UpdateSettingsResponse();
  auto* jdoc = new external_interface::Jdoc();
  _jdocsManager->GetJdoc(external_interface::JdocType::ROBOT_SETTINGS, *jdoc);
  response->set_allocated_doc(jdoc);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(response));
}


} // namespace Cozmo
} // namespace Anki
