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
  static const char* kWarnOnCloudVersionLaterKey = "warnOnCloudVersionLater";
  static const char* kErrorOnCloudVersionLaterKey = "errorOnCloudVersionLater";

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
  std::vector<JDocs::ReadItem> itemsToRequest;

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
    jdocInfo._warnOnCloudVersionLater = jdocConfig[kWarnOnCloudVersionLaterKey].asBool();
    jdocInfo._errorOnCloudVersionLater = jdocConfig[kErrorOnCloudVersionLaterKey].asBool();
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

    itemsToRequest.emplace_back(jdocItem._jdocName, 0); // 0 means 'get latest'
  }

  // Now ask the jdocs server to get the latest versions it has of each of these jdocs
#if 0 // Disabled for now
  std::string userID, thing;
  GetUserAndThingIDs(userID, thing);
  const auto readReq = JDocs::DocRequest::Createread(JDocs::ReadRequest{userID, thing, itemsToRequest});
  SendJdocsRequest(readReq);
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::GetUserAndThingIDs(std::string& userID, std::string& thingID) const
{
  userID = "55555"; // todo user id.  Just using bogus one for now
  auto *osstate = OSState::getInstance();
  thingID = "vic:" + osstate->GetSerialNumberAsString();
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
        // If there are any jdoc operations waiting to be sent, send them now:
        while (!_unsentDocRequestQueue.empty())
        {
          SendJdocsRequest(_unsentDocRequestQueue.front());
          _unsentDocRequestQueue.pop();
        }
      }
    }
#endif
  }

  if (_udpClient.IsConnected())
  {
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::UpdatePeriodicFileSaves(const float currTime_s)
{
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
  // If we're not connected to the jdocs server, put the request in another queue
  // (on connection, we'll send them)
  if (!_udpClient.IsConnected())
  {
    _unsentDocRequestQueue.push(docRequest);
    LOG_INFO("JdocsManager.SendJdocsRequest",
             "Jdocs server not connected; adding to unsent requests (size now %zu)",
             _unsentDocRequestQueue.size());
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
        LOG_INFO("JdocsManager.UpdateJdocsServerResponses.Write", "Received write response");
        HandleWriteResponse(docRequest.Get_write(), response.Get_write());
      }
      break;

      case JDocs::DocResponseTag::read:
      {
        LOG_INFO("JdocsManager.UpdateJdocsServerResponses.Read", "Received read response");
        HandleReadResponse(docRequest.Get_read(), response.Get_read());
      }
      break;

      case JDocs::DocResponseTag::deleteResp:
      {
        LOG_INFO("JdocsManager.UpdateJdocsServerResponses.DeleteResp", "Received deleteResp response");
        HandleDeleteResponse(docRequest.Get_deleteReq(), response.Get_deleteResp());
      }
      break;

      case JDocs::DocResponseTag::err:
      {
        LOG_INFO("JdocsManager.UpdateJdocsServerResponses.Err", "Received err response");
        HandleErrResponse(response.Get_err());
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
  // todo:  Handle case: On startup, we asked for jdoc, didn't exist, so sent one up.  handle the response to that
  // todo:  handle case: Regular update (e.g. RobotSettings change)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JdocsManager::HandleReadResponse(const JDocs::ReadRequest& readRequest, const JDocs::ReadResponse& readResponse)
{
  // Note: Currently this only happens after startup, when we're getting all 'latest jdocs'
  // ...if/when we do other read requests, we may have to add flags/logic to differentiate
  DEV_ASSERT_MSG(readRequest.items.size() == readResponse.items.size(),
                 "JdocsManager.HandleReadResponse.Mismatch",
                 "Mismatch of number of items in jdocs read request vs. response (%zu vs %zu)",
                 readRequest.items.size(), readResponse.items.size());

  std::string userID, thing;
  GetUserAndThingIDs(userID, thing);

  int index = 0;
  for (const auto& responseItem : readResponse.items)
  {
    const auto& requestItem = readRequest.items[index];
    const auto jdocType = JdocTypeFromDocName(requestItem.docName);
    const auto& jdoc = _jdocs[jdocType];

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
        if (jdoc._warnOnCloudVersionLater)
        {
          LOG_WARNING("JdocsManager.HandleReadResponse.LaterVersionWarn",
                      "Overwriting robot version of jdoc %s with a later version from cloud",
                      requestItem.docName.c_str());
        }
        if (jdoc._errorOnCloudVersionLater)
        {
          LOG_ERROR("JdocsManager.HandleReadResponse.LaterVersionError",
                    "Overwriting robot version of jdoc %s with a later version from cloud",
                    requestItem.docName.c_str());
        }

        static const bool kSaveToDiskImmediately = true;
        CopyJdocFromCloud(jdocType, responseItem.doc, kSaveToDiskImmediately);
      }
      else
      {
        // Do nothing.
        // TODO:  This is where we need to compare a 'minor version' stored in client metadata.
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
      SubmitJdocToCloud(jdocType, kIsNewJdocInCloud, userID, thing);
    }
    else if (responseItem.status == JDocs::ReadStatus::PermissionDenied)
    {
      LOG_ERROR("JdocsManager.HandleReadResponse.PermissionDenied",
                "Read response for doc %s got 'permission denied'",
                requestItem.docName.c_str());
    }
    else  // JDocs::ReadStatus::Unchanged
    {
      LOG_ERROR("JdocsManager.HandleReadResponse.Unchanged",
                "Unexpected 'unchanged' status returned for read request (at least for 'get latest')");
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
void JdocsManager::SubmitJdocToCloud(const external_interface::JdocType jdocTypeKey, const bool isNewJdocInCloud,
                                     const std::string& userID, const std::string& thing)
{
  // Jdocs are sent to/from the app with protobuf; jdocs are sent to/from vic-cloud with CLAD
  // Hence the differences/copying code here
  external_interface::Jdoc jdoc;
  GetJdoc(jdocTypeKey, jdoc);
  if (isNewJdocInCloud)
  {
    DEV_ASSERT(jdoc.doc_version() == 0, "Error: Non-zero jdoc version for one not found in the cloud");
  }

  JDocs::Doc jdocForCloud;
  jdocForCloud.docVersion = isNewJdocInCloud ? 0 : jdoc.doc_version();  // Zero means 'create new'
  jdocForCloud.fmtVersion = jdoc.fmt_version();
  jdocForCloud.metadata   = jdoc.client_metadata();
  jdocForCloud.jsonDoc    = jdoc.json_doc();

  const auto writeReq = JDocs::DocRequest::Createwrite(JDocs::WriteRequest{userID, thing, GetJdocName(jdocTypeKey), jdocForCloud});
  SendJdocsRequest(writeReq);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JdocsManager::CopyJdocFromCloud(const external_interface::JdocType jdocTypeKey,
                                     const JDocs::Doc& doc,
                                     const bool saveToDiskImmediately)
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
  jdocItem._jdocBody = doc.jsonDoc;

  if (!jdocItem._bodyOwnedByJM)
  {
    // TODO signal the code that handles this jdoc, that the contents have changed
    // This should e.g. signal SettingsManager, for the vic.RobotSettings jdoc,
    // so it can take a copy, and then apply those settings.
    // The 'signal' can just be a bool that's check in SettingsManager::update.
    // Or, maybe it's a direct call with a registered handler, so there's no race conditions.
  }

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
