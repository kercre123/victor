/**
* File: photographyManager.h
*
* Author: Paul Terry
* Created: 6/4/18
*
* Description: Controls the process of taking/storing photos and communicating with the app
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/components/photographyManager.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kDevIsStorageFull, "Photography", false);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
PhotographyManager::PhotographyManager()
: IDependencyManagedComponent(this, RobotComponentID::PhotographyManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::UpdateDependent(const RobotCompMap& dependentComps)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::EnablePhotoMode(bool enable)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::IsPhotoStorageFull()
{ 
  return kDevIsStorageFull;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::TakePhoto()
{

}


} // namespace Cozmo
} // namespace Anki
