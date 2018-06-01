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
 * Copyright: Anki, Inc. 2018
 */

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/robotComponents_fwd.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/variableSnapshot/variableSnapshotObject.h"
#include "engine/components/variableSnapshot/variableSnapshotSerializationFactory.h"
#include "engine/robot.h"
#include "json/json.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/fileUtils/fileUtils.h"

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Basestation_VariableSnapshotComponent_H__
#define __Cozmo_Basestation_VariableSnapshotComponent_H__

namespace Anki {
namespace Cozmo {

class Robot;


class VariableSnapshotComponent : public IDependencyManagedComponent<RobotComponentID>
{
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

private:
  std::unique_ptr<std::unordered_map<VariableSnapshotId, VariableSnapshotObject>> _variableSnapshotDataMap;
  Cozmo::Robot* _robot;
  std::shared_ptr<std::string> _osBuildVersionPtr;
  std::shared_ptr<std::string> _buildShaPtr;

};



template <typename T>
bool VariableSnapshotComponent::InitVariableSnapshot(VariableSnapshotId variableSnapshotId,
                                                     std::shared_ptr<T> dataValuePtr,
                                                     std::function<bool(std::shared_ptr<T>, Json::Value&)> serializeFn,
                                                     std::function<void(std::shared_ptr<T>, const Json::Value&)> deserializeFn) {
  auto& dataAccessorComp = _robot->GetDataAccessorComponent();
  bool isValueLoaded = false;

  auto serializeFnWrapper = [dataValuePtr, serializeFn] (Json::Value& outJson) { return serializeFn(dataValuePtr, outJson); };

  // create a new variable snapshot object with the new shared_ptr and serialization function
  VariableSnapshotObject newDataObj(serializeFnWrapper);

  // if a variable is reinitialized within one boot cycle, get its info from the data map
  auto variableSnapshotIter = _variableSnapshotDataMap->find(variableSnapshotId);
  if(variableSnapshotIter != _variableSnapshotDataMap->end()) {
    isValueLoaded = true;
  }

  // if the specified object has stored JSON data under the specified name, load it and return true.
  // else save the default data and return false
  auto variableSnapshotJsonIter = dataAccessorComp.GetVariableSnapshotJsonMap()->find(variableSnapshotId);
  if(!isValueLoaded && variableSnapshotJsonIter != dataAccessorComp.GetVariableSnapshotJsonMap()->end()) {
    deserializeFn(dataValuePtr, variableSnapshotJsonIter->second);
    isValueLoaded = true;
  }

  // store the new variable snapshot object
  auto result = _variableSnapshotDataMap->emplace(variableSnapshotId, std::move(newDataObj));

  // if the insert fails, then throw an error
  ANKI_VERIFY(result.second, "VariableSnapshotComponent", "%s was reinitialized, which is not currently supported.", VariableSnapshotIdToString(variableSnapshotId));

  // at this point, the data is a new snapshot by default
  return isValueLoaded;
}

} // Cozmo
} // Anki

#endif