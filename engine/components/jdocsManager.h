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

#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "proto/external_interface/settings.pb.h"

#include "clad/types/robotSettingsTypes.h"

#include <map>

namespace Anki {
namespace Vector {

class JdocsManager : public IDependencyManagedComponent<RobotComponentID>,
                     private Anki::Util::noncopyable
{
public:
  JdocsManager();

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
  
  bool            JdocNeedsCreation(const external_interface::JdocType jdocTypeKey) const;
  const std::string&    GetJdocName(const external_interface::JdocType jdocTypeKey) const;
  const Json::Value&    GetJdocBody(const external_interface::JdocType jdocTypeKey) const;
  Json::Value*   GetJdocBodyPointer(const external_interface::JdocType jdocTypeKey);
  bool                      GetJdoc(const external_interface::JdocType jdocTypeKey,
                                    external_interface::Jdoc& jdocOut) const;
  bool                   UpdateJdoc(const external_interface::JdocType jdocTypeKey,
                                    const Json::Value* jdocBody,
                                    const bool saveToDiskImmediately);
  bool                ClearJdocBody(const external_interface::JdocType jdocTypeKey);

private:

  bool LoadJdocFile(const external_interface::JdocType jdocTypeKey);
  void SaveJdocFile(const external_interface::JdocType jdocTypeKey);

  Robot*                    _robot = nullptr;
  Util::Data::DataPlatform* _platform = nullptr;
  std::string               _savePath;

  struct JdocInfo
  {
    // Start of jdoc contents
    uint64_t                  _jdocVersion;       // Major version of jdoc
    uint64_t                  _jdocFormatVersion; // Format version of jdoc
    std::string               _jdocClientMetadata; // (I think this may contain minor version at some point)
    Json::Value               _jdocBody;          // Body of jdoc in the form of JSON object
    // End of jdoc contents
    std::string               _jdocName;          // Official name; used in cloud API
    bool                      _needsCreation;     // True if this jdoc needs to be created (by another subsystem)
    bool                      _savedOnDisk;       // True if we keep a copy on disk
    std::string               _jdocFullPath;      // Full path of file on disk if applicable
    bool                      _diskFileDirty;
    int                       _diskSavePeriod_s;  // Disk save period, or 0 for always save immediately
    float                     _nextDiskSaveTime;  // Time of next disk save ("at this time or after")
    bool                      _bodyOwnedByJM;     // True when JdocsManager owns the jdoc body (otherwise it's a copy)
  };

  using Jdocs = std::map<external_interface::JdocType, JdocInfo>;
  Jdocs                       _jdocs;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Components_JdocsManager_H__
