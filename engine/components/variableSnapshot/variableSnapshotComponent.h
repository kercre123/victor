/*
 * File: variableSnapshotComponent.h
 *
 * Author: Hamzah Khan
 * Created: 5/31/2018
 *
 * Description: This is a component meant to provide persistence across boots for data within other 
 * 				components (like timers or faces) that could probably be remembered by the robot. 
 *        Snapshots are currently loaded into memory upon robot startup (so there is no load function).
 *
 * Note: The Variable Snapshot Component stores the versioning information of the build: the OS build
 *       version and the robot build SHA. If the version information changes, then all data is erased
 *       unless the kResetDataOnNewBuildVersion console variable is set to false.
 *
 * Copyright: Anki, Inc. 2018
 */

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/robotComponents_fwd.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/variableSnapshot/variableSnapshotEncoder.h"
#include "engine/robot.h"
#include "json/json.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Basestation_VariableSnapshotComponent_H__
#define __Cozmo_Basestation_VariableSnapshotComponent_H__

namespace Anki {
namespace Cozmo {

class Robot;


class VariableSnapshotComponent : public IDependencyManagedComponent<RobotComponentID> {
public:
  explicit VariableSnapshotComponent();
  virtual ~VariableSnapshotComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override final;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };

  virtual void UpdateDependent(const RobotCompMap& dependentComponents) override {};


  //////
  // API functions
  //////

  // InitVariable initializes the variable snapshot by looking up the desired id and loading its 
  // value into the pointer provided by the caller. If there is no existing data, the the pointer 
  // (and its value) remain unchanged. It returns true if a variable snapshot existed and was 
  // successfully loaded. Otherwise, it returns false.
  //
  // Note: If a variable is reinitialized, then the function will error and fail.
  // TODO: This doesn't allow for much flexibility at the moment, so this should be improved.
  // 
  // Note: the serialization function is stored by the component for later use.
  template <typename T>
  bool InitVariable(VariableSnapshotId, std::shared_ptr<T>);

  // save location for data
  static const char* kVariableSnapshotFolder;
  static const char* kVariableSnapshotFilename;

  // creates the path to the desired save location
  static std::string GetSavePath(const Util::Data::DataPlatform*, std::string, std::string);

private:
  
  // SaveVariableSnapshots saves the existing variable snapshots into the component or the subset 
  // specified by a vector of keys, and returns true if the save succeeded.
  bool SaveVariableSnapshots();

  using SerializationFnType = std::function<bool(Json::Value&)>;

  // this data structure stores the function required to serialize data
  std::unordered_map<VariableSnapshotId, SerializationFnType> _variableSnapshotDataMap;

  // this data structure points to the Json data loaded by RobotDataLoader
  RobotDataLoader::VariableSnapshotJsonMap* _variableSnapshotJsonMap;

  Cozmo::Robot* _robot;
};


template <typename T>
bool VariableSnapshotComponent::InitVariable(VariableSnapshotId variableSnapshotId, std::shared_ptr<T> dataValuePtr)
{
  bool isValueLoaded = false;

  // TODO: use this as it is meant to be used and return false
  const bool hasValidPtr = ANKI_VERIFY(nullptr != dataValuePtr,
                                         "VariableSnapshotComponent",
                                         "nullptr is an invalid location to initialize a persistent variable.");
  if( !hasValidPtr ) {
    return false;
  }

  // since dataPtr is a shared pointer of a specific type, we use this wrapper closure to standardize the
  // serialization function signature to be stored in the component and avoid templating.
  auto serializeFnWrapper = [dataValuePtr] (Json::Value& outJson) { return VariableSnapshotEncoder::Serialize<T>(dataValuePtr, outJson); };

  // TODO: if a variable is reinitialized within one boot cycle, get its info from the data map
  //       for when reinitialization is added as a feature

  // if the key exists, then throw an error - reinitialization not currently supported
  auto variableSnapshotIter = _variableSnapshotDataMap.find(variableSnapshotId);
  ANKI_VERIFY(variableSnapshotIter == _variableSnapshotDataMap.end(), 
              "VariableSnapshotComponent", 
              "%s was reinitialized, which is not currently supported.", 
              VariableSnapshotIdToString(variableSnapshotId));

  // if the specified object has stored JSON data under the specified name, load it and return true.
  // else save the default data and return false
  auto variableSnapshotJsonIter = _variableSnapshotJsonMap->find(variableSnapshotId);
  if(variableSnapshotJsonIter != _variableSnapshotJsonMap->end()) {
    VariableSnapshotEncoder::Deserialize<T>(dataValuePtr, variableSnapshotJsonIter->second);
    isValueLoaded = true;
  }

  // store the new serializer
  _variableSnapshotDataMap.emplace(variableSnapshotId, std::move(serializeFnWrapper));

  // at this point, the data is a new snapshot by default
  return isValueLoaded;
}

} // Cozmo
} // Anki

#endif
