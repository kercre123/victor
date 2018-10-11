/**
 * File: jdocsManager.h
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

#ifndef __Cozmo_Basestation_Components_JdocsManager_H__
#define __Cozmo_Basestation_Components_JdocsManager_H__

#include "coretech/common/engine/utils/recentOccurrenceTracker.h"
#include "coretech/messaging/shared/LocalUdpClient.h"
#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "proto/external_interface/settings.pb.h"

#include "clad/cloud/docs.h"

#include <map>
#include <queue>

namespace Anki {
namespace Vector {

class JdocsManager : public IDependencyManagedComponent<RobotComponentID>,
                     private Anki::Util::noncopyable
{
public:
  JdocsManager();
  ~JdocsManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  using JsonTransformFunc = Json::Value (*)(const Json::Value&);

  bool            JdocNeedsCreation(const external_interface::JdocType jdocTypeKey) const;
  bool           JdocNeedsMigration(const external_interface::JdocType jdocTypeKey) const;
  const std::string&    GetJdocName(const external_interface::JdocType jdocTypeKey) const;
  const uint64_t  GetJdocDocVersion(const external_interface::JdocType jdocTypeKey) const;
  const uint64_t  GetJdocFmtVersion(const external_interface::JdocType jdocTypeKey) const;
  const uint64_t   GetCurFmtVersion(const external_interface::JdocType jdocTypeKey) const;
  void   SetJdocFmtVersionToCurrent(const external_interface::JdocType jdocTypeKey);
  const Json::Value&    GetJdocBody(const external_interface::JdocType jdocTypeKey) const;
  Json::Value*   GetJdocBodyPointer(const external_interface::JdocType jdocTypeKey);
  bool                      GetJdoc(const external_interface::JdocType jdocTypeKey,
                                    external_interface::Jdoc& jdocOut,
                                    const JsonTransformFunc transformFunc = nullptr) const;
  bool                   UpdateJdoc(const external_interface::JdocType jdocTypeKey,
                                    const Json::Value* jdocBody,
                                    const bool saveToDiskImmediately,
                                    const bool saveToCloudImmediately,
                                    const bool setCloudDirtyIfNotImmediate = true);
  bool                ClearJdocBody(const external_interface::JdocType jdocTypeKey);

  bool SendJdocsRequest(const JDocs::DocRequest& docRequest);

  // For testing and development:
  void DebugFakeUserLogOut();
  void DebugCheckForUser();
  void DeleteJdocInCloud(const external_interface::JdocType jdocTypeKey);

  using OverwriteNotificationCallback = std::function<void(void)>;
  void RegisterOverwriteNotificationCallback(const external_interface::JdocType jdocTypeKey,
                                             const OverwriteNotificationCallback cb);

  using FormatMigrationCallback = std::function<void(void)>;
  void RegisterFormatMigrationCallback(const external_interface::JdocType jdocTypeKey,
                                       const FormatMigrationCallback cb);

private:

  bool LoadJdocFile(const external_interface::JdocType jdocTypeKey);
  void SaveJdocFile(const external_interface::JdocType jdocTypeKey,
                    const int cloudDirtyRemaining_s = 0);
  void UpdatePeriodicFileSaves(const bool isShuttingDown = false);

  bool ConnectToJdocsServer();
  bool SendUdpMessage(const JDocs::DocRequest& msg);
  void UpdatePeriodicCloudSaves();
  void UpdateJdocsServerResponses();
  void HandleWriteResponse(const JDocs::WriteRequest& writeRequest, const JDocs::WriteResponse& writeResponse);
  void HandleReadResponse(const JDocs::ReadRequest& readRequest, const JDocs::ReadResponse& readResponse);
  void HandleDeleteResponse(const JDocs::DeleteRequest& deleteRequest, const Void& voidResponse);
  void HandleErrResponse(const JDocs::DocRequest& request, const JDocs::ErrorResponse& errorResponse);
  void HandleUserResponse(const JDocs::UserResponse& userResponse);
  void HandleThingResponse(const JDocs::ThingResponse& thingResponse);
  void SubmitJdocToCloud(const external_interface::JdocType jdocTypeKey, const bool isJdocNewInCloud);
  bool CopyJdocFromCloud(const external_interface::JdocType jdocTypeKey, const JDocs::Doc& doc);

  external_interface::JdocType JdocTypeFromDocName(const std::string& docName) const;

  Robot*                    _robot = nullptr;
  Util::Data::DataPlatform* _platform = nullptr;
  std::string               _savePath;
  LocalUdpClient            _udpClient;
  std::string               _userID;
  std::string               _thingID;
  bool                      _gotLatestCloudJdocsAtStartup = false;
  // We save currTime_s here each tick, because we need it in the destructor, and by then BasetationTimer is gone
  float                     _currTime_s;
  float                     _nextUserLoginCheckTime_s;

  struct JdocInfo
  {
    // Start of jdoc contents
    uint64_t                  _jdocVersion;       // Major version of jdoc; ONLY the cloud server can change this
    uint64_t                  _jdocFormatVersion; // Format version of jdoc; cloud server CANNOT change this
    std::string               _jdocClientMetadata; // (I think this may contain minor version at some point)
    Json::Value               _jdocBody;          // Body of jdoc in the form of JSON object
    // End of jdoc contents

    std::string               _jdocName;          // Official name; used in cloud API
    bool                      _needsCreation;     // True if this jdoc needs to be created (by another subsystem)
    bool                      _needsMigration;    // True if this jdoc needs a format version migration at startup
    uint64_t                  _curFormatVersion;  // Current/latest format version this code knows about

    std::string               _jdocFullPath;      // Full path of file on disk if applicable
    bool                      _diskFileDirty;
    int                       _diskSavePeriod_s;  // Disk save period, or 0 for always save immediately
    float                     _nextDiskSaveTime;  // Time of next disk save ("at this time or after")

    bool                      _bodyOwnedByJM;     // True when JdocsManager owns the jdoc body (otherwise it's a copy)

    bool                      _warnOnCloudVersionLater;
    bool                      _errorOnCloudVersionLater;
    bool                      _cloudDirty;        // True when cloud copy of the jdoc needs to be updated
    int                       _cloudSavePeriod_s; // Cloud save period, or 0 for always save immediately
    int                       _origCloudSavePeriod_s; // Original cloud save period (so we can restore if changed)
    float                     _nextCloudSaveTime; // Time of next cloud save ("at this time or after")

    enum class CloudDisabled
    {
      NotDisabled,
      FormatVersion,      // Disabled because cloud has higher format version of this jdoc than code can handle
      CloudError,         // Disabled because we received a cloud error on write or read of jdoc to/from cloud
    };
    CloudDisabled             _cloudDisabled = CloudDisabled::NotDisabled;
    
    struct CloudAbuseDetectionConfig
    {
      CloudAbuseDetectionConfig();
      
      struct Rule
      {
        RecentOccurrenceTracker::Handle _recentOccurrenceHandle;
        int                             _numberOfTimes;
        float                           _amountOfSeconds;
        int                             _cloudAbuseSavePeriod_s;
      };
      std::vector<Rule>         _abuseRules;

      RecentOccurrenceTracker   _cloudWriteTracker; // Used to detect cloud spam abuse
      int                       _abuseLevel;
    } _abuseConfig;

    OverwriteNotificationCallback _overwrittenCB; // Called when this jdoc is overwritten from the cloud
    FormatMigrationCallback   _formatMigrationCB; // Called when this jdoc needs a format migration
  };

  using Jdocs = std::map<external_interface::JdocType, JdocInfo>;
  Jdocs                       _jdocs;

  using DocRequestQueue = std::queue<JDocs::DocRequest>;
  DocRequestQueue             _docRequestQueue;       // Outstanding requests that have been sent
  DocRequestQueue             _unsentDocRequestQueue; // Unsent requests that are waiting for connection
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Components_JdocsManager_H__
