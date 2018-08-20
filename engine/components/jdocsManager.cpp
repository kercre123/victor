/**
 * File: jdocsManager.cpp
 *
 * Author: Paul Terry
 * Created: 7/8/18
 *
 * Description: Manages Jdocs, including serializing to robot storage, and
 * talking to the cloud API for jdocs, and processing update requests from
 * various other engine subsystems.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/components/jdocsManager.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#define LOG_CHANNEL "JdocsManager"

namespace Anki {
namespace Vector {


// Set until I get more stuff actualy working
#define DISABLE_JDOCS_CLOUD_INTERACTION

namespace
{
  static const std::string kJdocsManagerFolder = "jdocs";

  static const char* kManagedJdocsKey = "managedJdocs";
  static const char* kSavedOnDiskKey = "savedOnDisk";
  static const char* kDocNameKey = "docName";
  static const char* kDocVersionKey = "doc_version";
  static const char* kFmtVersionKey = "fmt_version";
  static const char* kClientMetadataKey = "client_metadata";
  static const char* kFingerprintKey = "fingerprint"; // for backwards compatibility
  static const char* kJdocKey = "jdoc";
  static const char* kDiskSavePeriodKey = "diskSavePeriod_s";
  static const char* kBodyOwnedByJdocsManagerKey = "bodyOwnedByJdocManager";
  static const char* kWarnOnCloudVersionLaterKey = "warnOnCloudVersionLater";
  static const char* kErrorOnCloudVersionLaterKey = "errorOnCloudVersionLater";
  static const char* kCloudSavePeriodKey = "cloudSavePeriod_s";
  static const char* kJdocFormatVersionKey = "jdocFormatVersion";

  static const std::string emptyString;
  static const Json::Value emptyJson;

  JdocsManager* s_JdocsManager = nullptr;

#if REMOTE_CONSOLE_ENABLED

  static const char* kConsoleGroup = "JdocsManager";

  // Keep this in sync with JodcType enum
  constexpr const char* kJdocTypes = "RobotSettings,RobotLifetimeStats,AccountSettings,UserEntitlements";
  CONSOLE_VAR_ENUM(u8, kJdocType, kConsoleGroup, 0, kJdocTypes);

  void DebugDeleteSelectedJdocInCloud(ConsoleFunctionContextRef context)
  {
    std::string userID, thing;
    s_JdocsManager->GetUserAndThingIDs(userID, thing);
    const auto& docName = s_JdocsManager->GetJdocName(static_cast<external_interface::JdocType>(kJdocType));
    const auto deleteReq = JDocs::DocRequest::CreatedeleteReq(JDocs::DeleteRequest{userID, thing, docName});
    s_JdocsManager->SendJdocsRequest(deleteReq);
  }
  CONSOLE_FUNC(DebugDeleteSelectedJdocInCloud, kConsoleGroup);

  void DebugDeleteAllJdocsInCloud(ConsoleFunctionContextRef context)
  {
    std::string userID, thing;
    s_JdocsManager->GetUserAndThingIDs(userID, thing);
    for (int i = 0; i < external_interface::JdocType_ARRAYSIZE; i++)
    {
      const auto& docName = s_JdocsManager->GetJdocName(static_cast<external_interface::JdocType>(i));
      const auto deleteReq = JDocs::DocRequest::CreatedeleteReq(JDocs::DeleteRequest{userID, thing, docName});
      s_JdocsManager->SendJdocsRequest(deleteReq);
    }
  }
  CONSOLE_FUNC(DebugDeleteAllJdocsInCloud, kConsoleGroup);

#endif  // REMOTE_CONSOLE_ENABLED
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JdocsManager::JdocsManager()
: IDependencyManagedComponent(this, RobotComponentID::JdocsManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JdocsManager::~JdocsManager()
{
  if (_udpClient.IsConnected())
  {
    _udpClient.Disconnect();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  s_JdocsManager = this;

  _platform = robot->GetContextDataPlatform();
  DEV_ASSERT(_platform != nullptr, "JdocsManager.InitDependent.DataPlatformIsNull");

  auto *osstate = OSState::getInstance();
  _thingID = "vic:" + osstate->GetSerialNumberAsString();

  _savePath = _platform->pathToResource(Util::Data::Scope::Persistent, kJdocsManagerFolder);
  if (!Util::FileUtils::CreateDirectory(_savePath))
  {
    LOG_ERROR("JdocsManager.InitDependent.FailedToCreateFolder", "Failed to create folder %s", _savePath.c_str());
    return;
  }

  // Build our jdoc data structure based on the config data, and possible saved jdoc files on disk
  const auto& config = robot->GetContext()->GetDataLoader()->GetJdocsConfig();
  const auto& jdocsConfig = config[kManagedJdocsKey];
  const auto& memberNames = jdocsConfig.getMemberNames();

  for (const auto& name : memberNames)
  {
    JdocType jdocType;
    const bool valid = EnumFromString(name, jdocType);
    if (!valid)
    {
      LOG_ERROR("JdocsManager.InitDependent.InvalidJdocTypeInConfig",
                "Invalid jdoc type %s in jdoc config file; ignoring", name.c_str());
      continue;
    }
    const auto& jdocConfig = jdocsConfig[name];
    const external_interface::JdocType jdocTypeKey = static_cast<external_interface::JdocType>(jdocType);
    if (_jdocs.find(jdocTypeKey) != _jdocs.end())
    {
      LOG_ERROR("JdocsManager.InitDependent.DuplicateJdocTypeInConfig",
                "Duplicate jdoc type %s in jdoc config file; ignoring duplicate", name.c_str());
      continue;
    }
    JdocInfo jdocInfo;
    jdocInfo._jdocVersion = 0;
    jdocInfo._jdocFormatVersion = jdocConfig[kJdocFormatVersionKey].asUInt64();
    jdocInfo._jdocClientMetadata = "";
    jdocInfo._jdocBody = {};
    jdocInfo._jdocName = jdocConfig[kDocNameKey].asString();
    jdocInfo._needsCreation = false;
    jdocInfo._needsMigration = false;
    jdocInfo._savedOnDisk = jdocConfig[kSavedOnDiskKey].asBool();
    jdocInfo._diskFileDirty = false;
    jdocInfo._diskSavePeriod_s = jdocConfig[kDiskSavePeriodKey].asInt();
    jdocInfo._nextDiskSaveTime = jdocInfo._diskSavePeriod_s;
    jdocInfo._bodyOwnedByJM = jdocConfig[kBodyOwnedByJdocsManagerKey].asBool();
    jdocInfo._warnOnCloudVersionLater = jdocConfig[kWarnOnCloudVersionLaterKey].asBool();
    jdocInfo._errorOnCloudVersionLater = jdocConfig[kErrorOnCloudVersionLaterKey].asBool();
    jdocInfo._cloudDirty = false;
    jdocInfo._cloudSavePeriod_s = jdocConfig[kCloudSavePeriodKey].asInt();
    jdocInfo._nextCloudSaveTime = jdocInfo._cloudSavePeriod_s;
    if (jdocInfo._savedOnDisk)
    {
      jdocInfo._jdocFullPath = Util::FileUtils::FullFilePath({_savePath, jdocInfo._jdocName + ".json"});
    }
    jdocInfo._overwrittenCB = nullptr;

    _jdocs[jdocTypeKey] = jdocInfo;

    auto& jdocItem = _jdocs[jdocTypeKey];
    if (jdocItem._savedOnDisk)
    {
      if (Util::FileUtils::FileExists(jdocItem._jdocFullPath))
      {
        if (LoadJdocFile(jdocTypeKey))
        {
          const auto latestFormatVersion = jdocConfig[kJdocFormatVersionKey].asUInt64();
          if (jdocInfo._jdocFormatVersion < latestFormatVersion)
          {
            LOG_INFO("JdocsManager.InitDependent.FormatVersionMigration",
                     "Jdoc %s loaded from disk has older format version (%llu); migrating to %llu",
                     jdocInfo._jdocName.c_str(), jdocInfo._jdocFormatVersion, latestFormatVersion);
            jdocInfo._needsMigration = true;
          }
          else if (jdocInfo._jdocFormatVersion > latestFormatVersion)
          {
            LOG_ERROR("JdocsManager.InitDependent.FormatVersionError",
                      "Jdoc %s loaded from disk has newer format version (%llu) than robot handles (%llu); should not be possible",
                      jdocInfo._jdocName.c_str(), jdocInfo._jdocFormatVersion, latestFormatVersion);
          }
        }
        else
        {
          LOG_ERROR("JdocsManager.InitDependent.ErrorReadingJdocFile",
                    "Error reading jdoc file %s", jdocItem._jdocFullPath.c_str());
          jdocItem._needsCreation = true;
        }
      }
      else
      {
        LOG_WARNING("JdocsManager.InitDependent.NoJdocFile", "Serialized jdoc file not found; to be created by owning subsystem");
        jdocItem._needsCreation = true;
      }
    }
  }

  // Now queue up a reqeust to the jdocs server (vic-cloud) for the userID
  const auto userReq = JDocs::DocRequest::Createuser(Void{});
  SendJdocsRequest(userReq);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::GetUserAndThingIDs(std::string& userID, std::string& thingID) const
{
  userID  = _userID;
  thingID = _thingID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::RegisterOverwriteNotificationCallback(const external_interface::JdocType jdocTypeKey,
                                                         const OverwriteNotificationCallback cb)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.RegisterOverwriteNotificationCallback.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return;
  }
  auto& jdocItem = (*it).second;
  jdocItem._overwrittenCB = cb;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  UpdatePeriodicFileSaves(currTime_s);

  if (!_udpClient.IsConnected())
  {
#ifndef SIMULATOR // vic-cloud jdocs stuff doesn't work on webots yet
    static const float kTimeBetweenConnectionAttempts_s = 1.0f;
    static float nextAttemptTime_s = 0.0f;
    if (currTime_s >= nextAttemptTime_s)
    {
      nextAttemptTime_s = currTime_s + kTimeBetweenConnectionAttempts_s;
      const bool nowConnected = ConnectToJdocsServer();
      if (nowConnected)
      {
        // Now that we're connected, verify that the first jdoc request queued
        // is THE 'get user id' request, and send that one (only).
        if (_unsentDocRequestQueue.empty() ||
            _unsentDocRequestQueue.front().GetTag() != JDocs::DocRequestTag::user)
        {
          LOG_ERROR("JdocsManager.UpdateDependent.QueueError",
                    "First item in unsent queue should be the 'get user id' item");
        }
#ifndef DISABLE_JDOCS_CLOUD_INTERACTION
        SendJdocsRequest(_unsentDocRequestQueue.front());
        _unsentDocRequestQueue.pop();
#endif
      }
    }
#endif
  }

  if (_udpClient.IsConnected())
  {
    if (!_userID.empty())
    {
      UpdatePeriodicCloudSaves(currTime_s);
    }

    UpdateJdocsServerResponses();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::JdocNeedsCreation(const external_interface::JdocType jdocTypeKey) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.JdocNeedsCreation.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }
  const auto& jdocItem = (*it).second;
  return jdocItem._needsCreation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::JdocNeedsMigration(const external_interface::JdocType jdocTypeKey) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.JdocNeedsMigration.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }
  const auto& jdocItem = (*it).second;
  return jdocItem._needsMigration;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& JdocsManager::GetJdocName(const external_interface::JdocType jdocTypeKey) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdocName.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return emptyString;
  }
  const auto& jdocItem = (*it).second;
  return jdocItem._jdocName;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint64_t JdocsManager::GetJdocDocVersion(const external_interface::JdocType jdocTypeKey) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdocDocVersion.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return 0;
  }
  const auto& jdocItem = (*it).second;
  return jdocItem._jdocVersion;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Json::Value& JdocsManager::GetJdocBody(const external_interface::JdocType jdocTypeKey) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdocBody.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return emptyJson;
  }
  const auto& jdocItem = (*it).second;
  return jdocItem._jdocBody;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value* JdocsManager::GetJdocBodyPointer(const external_interface::JdocType jdocTypeKey)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdocBodyPointer.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return nullptr;
  }
  auto& jdocItem = (*it).second;
  if (!jdocItem._bodyOwnedByJM)
  {
    LOG_ERROR("JdocsManager.GetJdocBodyPointer.BodyNotOwnedByJdocsManager",
              "Cannot get jdoc body pointer when body is not owned by jdoc manager");
    return nullptr;
  }
  return &jdocItem._jdocBody;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::GetJdoc(const external_interface::JdocType jdocTypeKey,
                           external_interface::Jdoc& jdocOut) const
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdoc.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }

  const auto& jdocItem = (*it).second;
  jdocOut.set_doc_version(jdocItem._jdocVersion);
  jdocOut.set_fmt_version(jdocItem._jdocFormatVersion);
  jdocOut.set_client_metadata(jdocItem._jdocClientMetadata);
  Json::StyledWriter writer;
  const std::string jdocBodyString = writer.write(jdocItem._jdocBody);
  jdocOut.set_json_doc(jdocBodyString);

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::UpdateJdoc(const external_interface::JdocType jdocTypeKey,
                              const Json::Value* jdocBody,
                              const bool saveToDiskImmediately,
                              const bool saveToCloudImmediately,
                              const bool setCloudDirtyIfNotImmediate)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.GetJdocName.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }
  auto& jdocItem = (*it).second;
  if (jdocBody != nullptr)
  {
    if (jdocItem._bodyOwnedByJM)
    {
      LOG_ERROR("JdocsManager.UpdateJdoc.CannotAcceptJdocBody",
                "Cannot accept jdoc body when body is owned by jdoc manager");
      return false;
    }

    // Copy the jdoc json
    jdocItem._jdocBody = *jdocBody;
  }
  else
  {
    if (!jdocItem._bodyOwnedByJM)
    {
      LOG_ERROR("JdocsManager.UpdateJdoc.MustProvideJdocBody",
                "Must provide jdoc body when body is not owned by jdoc manager");
      return false;
    }
  }

  // TODO: Increment some sort of 'minor version'

  if (saveToCloudImmediately)
  {
    static const bool kIsNewJdocInCloud = false;
    SubmitJdocToCloud(jdocTypeKey, kIsNewJdocInCloud);
  }
  else
  {
    if (setCloudDirtyIfNotImmediate)
    {
      jdocItem._cloudDirty = true;
    }
  }

  if (jdocItem._savedOnDisk)
  {
    if (saveToDiskImmediately)
    {
      SaveJdocFile(jdocTypeKey);
    }
    else
    {
      jdocItem._diskFileDirty = true;
    }
  }
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::ClearJdocBody(const external_interface::JdocType jdocTypeKey)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.ClearJdocBody.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }
  auto& jdocItem = (*it).second;
  if (!jdocItem._bodyOwnedByJM)
  {
    LOG_ERROR("JdocsManager.ClearJdocBody.BodyNotOwnedByJdocsManager",
              "Cannot clear jdoc body when body is not owned by jdoc manager");
    return false;
  }

  jdocItem._jdocBody = Json::Value(Json::objectValue);

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::LoadJdocFile(const external_interface::JdocType jdocTypeKey)
{
  auto& jdocItem = _jdocs[jdocTypeKey];
  Json::Value jdocJson;
  if (!_platform->readAsJson(jdocItem._jdocFullPath, jdocJson))
  {
    LOG_ERROR("JdocsManager.LoadJdocFile.Failed", "Failed to read %s",
              jdocItem._jdocFullPath.c_str());
    return false;
  }

  jdocItem._jdocVersion        = jdocJson[kDocVersionKey].asUInt64();
  jdocItem._jdocFormatVersion  = jdocJson[kFmtVersionKey].asUInt64();
  if (jdocJson.isMember(kClientMetadataKey))
  {
    jdocItem._jdocClientMetadata = jdocJson[kClientMetadataKey].asString();
  }
  else
  {
    // Temp code for backwards compatibility due to field rename
    jdocItem._jdocClientMetadata = jdocJson[kFingerprintKey].asString();
    jdocItem._diskFileDirty = true; // So we write it out with the correct key string
  }
  jdocItem._jdocBody           = jdocJson[kJdocKey];

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::SaveJdocFile(const external_interface::JdocType jdocTypeKey)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  auto& jdocItem = (*it).second;

  Json::Value jdocJson;
  jdocJson[kDocVersionKey]     = jdocItem._jdocVersion;
  jdocJson[kFmtVersionKey]     = jdocItem._jdocFormatVersion;
  jdocJson[kClientMetadataKey] = jdocItem._jdocClientMetadata;
  jdocJson[kJdocKey]           = jdocItem._jdocBody;

  if (!_platform->writeAsJson(jdocItem._jdocFullPath, jdocJson))
  {
    LOG_ERROR("JdocsManager.SaveJdocFile.Failed", "Failed to write jdoc file %s",
              jdocItem._jdocFullPath.c_str());
    return;
  }

  jdocItem._needsCreation = false;
  jdocItem._diskFileDirty = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdatePeriodicFileSaves(const float currTime_s)
{
  for (auto& jdocPair : _jdocs)
  {
    auto& jdoc = jdocPair.second;
    if (jdoc._savedOnDisk && jdoc._diskFileDirty && currTime_s > jdoc._nextDiskSaveTime)
    {
      SaveJdocFile(jdocPair.first);
      jdoc._nextDiskSaveTime = currTime_s + jdoc._diskSavePeriod_s;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::ConnectToJdocsServer()
{
  static const std::string sockName = std::string{LOCAL_SOCKET_PATH} + "jdocs_engine_client";
  static const std::string peerName = std::string{LOCAL_SOCKET_PATH} + "jdocs_server";
  const bool udpSuccess = _udpClient.Connect(sockName, peerName);
  LOG_INFO("JdocsManager.ConnectToJdocsServer.Attempt", "Attempted connection from %s to %s: Result: %s",
           sockName.c_str(), peerName.c_str(), udpSuccess ? "SUCCESS" : "Failed");
  return udpSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::SendJdocsRequest(const JDocs::DocRequest& docRequest)
{
  // If we're not connected to the jdocs server, or we haven't received userID yet,
  // put the request in another queue (on connection, we'll send them)
  if (!_udpClient.IsConnected() ||
      (_userID.empty() && (docRequest.GetTag() != JDocs::DocRequestTag::user)))
  {
    const auto unsentQueueSize = _unsentDocRequestQueue.size();
    static const size_t kMaxUnsentQueueSize = 100;
    if (unsentQueueSize >= kMaxUnsentQueueSize)
    {
      LOG_ERROR("JdocsManager.SendJdocsRequest",
                "Unsent queue size is at max at %zu items; IGNORING jdocs request operation!",
                unsentQueueSize);
      // TODO:  Think later about what we actually want to do in this situation
      // (Running without being logged in)
      return false;
    }

    _unsentDocRequestQueue.push(docRequest);
    LOG_INFO("JdocsManager.SendJdocsRequest",
             "Jdocs server not connected; adding to unsent requests (size now %zu)",
             unsentQueueSize + 1);

    return true;
  }

  const bool sendSuccessful = SendUdpMessage(docRequest);
  if (!sendSuccessful)
  {
    return false;
  }
  LOG_INFO("JdocsManager.SendJdocsRequest", "Sent request with tag %i",
           static_cast<int>(docRequest.GetTag()));
  _docRequestQueue.push(docRequest);
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::SendUdpMessage(const JDocs::DocRequest& msg)
{
  std::vector<uint8_t> buf(msg.Size());
  msg.Pack(buf.data(), buf.size());
  const size_t bytesSent = _udpClient.Send((const char*)buf.data(), buf.size());
  return bytesSent > 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdatePeriodicCloudSaves(const float currTime_s)
{
  for (auto& jdocPair : _jdocs)
  {
    auto& jdoc = jdocPair.second;
    if (jdoc._cloudDirty && currTime_s > jdoc._nextCloudSaveTime)
    {
      jdoc._nextCloudSaveTime = currTime_s + jdoc._cloudSavePeriod_s;

      static const bool kIsNewJdocInCloud = false;
      SubmitJdocToCloud(jdocPair.first, kIsNewJdocInCloud);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdateJdocsServerResponses()
{
  static constexpr size_t kMaxReceiveBytes = 20 * 1024; // must be large enough for receiving all 4 jdocs back
  uint8_t receiveArray[kMaxReceiveBytes];
  const ssize_t bytesReceived = _udpClient.Recv((char*)receiveArray, kMaxReceiveBytes);

  if (bytesReceived > 0)
  {
    DEV_ASSERT(!_docRequestQueue.empty(), "Doc request queue is empty but we're receiving a response");
    const auto& docRequest = _docRequestQueue.front();
    JDocs::DocResponse response{receiveArray, (size_t)bytesReceived};
    bool valid = true;
    switch (response.GetTag())
    {
      case JDocs::DocResponseTag::write:
      {
        HandleWriteResponse(docRequest.Get_write(), response.Get_write());
      }
      break;

      case JDocs::DocResponseTag::read:
      {
        HandleReadResponse(docRequest.Get_read(), response.Get_read());
      }
      break;

      case JDocs::DocResponseTag::deleteResp:
      {
        HandleDeleteResponse(docRequest.Get_deleteReq(), response.Get_deleteResp());
      }
      break;

      case JDocs::DocResponseTag::err:
      {
        HandleErrResponse(response.Get_err());
      }
      break;

      case JDocs::DocResponseTag::user:
      {
        HandleUserResponse(response.Get_user());
      }
      break;

      default:
      {
        LOG_INFO("JdocsManager.UpdateJdocsServerResponses.UnexpectedSignal",
                 "0x%x 0x%x", receiveArray[0], receiveArray[1]);
        valid = false;
      }
      break;
    }

    if (valid)
    {
      _docRequestQueue.pop();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleWriteResponse(const JDocs::WriteRequest& writeRequest, const JDocs::WriteResponse& writeResponse)
{
  LOG_INFO("JdocsManager.HandleWriteResponse", "Received write response for jdoc %s:  Status %s, latest version %llu",
           writeRequest.docName.c_str(), EnumToString(writeResponse.status), writeResponse.latestVersion);
  const auto jdocType = JdocTypeFromDocName(writeRequest.docName);
  auto& jdoc = _jdocs[jdocType];
  if (writeResponse.status == JDocs::WriteStatus::Accepted)
  {
    // Cloud has accepted the new or updated jdoc, and incremented the cloud-managed
    // version number, so update that version number in our jdoc in memory
    jdoc._jdocVersion = writeResponse.latestVersion;

    if (jdoc._savedOnDisk)
    {
      // TODO:  We're also saving to disk (often) just before we submit the write request;
      // this save is needed (because of the changed doc version), but can I make it so we're
      // not saving to disk so much?
      SaveJdocFile(jdocType);
    }
  }
  else if (writeResponse.status == JDocs::WriteStatus::RejectedDocVersion)
  {
    LOG_WARNING("JdocsManager.HandleWriteResponse.RejectedDocVersion",
                "Submitted jdoc's version %llu does not match the version in the cloud (%llu); update not allowed",
                writeRequest.doc.docVersion, writeResponse.latestVersion);
    // TODO Not sure what to do here for now
  }
  else if (writeResponse.status == JDocs::WriteStatus::RejectedFmtVersion)
  {
    // The client format version is less than the server format version; update not allowed
    LOG_ERROR("JodcsManager.HandleWriteResponse.RejectedFmtVersion",
              "Submitted jdoc's format version %llu is less than the format version in the cloud; update not allowed",
              writeRequest.doc.fmtVersion);
    // Not sure what to do here quite yet
  }
  else  // writeResponse.status == JDocs::WriteStatus::Error
  {
    LOG_ERROR("JodcsManager.HandleWriteResponse.Error", "Error returned from write jdoc attempt");
    // Not sure (yet) what to do if we get this
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleReadResponse(const JDocs::ReadRequest& readRequest, const JDocs::ReadResponse& readResponse)
{
  LOG_INFO("JdocsManager.HandleReadResponse.Read", "Received read response");

  // Note: Currently this only happens after startup, when we're getting all 'latest jdocs'
  // ...if/when we do other read requests, we may have to add flags/logic to differentiate
  DEV_ASSERT_MSG(readRequest.items.size() == readResponse.items.size(),
                 "JdocsManager.HandleReadResponse.Mismatch",
                 "Mismatch of number of items in jdocs read request vs. response (%zu vs %zu)",
                 readRequest.items.size(), readResponse.items.size());

  int index = 0;
  for (const auto& responseItem : readResponse.items)
  {
    const auto& requestItem = readRequest.items[index];
    const auto jdocType = JdocTypeFromDocName(requestItem.docName);
    const auto& jdoc = _jdocs[jdocType];
    const bool wasRequestingLatestVersion = requestItem.myDocVersion == 0;

    if (responseItem.status == JDocs::ReadStatus::Changed)
    {
      // When we've requested 'get latest', if it exists, we got "Changed" status
      const auto& ourDocVersion = GetJdocDocVersion(jdocType);
      LOG_INFO("JdocsManager.HandleReadResponse.Found",
               "Read response for doc %s got 'changed'; cloud version %llu, our version %llu",
               requestItem.docName.c_str(), responseItem.doc.docVersion, ourDocVersion);
      DEV_ASSERT(responseItem.doc.docVersion > 0, "Error: Cloud returned a jdoc with a zero version");
      if (responseItem.doc.docVersion < ourDocVersion)
      {
        // We have a newer version than the cloud has.  This should not
        // be possible because only the cloud can change the version number.
        LOG_ERROR("JdocsManager.HandlerReadResponse.NewerVersionThanCloud",
                  "The version we have is newer than the cloud version (should not be possible)");
      }
      else if (responseItem.doc.docVersion > ourDocVersion)
      {
        // Cloud has a newer version than we do; so pull in that version, overwriting our version
        if (jdoc._errorOnCloudVersionLater)
        {
          LOG_ERROR("JdocsManager.HandleReadResponse.LaterVersionError",
                    "Overwriting robot version of jdoc %s with a later version from cloud",
                    requestItem.docName.c_str());
        }
        else if (jdoc._warnOnCloudVersionLater)
        {
          LOG_WARNING("JdocsManager.HandleReadResponse.LaterVersionWarn",
                      "Overwriting robot version of jdoc %s with a later version from cloud",
                      requestItem.docName.c_str());
        }
        else
        {
          LOG_INFO("JdocsManager.HandleReadResponse.LaterVersionInfo",
                   "Overwriting robot version of jdoc %s with a later version from cloud",
                   requestItem.docName.c_str());
        }

        // Check 'format version' which is our method for occasionally changing the format of the jdoc body
        bool formatVersionOK = true;
        if (responseItem.doc.fmtVersion > jdoc._jdocFormatVersion)
        {
          LOG_ERROR("JdocsManager.HandleReadResponse.FmtVersionError",
                    "Rejecting jdoc from cloud because its format version (%llu) is later than what robot can handle (%llu)",
                    responseItem.doc.fmtVersion, jdoc._jdocFormatVersion);
          formatVersionOK = false;
        }
        else if (responseItem.doc.fmtVersion < jdoc._jdocFormatVersion)
        {
          LOG_INFO("JdocsManager.HandleReadResponse.FmtVersionWarn",
                   "Jdoc from cloud has older format version (%llu) than robot has (%llu); migrating to newer version",
                   responseItem.doc.fmtVersion, jdoc._jdocFormatVersion);
          //responseItem.doc.fmtVersion = jdoc._jdocFormatVersion;
          // This is where we would call any needed backwards-compatibility migration code
        }
        if (formatVersionOK)
        {
          CopyJdocFromCloud(jdocType, responseItem.doc);
        }
      }
      else
      {
        // Do nothing.
        // TODO:  This is where we MAY need to compare a 'minor version' stored in client metadata.
        // (e.g. for RobotLifetimeStats, which are updated more frequently than we submit its jdoc to the cloud)
      }
    }
    else if (responseItem.status == JDocs::ReadStatus::NotFound)
    {
      // Cloud does not have this jdoc, so submit it to the cloud
      LOG_INFO("JdocsManager.HandleReadResponse.NotFound",
               "Read response for doc %s got 'not found', so creating one",
               requestItem.docName.c_str());

      static const bool kIsNewJdocInCloud = true;
      SubmitJdocToCloud(jdocType, kIsNewJdocInCloud);
    }
    else if (responseItem.status == JDocs::ReadStatus::PermissionDenied)
    {
      LOG_ERROR("JdocsManager.HandleReadResponse.PermissionDenied",
                "Read response for doc %s got 'permission denied'",
                requestItem.docName.c_str());
    }
    else  // JDocs::ReadStatus::Unchanged
    {
      if (wasRequestingLatestVersion)
      {
        LOG_ERROR("JdocsManager.HandleReadResponse.Unchanged",
                  "Unexpected 'unchanged' status returned for 'get latest' read request");
      }
    }

    index++;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleDeleteResponse(const JDocs::DeleteRequest& deleteRequest, const Void& voidResponse)
{
  LOG_INFO("JdocsManager.HandleDeleteResponse",
           "Received delete doc response from jdocs server, for userID %s, thingID %s, docname %s",
           deleteRequest.account.c_str(), deleteRequest.thing.c_str(),
           deleteRequest.docName.c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleErrResponse(const JDocs::ErrorResponse& errorResponse)
{
  LOG_ERROR("JdocsManager.HandleErrResponse", "Received error response from jdocs server, with error: %s",
            EnumToString(errorResponse.err));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleUserResponse(const JDocs::UserResponse& userResponse)
{
  _userID = userResponse.userId;
  if (_userID.empty())
  {
    LOG_ERROR("JdocsManager.HandleUserResponse.Error", "Received user response from jdocs server, but ID is empty (not logged in?)");
    return;
  }

  LOG_INFO("JdocsManager.HandleUserResponse", "Received user response from jdocs server, with userID: '%s'",
            _userID.c_str());

  // Now ask the jdocs server to get the latest versions it has of each of these jdocs
  std::vector<JDocs::ReadItem> itemsToRequest;
  for (const auto& jdoc : _jdocs)
  {
    itemsToRequest.emplace_back(jdoc.second._jdocName, 0); // 0 means 'get latest'
  }

  const auto readReq = JDocs::DocRequest::Createread(JDocs::ReadRequest{_userID, _thingID, itemsToRequest});
  SendJdocsRequest(readReq);

  // Finally, if there are any jdoc operations waiting to be sent,
  // send them now, and for each one, fill in the missing userID
  while (!_unsentDocRequestQueue.empty())
  {
    auto unsentRequest = _unsentDocRequestQueue.front();
    _unsentDocRequestQueue.pop();

    if (unsentRequest.GetTag() == JDocs::DocRequestTag::read)
    {
      auto readReq = unsentRequest.Get_read();
      readReq.account = _userID;
      unsentRequest.Set_read(readReq);
    }
    else if (unsentRequest.GetTag() == JDocs::DocRequestTag::write)
    {
      auto writeReq = unsentRequest.Get_write();
      writeReq.account = _userID;
      unsentRequest.Set_write(writeReq);
    }
    else if (unsentRequest.GetTag() == JDocs::DocRequestTag::deleteReq)
    {
      auto deleteReq = unsentRequest.Get_deleteReq();
      deleteReq.account = _userID;
      unsentRequest.Set_deleteReq(deleteReq);
    }

    SendJdocsRequest(unsentRequest);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::SubmitJdocToCloud(const external_interface::JdocType jdocTypeKey, const bool isNewJdocInCloud)
{
  // Jdocs are sent to/from the app with protobuf; jdocs are sent to/from vic-cloud with CLAD
  // Hence the differences/copying code here
  external_interface::Jdoc jdoc;
  GetJdoc(jdocTypeKey, jdoc);
  if (isNewJdocInCloud)
  {
    DEV_ASSERT(jdoc.doc_version() == 0, "Error: Non-zero jdoc version for one not found in the cloud");
  }

  LOG_INFO("JdocsManager.SubmitJdocToCloud", "Submitted jdoc to cloud: %s, doc version %llu, fmt version %llu",
           external_interface::JdocType_Name(jdocTypeKey).c_str(), jdoc.doc_version(), jdoc.fmt_version());
  JDocs::Doc jdocForCloud;
  jdocForCloud.docVersion = isNewJdocInCloud ? 0 : jdoc.doc_version();  // Zero means 'create new'
  jdocForCloud.fmtVersion = jdoc.fmt_version();
  jdocForCloud.metadata   = jdoc.client_metadata();
  jdocForCloud.jsonDoc    = jdoc.json_doc();

  const auto writeReq = JDocs::DocRequest::Createwrite(JDocs::WriteRequest{_userID, _thingID, GetJdocName(jdocTypeKey), jdocForCloud});
  SendJdocsRequest(writeReq);

  _jdocs[jdocTypeKey]._cloudDirty = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::CopyJdocFromCloud(const external_interface::JdocType jdocTypeKey,
                                     const JDocs::Doc& doc)
{
  const auto& it = _jdocs.find(jdocTypeKey);
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.CopyJdocFromCloud.InvalidJdocTypeKey",
              "Invalid jdoc type key (not managed by JdocsManager) %i", (int)jdocTypeKey);
    return false;
  }
  auto& jdocItem = (*it).second;

  jdocItem._jdocVersion = doc.docVersion;
  jdocItem._jdocFormatVersion = doc.fmtVersion;
  jdocItem._jdocClientMetadata = doc.metadata;
  // Convert the single jdoc STRING to a JSON::Value object
  Json::Reader reader;
  const bool success = reader.parse(doc.jsonDoc, jdocItem._jdocBody);
  if (!success)
  {
    LOG_ERROR("JdocsManager.CopyJdocFromCloud.JsonError",
              "Failed to parse json string for jdoc %s body, received from cloud",
              jdocItem._jdocName.c_str());
  }

  // Notify the manager that handles this jdoc data that the data has just been replaced
  if (jdocItem._overwrittenCB != nullptr)
  {
    jdocItem._overwrittenCB();
  }

  if (jdocItem._savedOnDisk)
  {
    SaveJdocFile(jdocTypeKey);
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
external_interface::JdocType JdocsManager::JdocTypeFromDocName(const std::string& docName) const
{
  const auto it = std::find_if(_jdocs.begin(), _jdocs.end(), [&docName](const auto& jdocInfo) {
    return jdocInfo.second._jdocName == docName;
  });
  if (it == _jdocs.end())
  {
    LOG_ERROR("JdocsManager.JdocTypeFromDocName.DocTypeNotFound",
              "No matching enum for doc name %s", docName.c_str());
    return external_interface::JdocType::ROBOT_SETTINGS;  // Have to return something
  }
  return it->first;
}


} // namespace Vector
} // namespace Anki
