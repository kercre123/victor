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

#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"
#include "engine/externalInterface/gatewayInterface.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include "coretech/common/engine/utils/timer.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class VisionComponent;
class IGatewayInterface;
namespace external_interface {
  class Photo;
  class PhotosInfoRequest;
  class PhotoRequest;
  class ThumbnailRequest;
  class DeletePhotoRequest;
}


class PhotographyManager : public IDependencyManagedComponent<RobotComponentID>, 
                           private Anki::Util::noncopyable
{
public:
  PhotographyManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;
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

  static const char * GetPhotoExtension() { return "jpg"; }
  static const char * GetThumbExtension() { return "thm.jpg"; }

private:

  // Load/save the photos 'database' json file
  bool LoadPhotosFile();
  void SavePhotosFile() const;

  bool DeletePhotoByID(const int id, const bool savePhotosFile = true);

  bool SendImageHelper(const int id, const bool isThumbnail,
                       external_interface::Photo* photo);

  std::string GetSavePath() const;
  std::string GetBasename(int photoID) const;
  const std::string& GetStateString() const;
  int PhotoIndexFromID(const int id) const; // Returns photo info index, or -1 if not found
  
  void HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void OnRequestPhotosInfo (const external_interface::PhotosInfoRequest&  photosInfoRequest);
  void OnRequestPhoto      (const external_interface::PhotoRequest&       photoRequest);
  void OnRequestThumbnail  (const external_interface::ThumbnailRequest&   thumbnailRequest);
  void OnRequestDeletePhoto(const external_interface::DeletePhotoRequest& deletePhotoRequest);

  enum class State {
    Idle,
    WaitingForPhotoModeEnable,
    InPhotoMode,
    WaitingForTakePhoto,
    WaitingForPhotoModeDisable,
  };

  Util::Data::DataPlatform* _platform = nullptr;
  State             _state = State::Idle;
  VisionComponent*  _visionComponent = nullptr;
  PhotoHandle       _lastRequestedPhotoHandle = 0;
  PhotoHandle       _lastSavedPhotoHandle = 0;
  int               _nextPhotoID = 0;
  std::string       _savePath = "";
  std::string       _fullPathPhotoInfoFile = "";
  bool              _disableWhenPossible = false;
  IGatewayInterface* _gatewayInterface = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  struct PhotoInfo
  {
    PhotoInfo() {}
    PhotoInfo(int id, TimeStamp_t dt, bool copied) : _id(id), _dateTimeTaken(dt), _copiedToApp(copied) {}

    int         _id;
    TimeStamp_t _dateTimeTaken;
    bool        _copiedToApp;
  };

  std::vector<PhotoInfo> _photoInfos;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_PhotographyManager_H__
