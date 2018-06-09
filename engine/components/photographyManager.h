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

// Forward declaration
class VisionComponent;
  
class PhotographyManager : public IDependencyManagedComponent<RobotComponentID>, 
                           private Anki::Util::noncopyable
{
public:
  PhotographyManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  }
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::Vision);
  }
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  // Returns true if EnablePhotoMode() is safe to call
  bool IsReadyToSwitchModes() const;
  
  // Updates camera settings when taking a photo so that the image is aesthetically pleasing
  Result EnablePhotoMode(bool enable);
  
  // Returns true once switch to photo mode is complete after a call to EnablePhotoMode(true)
  bool IsReadyToTakePhoto() const;

  // Tells Vision component to take a photograph and saves it to the user's photo storage
  // Returns a handle to use to check if the photo has been taken/saved yet
  // If called when not ready to take photo (see above check), returns 0
  using PhotoHandle = size_t;
  PhotoHandle TakePhoto();

  // Returns true once the corresponding TakePhoto() call has completed
  bool WasPhotoTaken(const PhotoHandle handle) const;

  // Returns true if the user has used up all their photo storage space
  bool IsPhotoStorageFull();

  // Called by VisionComponent when photo has been taken and saved
  void SetLastPhotoTimeStamp(TimeStamp_t timestamp);

private:
  
  std::string GetSavePath() const;
  std::string GetBasename() const;
  
  enum class State {
    Idle,
    WaitingForPhotoModeEnable,
    InPhotoMode,
    WaitingForTakePhoto,
    WaitingForPhotoModeDisable,
  };
  
  State             _state = State::Idle;
  VisionComponent*  _visionComponent = nullptr;
  PhotoHandle       _lastRequestedPhotoHandle = 0;
  PhotoHandle       _lastSavedPhotoHandle = 0;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_PhotographyManager_H__
