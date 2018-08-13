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
namespace Vector {


namespace
{
  SettingsCommManager* s_SettingsCommManager = nullptr;
  static const bool kUpdateSettingsJdoc = true;

#if REMOTE_CONSOLE_ENABLED

  static const char* kConsoleGroup = "RobotSettings";

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
  constexpr const char* kEyeColors = "TipOverTeal,OverfitOrange,UncannyYellow,NonLinearLime,SingularitySapphire,FalsePositivePurple,ConfusionMatrixGreen";
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
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPullJdocsRequest,      commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPushJdocsRequest,      commonCallback));
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
  const bool curSetting = _settingsManager->GetRobotSettingAsBool(robotSetting);
  return HandleRobotSettingChangeRequest(robotSetting, Json::Value(!curSetting), kUpdateSettingsJdoc);
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
    case external_interface::GatewayWrapperTag::kPullJdocsRequest:
      OnRequestPullJdocs(event.GetData().pull_jdocs_request());
      break;
    case external_interface::GatewayWrapperTag::kPushJdocsRequest:
      OnRequestPushJdocs(event.GetData().push_jdocs_request());
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
void SettingsCommManager::OnRequestPullJdocs(const external_interface::PullJdocsRequest& pullJdocsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestPullJdocs", "Pull Jdocs request");
  const auto numDocsRequested = pullJdocsRequest.jdoc_types_size();
  auto* pullJdocsResp = new external_interface::PullJdocsResponse();
  for (int i = 0; i < numDocsRequested; i++)
  {
    auto jdocType = pullJdocsRequest.jdoc_types(i);
    auto* namedJdoc = pullJdocsResp->add_named_jdocs();
    namedJdoc->set_jdoc_type(jdocType);
    _jdocsManager->GetJdoc(jdocType, *namedJdoc->mutable_doc());
  }
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pullJdocsResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestPushJdocs(const external_interface::PushJdocsRequest& pushJdocsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestPushJdocs", "Push Jdocs request");
  const auto numDocsBeingPushed = pushJdocsRequest.named_jdocs_size();
  for (int i = 0; i < numDocsBeingPushed; i++)
  {
    const auto& namedJdoc = pushJdocsRequest.named_jdocs(i);
    const auto& jdocType = namedJdoc.jdoc_type();
    static const bool saveToDiskImmediately = true;
    // TOOD: Pass in/resolve version number, etc.

    // Convert the single jdoc STRING to a JSON::Value object
    Json::Reader reader;
    Json::Value docBodyJson;
    bool success = reader.parse(namedJdoc.doc().json_doc(), docBodyJson);
    if (!success)
    {
      LOG_ERROR("SettingsCommManager.OnRequestPushJdocs.JsonError",
                "Error in parsing JSON string in body of jdoc being pushed to robot");
    }
    if (jdocType == external_interface::ROBOT_SETTINGS)
    {
      LOG_WARNING("SettingsCommManager.OnRequestPushJdocs.PushDirectionIssue",
                  "WARNING: robot settings jdoc is being pushed to robot");
    }
    else if (jdocType == external_interface::ROBOT_LIFETIME_STATS)
    {
      LOG_WARNING("SettingsCommManager.OnRequestPushJdocs.PushDirectionIssue",
                  "WARNING: robot lifetime stats jdoc is being pushed to robot");
    }
    _jdocsManager->UpdateJdoc(jdocType, &docBodyJson, saveToDiskImmediately);
  }
  auto* pushJdocsResp = new external_interface::PushJdocsResponse();
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pushJdocsResp));
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


} // namespace Vector
} // namespace Anki
