/*
 * File: variableSnapshotComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 5/31/2018
 *
 * Description: This is a component meant to provide persistence across boots for data within other 
 * 				      components (like timers or faces) that should be remembered by the robot.
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

CONSOLE_VAR(bool, kResetDataOnNewBuildVersion, "VariableSnapshotComponent", true);

// save location for data
const char* VariableSnapshotComponent::kVariableSnapshotFolder = "variableSnapshotStorage";
const char* VariableSnapshotComponent::kVariableSnapshotFilename = "variableSnapshot";


VariableSnapshotComponent::VariableSnapshotComponent():
  IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::VariableSnapshotComponent),
  _variableSnapshotJsonMap(_robot->GetDataAccessorComponent().GetVariableSnapshotJsonMap()),
  _robot(nullptr) 
{
}

VariableSnapshotComponent::~VariableSnapshotComponent() 
{
  // upon destruction, save all data
  SaveVariableSnapshots();
}

void VariableSnapshotComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;

  // initialize save path for the component
  InitSavePath(kVariableSnapshotFolder, kVariableSnapshotFilename);

  // if we want to reset data when there is a new build version, check for a new build version
  if(kResetDataOnNewBuildVersion) {
    // get current build version/SHA
    auto* osState = Anki::Cozmo::OSState::getInstance();
    DEV_ASSERT(osState != nullptr, "VariableSnapshotComponent.VariableSnapshotComponent.InvalidOSState");
    _osBuildVersionPtr = std::make_shared<std::string>(osState->GetOSBuildVersion());
    _buildShaPtr = std::make_shared<std::string>(osState->GetBuildSha());

    // get versioning data
    InitVariable<std::string>(VariableSnapshotId::_RobotOSBuildVersion,
                              _osBuildVersionPtr,
                              VariableSnapshotEncoder::SerializeString,
                              VariableSnapshotEncoder::DeserializeString);
    InitVariable<std::string>(VariableSnapshotId::_RobotBuildSha,
                              _buildShaPtr,
                              VariableSnapshotEncoder::SerializeString,
                              VariableSnapshotEncoder::DeserializeString);

    if(*_osBuildVersionPtr != osState->GetOSBuildVersion() || *_buildShaPtr != osState->GetBuildSha()) {
      _variableSnapshotJsonMap->clear();
      *_osBuildVersionPtr = osState->GetOSBuildVersion();
      *_buildShaPtr = osState->GetBuildSha();
    }
  }
}

static std::string VariableSnapshotComponent::GetSavePath(std::string folderName, std::string filename) 
{
  auto platform = _robot->GetContextDataPlatform();

  // cache the name of our save directory
  std::string saveFolder = platform->pathToResource( Util::Data::Scope::Persistent, folderName );
  saveFolder = Util::FileUtils::AddTrailingFileSeparator( saveFolder );

  // make sure our folder structure exists
  if(Util::FileUtils::DirectoryDoesNotExist( saveFolder )) {
    Util::FileUtils::CreateDirectory( saveFolder, false, true );
    PRINT_CH_DEBUG( "DataLoader", "VariableSnapshot", "Creating variable snapshot directory: %s", saveFolder.c_str() );
  }
  
  // read in our data
  std::string variableSnapshotSavePath = ( saveFolder + filename + ".json" );

  if(!Util::FileUtils::FileExists( variableSnapshotSavePath )) {
    PRINT_CH_DEBUG( "DataLoader", "VariableSnapshot", "Creating variable snapshot file: %s", variableSnapshotSavePath.c_str() );
  }

  return variableSnapshotSavePath;
}

bool VariableSnapshotComponent::SaveVariableSnapshots()
{
  auto platform = _robot->GetContextDataPlatform();

  // update the stored json data
  for(const auto& dataMapIter : _variableSnapshotDataMap) {
    Json::Value outJson;
    VariableSnapshotId variableSnapshotId = dataMapIter.first;
    
    dataMapIter.second(outJson);
    outJson[VariableSnapshotEncoder::kVariableSnapshotIdKey] = VariableSnapshotIdToString(variableSnapshotId);
    (*_variableSnapshotJsonMap)[variableSnapshotId] = std::move(outJson);
  }

  // create a json list that will be stored
  Json::Value subscriberListJson;
  for(const auto& subscriber : *_variableSnapshotJsonMap) {
    subscriberListJson.append(subscriber.second);
  }

  const bool success = platform->writeAsJson(Util::Data::Scope::Persistent, 
                                             VariableSnapshotComponent::variableSnapshotSavePath, 
                                             subscriberListJson);
  return success;
}


} // Cozmo
} // Anki

#endif