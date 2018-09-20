/**
 * File: settingsCommManager.cpp
 *
 * Author: Paul Terry
 * Created: 6/8/18
 *
 * Description: Communicates settings with App and Cloud; calls into SettingsManager
 * (for robot settings), AccountSettingsManager and UserEntitlementsManager
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/components/settingsCommManager.h"

#include "engine/robot.h"
#include "engine/components/accountSettingsManager.h"
#include "engine/components/jdocsManager.h"
#include "engine/components/settingsManager.h"
#include "engine/components/userEntitlementsManager.h"
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/robotInterface/messageHandler.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#define LOG_CHANNEL "SettingsCommManager"

namespace Anki {
namespace Vector {


namespace
{
  SettingsCommManager* s_SettingsCommManager = nullptr;
  static const bool kUpdateSettingsJdoc = true;
  static const bool kUpdateAccountSettingsJdoc = true;
  static const bool kUpdateUserEntitlementsJdoc = true;

#if REMOTE_CONSOLE_ENABLED

  static const char* kConsoleGroup = "RobotSettings";
  static const char* kConsoleGroupAccountSettings = "AccountSettings";
  static const char* kConsoleGroupUserEntitlements = "UserEntitlements";

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ROBOT SETTINGS console vars and functions:

  // NOTE: Need to keep kMasterVolumeLevels in sync with enum Volume in settings.proto
  constexpr const char* kMasterVolumeLevels = "MUTE,LOW,MEDIUM_LOW,MEDIUM,MEDIUM_HIGH,HIGH";
  CONSOLE_VAR_ENUM(u8, kMasterVolumeLevel, kConsoleGroup, 0, kMasterVolumeLevels);
  void DebugSetMasterVolume(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::master_volume,
                                                           Json::Value(kMasterVolumeLevel),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetMasterVolume, kConsoleGroup);

  // NOTE: Need to keep kEyeColors in sync with enum EyeColor in settings.proto
  constexpr const char* kEyeColors = "TIP_OVER_TEAL,OVERFIT_ORANGE,UNCANNY_YELLOW,NON_LINEAR_LIME,SINGULARITY_SAPPHIRE,FALSE_POSITIVE_PURPLE,CONFUSION_MATRIX_GREEN";
  CONSOLE_VAR_ENUM(u8, kEyeColor, kConsoleGroup, 0, kEyeColors);
  void DebugSetEyeColor(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::eye_color,
                                                           Json::Value(kEyeColor),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetEyeColor, kConsoleGroup);

  void DebugSetLocale(ConsoleFunctionContextRef context)
  {
    const std::string& localeValue = ConsoleArg_Get_String(context, "localeValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::locale,
                                                           Json::Value(localeValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetLocale, kConsoleGroup, const char* localeValue);

  void DebugSetTimeZone(ConsoleFunctionContextRef context)
  {
    const std::string& timeZoneValue = ConsoleArg_Get_String(context, "timeZoneValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::time_zone,
                                                           Json::Value(timeZoneValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetTimeZone, kConsoleGroup, const char* timeZoneValue);

  void DebugSetDefaultLocation(ConsoleFunctionContextRef context)
  {
    const std::string& defaultLocationValue = ConsoleArg_Get_String(context, "defaultLocationValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::default_location,
                                                           Json::Value(defaultLocationValue),
                                                           kUpdateSettingsJdoc);
  }
  CONSOLE_FUNC(DebugSetDefaultLocation, kConsoleGroup, const char* defaultLocationValue);

  void DebugToggle24HourClock(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(external_interface::RobotSetting::clock_24_hour);
  }
  CONSOLE_FUNC(DebugToggle24HourClock, kConsoleGroup);

  void DebugToggleTempIsFahrenheit(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(external_interface::RobotSetting::temp_is_fahrenheit);
  }
  CONSOLE_FUNC(DebugToggleTempIsFahrenheit, kConsoleGroup);

  void DebugToggleDistIsMetric(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleRobotSettingHelper(external_interface::RobotSetting::dist_is_metric);
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
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::locale,
                                                           Json::Value(localeValue));

    static const bool isFahrenheitFlags[kNumLocales] = {true, false, false, false};
    const bool isFahrenheit = isFahrenheitFlags[localeIndex];
    s_SettingsCommManager->HandleRobotSettingChangeRequest(external_interface::RobotSetting::temp_is_fahrenheit,
                                                           Json::Value(isFahrenheit),
                                                           kUpdateSettingsJdoc);
    kDebugDemoLocaleIndex = localeIndex;
  }
  CONSOLE_FUNC(DebugDemoSetLocaleIndex, kConsoleGroup, int localeIndex);


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ACCOUNT SETTINGS console vars and functions:

  void DebugToggleDataCollection(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleAccountSettingHelper(external_interface::AccountSetting::DATA_COLLECTION);
  }
  CONSOLE_FUNC(DebugToggleDataCollection, kConsoleGroupAccountSettings);


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // USER ENTITLEMENTS console vars and functions:

  void DebugToggleKickstarterEyes(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->ToggleUserEntitlementHelper(external_interface::UserEntitlement::KICKSTARTER_EYES);
  }
  CONSOLE_FUNC(DebugToggleKickstarterEyes, kConsoleGroupUserEntitlements);

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
  _robot = robot;
  s_SettingsCommManager = this;
  _settingsManager = &robot->GetComponent<SettingsManager>();
  _accountSettingsManager = &robot->GetComponent<AccountSettingsManager>();
  _userEntitlementsManager = &robot->GetComponent<UserEntitlementsManager>();
  _jdocsManager = &robot->GetComponent<JdocsManager>();
  _gatewayInterface = robot->GetGatewayInterface();
  auto* gi = _gatewayInterface;
  if (gi != nullptr)
  {
    auto commonCallback = std::bind(&SettingsCommManager::HandleEvents, this, std::placeholders::_1);
    // Subscribe to desired simple events
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPullJdocsRequest,              commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kUpdateSettingsRequest,         commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kUpdateAccountSettingsRequest,  commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kUpdateUserEntitlementsRequest, commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kCheckCloudRequest,             commonCallback));
  }
  auto* messageHandler = robot->GetRobotMessageHandler();
  _signalHandles.push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::reportCloudConnectivity, [this](const AnkiEvent<RobotInterface::RobotToEngine>& event) {
    const auto code = static_cast<int>(event.GetData().Get_reportCloudConnectivity().code);
    const auto numPackets = event.GetData().Get_reportCloudConnectivity().numPackets;
    const auto expectedPackets = event.GetData().Get_reportCloudConnectivity().expectedPackets;
    LOG_INFO("SettingsCommManager.RcvdConnectionStatus", "Engine received cloud connectivity check code %i, packets received %i, packets expected %i",
             code, numPackets, expectedPackets);
    auto* response = new external_interface::CheckCloudResponse();
    // The proto enum set has a UNKNOWN at 0, and the rest are the same as the CLAD enum
    const auto protoVersionOfCodeEnum = code + 1;
    response->set_code(static_cast<::Anki::Vector::external_interface::CheckCloudResponse_ConnectionCode>(protoVersionOfCodeEnum));
    response->set_num_packets(static_cast<::google::protobuf::int32>(numPackets));
    response->set_expected_packets(static_cast<::google::protobuf::int32>(expectedPackets));
    _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(response));
  }));

#if REMOTE_CONSOLE_ENABLED
  // HACK:  Fill in a special debug console var used in the PR demo (related to locale and temperature units)
  const auto& localeSetting = _settingsManager->GetRobotSettingAsString(external_interface::RobotSetting::locale);
  const auto& isFahrenheitSetting = _settingsManager->GetRobotSettingAsBool(external_interface::RobotSetting::temp_is_fahrenheit);
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
bool SettingsCommManager::HandleRobotSettingChangeRequest(const external_interface::RobotSetting robotSetting,
                                                          const Json::Value& settingJson,
                                                          const bool updateSettingsJdoc)
{
  // Change the robot setting and apply the change
  bool ignoredDueToNoChange = false;
  const bool success = _settingsManager->SetRobotSetting(robotSetting, settingJson,
                                                         updateSettingsJdoc, ignoredDueToNoChange);
  if (!success)
  {
    if (!ignoredDueToNoChange)
    {
      LOG_ERROR("SettingsCommManager.HandleRobotSettingChangeRequest",
                "Error setting key %s to value %s",
                RobotSetting_Name(robotSetting).c_str(), settingJson.asString().c_str());
    }
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::ToggleRobotSettingHelper(const external_interface::RobotSetting robotSetting)
{
  const bool curSetting = _settingsManager->GetRobotSettingAsBool(robotSetting);
  return HandleRobotSettingChangeRequest(robotSetting, Json::Value(!curSetting), kUpdateSettingsJdoc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::HandleAccountSettingChangeRequest(const external_interface::AccountSetting accountSetting,
                                                            const Json::Value& settingJson,
                                                            const bool updateSettingsJdoc)
{
  // Change the account setting and apply the change
  bool ignoredDueToNoChange = false;
  const bool success = _accountSettingsManager->SetAccountSetting(accountSetting, settingJson,
                                                                  updateSettingsJdoc, ignoredDueToNoChange);
  if (!success)
  {
    if (!ignoredDueToNoChange)
    {
      LOG_ERROR("SettingsCommManager.HandleAccountSettingChangeRequest",
                "Error setting key %s to value %s", external_interface::AccountSetting_Name(accountSetting).c_str(),
                settingJson.asString().c_str());
    }
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::ToggleAccountSettingHelper(const external_interface::AccountSetting accountSetting)
{
  const bool curSetting = _accountSettingsManager->GetAccountSettingAsBool(accountSetting);
  return HandleAccountSettingChangeRequest(accountSetting, Json::Value(!curSetting), kUpdateAccountSettingsJdoc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::HandleUserEntitlementChangeRequest(const external_interface::UserEntitlement userEntitlement,
                                                             const Json::Value& settingJson,
                                                             const bool updateUserEntitlementsJdoc)
{
  // Change the user entitlement and apply the change
  bool ignoredDueToNoChange = false;
  const bool success = _userEntitlementsManager->SetUserEntitlement(userEntitlement, settingJson,
                                                                    updateUserEntitlementsJdoc, ignoredDueToNoChange);
  if (!success)
  {
    if (!ignoredDueToNoChange)
    {
      LOG_ERROR("SettingsCommManager.HandleUserEntitlementChangeRequest",
                "Error setting key %s to value %s", external_interface::UserEntitlement_Name(userEntitlement).c_str(),
                settingJson.asString().c_str());
    }
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::ToggleUserEntitlementHelper(const external_interface::UserEntitlement userEntitlement)
{
  const bool curSetting = _userEntitlementsManager->GetUserEntitlementAsBool(userEntitlement);
  return HandleUserEntitlementChangeRequest(userEntitlement, Json::Value(!curSetting), kUpdateUserEntitlementsJdoc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::RefreshConsoleVars()
{
#if REMOTE_CONSOLE_ENABLED
  const auto& masterVolumeValue = _settingsManager->GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
  kMasterVolumeLevel = static_cast<u8>(masterVolumeValue);

  const auto& eyeColorValue = _settingsManager->GetRobotSettingAsUInt(external_interface::RobotSetting::eye_color);
  kEyeColor = static_cast<u8>(eyeColorValue);
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
    case external_interface::GatewayWrapperTag::kUpdateSettingsRequest:
      OnRequestUpdateSettings(event.GetData().update_settings_request());
      break;
    case external_interface::GatewayWrapperTag::kUpdateAccountSettingsRequest:
      OnRequestUpdateAccountSettings(event.GetData().update_account_settings_request());
      break;
    case external_interface::GatewayWrapperTag::kUpdateUserEntitlementsRequest:
      OnRequestUpdateUserEntitlements(event.GetData().update_user_entitlements_request());
      break;
    case external_interface::GatewayWrapperTag::kCheckCloudRequest:
      OnRequestCheckCloud(event.GetData().check_cloud_request());
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
void SettingsCommManager::OnRequestUpdateSettings(const external_interface::UpdateSettingsRequest& updateSettingsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestUpdateSettings", "Update settings request");
  const auto& settings = updateSettingsRequest.settings();
  bool updateSettingsJdoc = false;
  bool saveToCloudImmediately = false;

  if (settings.oneof_clock_24_hour_case() == external_interface::RobotSettingsConfig::OneofClock24HourCase::kClock24Hour)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::clock_24_hour,
                                        Json::Value(settings.clock_24_hour())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::clock_24_hour);
    }
  }

  if (settings.oneof_eye_color_case() == external_interface::RobotSettingsConfig::OneofEyeColorCase::kEyeColor)
  {
    const auto eyeColor = static_cast<uint32_t>(settings.eye_color());
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::eye_color,
                                        Json::Value(eyeColor)))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::eye_color);
    }
  }

  if (settings.oneof_default_location_case() == external_interface::RobotSettingsConfig::OneofDefaultLocationCase::kDefaultLocation)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::default_location,
                                        Json::Value(settings.default_location())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::default_location);
    }
  }

  if (settings.oneof_dist_is_metric_case() == external_interface::RobotSettingsConfig::OneofDistIsMetricCase::kDistIsMetric)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::dist_is_metric,
                                        Json::Value(settings.dist_is_metric())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::dist_is_metric);
    }
  }

  if (settings.oneof_locale_case() ==  external_interface::RobotSettingsConfig::OneofLocaleCase::kLocale)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::locale,
                                        Json::Value(settings.locale())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::locale);
    }
  }

  if (settings.oneof_master_volume_case() == external_interface::RobotSettingsConfig::OneofMasterVolumeCase::kMasterVolume)
  {
    const auto masterVolume = static_cast<uint32_t>(settings.master_volume());
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::master_volume,
                                        Json::Value(masterVolume)))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::master_volume);
    }
  }

  if (settings.oneof_temp_is_fahrenheit_case() == external_interface::RobotSettingsConfig::OneofTempIsFahrenheitCase::kTempIsFahrenheit)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::temp_is_fahrenheit,
                                        Json::Value(settings.temp_is_fahrenheit())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::temp_is_fahrenheit);
    }
  }

  if (settings.oneof_time_zone_case() == external_interface::RobotSettingsConfig::OneofTimeZoneCase::kTimeZone)
  {
    if (HandleRobotSettingChangeRequest(external_interface::RobotSetting::time_zone,
                                        Json::Value(settings.time_zone())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _settingsManager->DoesSettingUpdateCloudImmediately(external_interface::RobotSetting::time_zone);
    }
  }

  // The request can handle multiple settings changes, but we only update the jdoc once, for efficiency
  if (updateSettingsJdoc)
  {
    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
    _settingsManager->UpdateSettingsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);

    DASMSG(robot_settings_updated, "robot.settings.updated", "A robot setting(s) was updated");
    DASMSG_SET(s1, "app", "Where changes came from (app or voice)");
    DASMSG_SEND();
  }

  auto* response = new external_interface::UpdateSettingsResponse();
  auto* jdoc = new external_interface::Jdoc();
  _jdocsManager->GetJdoc(external_interface::JdocType::ROBOT_SETTINGS, *jdoc);
  response->set_allocated_doc(jdoc);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(response));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestUpdateAccountSettings(const external_interface::UpdateAccountSettingsRequest& updateAccountSettingsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestUpdateAccountSettings", "Update account settings request");
  const auto& settings = updateAccountSettingsRequest.account_settings();
  bool updateSettingsJdoc = false;
  bool saveToCloudImmediately = false;

  if (settings.oneof_data_collection_case() == external_interface::AccountSettingsConfig::OneofDataCollectionCase::kDataCollection)
  {
    if (HandleAccountSettingChangeRequest(external_interface::AccountSetting::DATA_COLLECTION,
                                          Json::Value(settings.data_collection())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _accountSettingsManager->DoesSettingUpdateCloudImmediately(external_interface::AccountSetting::DATA_COLLECTION);
    }
  }

  if (settings.oneof_app_locale_case() == external_interface::AccountSettingsConfig::OneofAppLocaleCase::kAppLocale)
  {
    if (HandleAccountSettingChangeRequest(external_interface::AccountSetting::APP_LOCALE,
                                          Json::Value(settings.app_locale())))
    {
      updateSettingsJdoc = true;
      saveToCloudImmediately |= _accountSettingsManager->DoesSettingUpdateCloudImmediately(external_interface::AccountSetting::APP_LOCALE);
    }
  }

  // The request can handle multiple settings changes, but we only update the jdoc once, for efficiency
  if (updateSettingsJdoc)
  {
    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
    _accountSettingsManager->UpdateAccountSettingsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);
  }

  auto* response = new external_interface::UpdateAccountSettingsResponse();
  auto* jdoc = new external_interface::Jdoc();
  _jdocsManager->GetJdoc(external_interface::JdocType::ACCOUNT_SETTINGS, *jdoc);
  response->set_allocated_doc(jdoc);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(response));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestUpdateUserEntitlements(const external_interface::UpdateUserEntitlementsRequest& updateUserEntitlementsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestUpdateUserEntitlements", "Update user entitlements request");
  const auto& userEntitlements = updateUserEntitlementsRequest.user_entitlements();
  bool updateUserEntitlementsJdoc = false;
  bool saveToCloudImmediately = false;

  if (userEntitlements.oneof_kickstarter_eyes_case() == external_interface::UserEntitlementsConfig::OneofKickstarterEyesCase::kKickstarterEyes)
  {
    if (HandleUserEntitlementChangeRequest(external_interface::UserEntitlement::KICKSTARTER_EYES,
                                           Json::Value(userEntitlements.kickstarter_eyes())))
    {
      updateUserEntitlementsJdoc = true;
      saveToCloudImmediately |= _userEntitlementsManager->DoesUserEntitlementUpdateCloudImmediately(external_interface::UserEntitlement::KICKSTARTER_EYES);
    }
  }

  // The request can handle multiple changes, but we only update the jdoc once, for efficiency
  if (updateUserEntitlementsJdoc)
  {
    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
    _userEntitlementsManager->UpdateUserEntitlementsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);
  }

  auto* response = new external_interface::UpdateUserEntitlementsResponse();
  auto* jdoc = new external_interface::Jdoc();
  _jdocsManager->GetJdoc(external_interface::JdocType::USER_ENTITLEMENTS, *jdoc);
  response->set_allocated_doc(jdoc);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(response));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestCheckCloud(const external_interface::CheckCloudRequest& checkCloudRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestCheckCloud", "Check cloud connectivity request");
  // Send a CLAD message to anim, requesting connectivity check
  // Note that this path is: app -> gateway -> engine (YOU ARE HERE) -> anim -> cloud ->
  //    actual cloud -> cloud -> anim -> engine -> gateway -> app
  // See "reportCloudConnectivity" in this file
  _robot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::CheckCloudConnectivity{}));
}


} // namespace Vector
} // namespace Anki
