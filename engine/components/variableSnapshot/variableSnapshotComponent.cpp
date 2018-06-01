/*
 * File: variableSnapshotComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 5/31/2018
 *
 * Description: This is a component meant to provide persistence across boots for data within other 
 * 				components (like timers or faces) that should be remembered by the robot.
 *
 *
 * Copyright: Anki, Inc. 2018
 */

#include "engine/components/variableSnapshot/variableSnapshotComponent.h"

#include "osState/osState.h"
#include "util/console/consoleInterface.h"

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Basestation_VariableSnapshotComponent__
#define __Cozmo_Basestation_VariableSnapshotComponent__

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kVariableSnapshotComponent_ResetDataOnNewBuildVersion, "VariableSnapshotComponent", true);

// save location for data
const char* VariableSnapshotComponent::kVariableSnapshotFolder = "variableSnapshotStorage";
const char* VariableSnapshotComponent::kVariableSnapshotFilename = "variableSnapshot";


VariableSnapshotComponent::VariableSnapshotComponent():
  IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::VariableSnapshotComponent),
  _variableSnapshotDataMap(std::unordered_map<VariableSnapshotId, VariableSnapshotObject>()),
  _robot(nullptr) 
{
}

VariableSnapshotComponent::~VariableSnapshotComponent() 
{
}


//////
// IDependencyManagedComponent functions
//////
void VariableSnapshotComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  auto& dataAccessorComp = _robot->GetDataAccessorComponent();

  // if we want to reset data when there is a new build version, check for a new build version
  if(kVariableSnapshotComponent_ResetDataOnNewBuildVersion) {
    // get current build version/SHA
    auto* osState = Anki::Cozmo::OSState::getInstance();
    DEV_ASSERT(osState != nullptr, "VariableSnapshotComponent.VariableSnapshotComponent.InvalidOSState");
    _osBuildVersionPtr = std::make_shared<std::string>(osState->GetOSBuildVersion());
    _buildShaPtr = std::make_shared<std::string>(osState->GetBuildSha());

    // get versioning data
    InitVariableSnapshot<std::string>(VariableSnapshotId::_RobotOSBuildVersion,
                                      _osBuildVersionPtr,
                                      VariableSnapshotEncoder::SerializeString,
                                      VariableSnapshotEncoder::DeserializeString);
    InitVariableSnapshot<std::string>(VariableSnapshotId::_RobotBuildSha,
                                      _buildShaPtr,
                                      VariableSnapshotEncoder::SerializeString,
                                      VariableSnapshotEncoder::DeserializeString);

    if(*_osBuildVersionPtr != osState->GetOSBuildVersion() || *_buildShaPtr != osState->GetBuildSha()) {
      dataAccessorComp.GetVariableSnapshotJsonMap()->clear();
      *_osBuildVersionPtr = osState->GetOSBuildVersion();
      *_buildShaPtr = osState->GetBuildSha();
    }
  }
}


//////
// API functions
//////
bool VariableSnapshotComponent::SaveVariableSnapshots()
{
  auto& dataAccessorComp = _robot->GetDataAccessorComponent();
  auto platform = _robot->GetContextDataPlatform();

  // get the path to the file
  std::string saveFolder = platform->pathToResource( Util::Data::Scope::Persistent, kVariableSnapshotFolder );
  saveFolder = Util::FileUtils::AddTrailingFileSeparator( saveFolder );
  std::string pathToVariableSnapshotFile = ( saveFolder + kVariableSnapshotFilename + ".json" );

  // update the stored json data
  for(const auto& subscriber : _variableSnapshotDataMap) {
    Json::Value subscriberJson;
    VariableSnapshotId variableSnapshotId = subscriber.first;
    
    subscriber.second.Serialize(subscriberJson);
    subscriberJson[VariableSnapshotEncoder::kVariableSnapshotIdKey] = VariableSnapshotIdToString(variableSnapshotId);
    (*dataAccessorComp.GetVariableSnapshotJsonMap())[variableSnapshotId] = std::move(subscriberJson);
  }

  // create a json list that will be stored
  Json::Value subscriberListJson;
  for(const auto& subscriber : *(dataAccessorComp.GetVariableSnapshotJsonMap())) {
    subscriberListJson.append(subscriber.second);
  }

  const bool success = platform->writeAsJson(Util::Data::Scope::Persistent, 
                                             pathToVariableSnapshotFile, 
                                             subscriberListJson);
  return success;
}


} // Cozmo
} // Anki

#endif