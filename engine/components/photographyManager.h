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

#ifndef __Cozmo_Basestation_Components_PhotographyManager_H__
#define __Cozmo_Basestation_Components_PhotographyManager_H__

#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class PhotographyManager : public IDependencyManagedComponent<RobotComponentID>, 
                           private Anki::Util::noncopyable
{
public:
  PhotographyManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  // Updates camera settings when taking a photo so that the image is aesthetically pleasing
  void EnablePhotoMode(bool enable);

  // Returns true if the user has used up all their photo storage space
  bool IsPhotoStorageFull();

  // Takes a photograph and saves it to the user's photo storage
  void TakePhoto();

private:
 
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_PhotographyManager_H__
