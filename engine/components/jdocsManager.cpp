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

#include "util/console/consoleInterface.h"

#include "clad/robotInterface/messageEngineToRobot.h"


#define LOG_CHANNEL "JdocsManager"

namespace Anki {
namespace Vector {


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
  static const char* kDiskSavePeriodKey = "delayedDiskSavePeriod_s";
  static const char* kBodyOwnedByJdocsManagerKey = "bodyOwnedByJdocManager";

  static const std::string emptyString;
  static const Json::Value emptyJson;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JdocsManager::JdocsManager()
: IDependencyManagedComponent(this, RobotComponentID::JdocsManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;

  _platform = robot->GetContextDataPlatform();
  DEV_ASSERT(_platform != nullptr, "JdocsManager.InitDependent.DataPlatformIsNull");

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
    LOG_INFO("JdocsManager.InitDependent", "Found name %s", name.c_str());
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
    jdocInfo._jdocFormatVersion = 0;
    jdocInfo._jdocClientMetadata = "";
    jdocInfo._jdocBody = {};
    jdocInfo._jdocName = jdocConfig[kDocNameKey].asString();
    jdocInfo._needsCreation = false;
    jdocInfo._savedOnDisk = jdocConfig[kSavedOnDiskKey].asBool();
    jdocInfo._diskFileDirty = false;
    jdocInfo._diskSavePeriod_s = jdocConfig[kDiskSavePeriodKey].asInt();
    jdocInfo._nextDiskSaveTime = jdocInfo._diskSavePeriod_s;
    jdocInfo._bodyOwnedByJM = jdocConfig[kBodyOwnedByJdocsManagerKey].asBool();
    if (jdocInfo._savedOnDisk)
    {
      jdocInfo._jdocFullPath = Util::FileUtils::FullFilePath({_savePath, jdocInfo._jdocName + ".json"});
    }
    _jdocs[jdocTypeKey] = jdocInfo;

    auto& jdocItem = _jdocs[jdocTypeKey];
    if (jdocItem._savedOnDisk)
    {
      if (Util::FileUtils::FileExists(jdocItem._jdocFullPath))
      {
        if (!LoadJdocFile(jdocTypeKey))
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
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  for (auto& jdocPair : _jdocs)
  {
    auto& jdoc = jdocPair.second;
    if (jdoc._savedOnDisk && jdoc._diskFileDirty && currTime_s > jdoc._nextDiskSaveTime)
    {
      SaveJdocFile(jdocPair.first);
      jdoc._diskFileDirty = false;
      jdoc._nextDiskSaveTime = currTime_s + jdoc._diskSavePeriod_s;
    }
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
                              const bool saveToDiskImmediately)
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
  // TODO: Cloud API
  // ... at some point this version needs to be 'committed' to the cloud, whether it's the first version or not

  // TODO later: When sending a jdoc to cloud, or app, use the two below lines to convert to a single big string.
  //  Json::StyledWriter writer;
  //  const std::string jdocBodyString = writer.write(jdocBodyJSON);


  if (jdocItem._savedOnDisk)
  {
    if (saveToDiskImmediately)
    {
      SaveJdocFile(jdocTypeKey);
      jdocItem._diskFileDirty = false;
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
    LOG_ERROR("JdocsManager.SaveJdocFile.Failed", "Failed to write settings file %s",
              jdocItem._jdocFullPath.c_str());
  }
  else
  {
    jdocItem._needsCreation = false;
  }
}



} // namespace Vector
} // namespace Anki
