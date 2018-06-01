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
 * Note: This component stores the versioning information of the build. If the version changes, then all
 *       data is erased unless the kVariableSnapshotComponent_ResetDataOnNewBuildVersion console variable
 *       is set to false.
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

  // SaveVariableSnapshots saves the existing variable snapshots into the component or the subset 
  // specified by a vector of keys, and returns true if the save succeeded.
  bool SaveVariableSnapshots();


  // InitVariableSnapshot initializes the variable snapshot by looking up the desired id and loading 
  // its value into the pointer provided by the caller. If there is no existing data, the the pointer 
  // (and its value) remain unchanged. It returns true if a variable snapshot existed and was 
  // successfully loaded. Otherwise, it returns false.
  //
  // Note: If a variable is reinitialized, then the function will error and fail.
  // TODO: This doesn't allow for much flexibility at the moment, so this should be improved.
  // 
  // Note: the serialization function is stored by the component for later use.
  template <typename T>
  bool InitVariableSnapshot(VariableSnapshotId,
                            std::shared_ptr<T>,
                            std::function<bool(std::shared_ptr<T>, Json::Value&)>,
                            std::function<void(std::shared_ptr<T>, const Json::Value&)>);

  // save location for data
  static const char* kVariableSnapshotFolder;
  static const char* kVariableSnapshotFilename;

private:
  using SerializationFnType = std::function<bool(Json::Value&)>;

  // This class defines an interface for objects that are used to represent persistent data in the
  // variable snapshot component. The interface provides a single function to serialize the data to
  // a JSON value.
  class VariableSnapshotObject {
  public:
    VariableSnapshotObject(SerializationFnType serializeFn)
      :  _serializeFn(serializeFn) {};

    // serialization functionality
    bool Serialize(Json::Value& outJsonData) const { return _serializeFn(outJsonData); };

  private:
    SerializationFnType _serializeFn;
  };

  std::unordered_map<VariableSnapshotId, VariableSnapshotObject> _variableSnapshotDataMap;
  Cozmo::Robot* _robot;
  std::shared_ptr<std::string> _osBuildVersionPtr;
  std::shared_ptr<std::string> _buildShaPtr;

};


template <typename T>
bool VariableSnapshotComponent::InitVariableSnapshot(VariableSnapshotId variableSnapshotId,
                                                     std::shared_ptr<T> dataValuePtr,
                                                     std::function<bool(std::shared_ptr<T>, Json::Value&)> serializeFn,
                                                     std::function<void(std::shared_ptr<T>, const Json::Value&)> deserializeFn)
{
  auto& dataAccessorComp = _robot->GetDataAccessorComponent();
  bool isValueLoaded = false;

  // since dataPtr is a shared pointer of a specific type, we use this wrapper closure to standardize the
  // serialization function signature to be stored in the component and avoid templating.
  auto serializeFnWrapper = [dataValuePtr, serializeFn] (Json::Value& outJson) { return serializeFn(dataValuePtr, outJson); };

  // create a new variable snapshot object with the new shared_ptr and serialization function
  VariableSnapshotObject newDataObj(serializeFnWrapper);

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
  auto variableSnapshotJsonIter = dataAccessorComp.GetVariableSnapshotJsonMap()->find(variableSnapshotId);
  if(variableSnapshotJsonIter != dataAccessorComp.GetVariableSnapshotJsonMap()->end()) {
    deserializeFn(dataValuePtr, variableSnapshotJsonIter->second);
    isValueLoaded = true;
  }

  // store the new variable snapshot object
  _variableSnapshotDataMap.emplace(variableSnapshotId, std::move(newDataObj));

  // at this point, the data is a new snapshot by default
  return isValueLoaded;
}

} // Cozmo
} // Anki

#endif
