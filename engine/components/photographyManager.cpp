/**
* File: photographyManager.cpp
*
* Author: Paul Terry
* Created: 6/4/18
*
* Description: Controls the process of taking/storing photos and communicating with the app
*
* Copyright: Anki, Inc. 2018
*
**/

#include "coretech/vision/engine/imageCache.h"

#include "engine/components/photographyManager.h"
#include "engine/components/visionComponent.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kDevIsStorageFull, "Photography", false);
  
// If true, requires OS version that supports camera format change
CONSOLE_VAR(bool, kTakePhoto_UseFullSensorResolution, "Photography", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
PhotographyManager::PhotographyManager()
: IDependencyManagedComponent(this, RobotComponentID::PhotographyManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _visionComponent = robot->GetComponentPtr<VisionComponent>();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if((State::WaitingForPhotoModeDisable == _state) || (State::WaitingForPhotoModeEnable == _state))
  {
    if(!_visionComponent->IsWaitingForCaptureFormatChange())
    {
      if(State::WaitingForPhotoModeDisable == _state)
      {
        _state = State::Idle;
      }
      else
      {
        _state = State::InPhotoMode;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PhotographyManager::EnablePhotoMode(bool enable)
{
  // TODO: remove this once VIC-3589 is fixed
  if(!kTakePhoto_UseFullSensorResolution)
  {
    // No format change: immediately in photo mode
    _state = State::InPhotoMode;
    return RESULT_OK;
  }
  
  DEV_ASSERT(nullptr != _visionComponent, "PhotographyManager.EnablePhotoMode.NullVisionComponent");

  if(!IsReadyToSwitchModes())
  {
    PRINT_NAMED_WARNING("PhotographyManager.EnablePhotoMode.NotReady", "");
    return RESULT_FAIL;
  }

  const ImageEncoding format = (enable ? ImageEncoding::YUV420sp : ImageEncoding::RawRGB);
  _visionComponent->SetCameraCaptureFormat(format);
  _state = (enable ? State::WaitingForPhotoModeEnable : State::WaitingForPhotoModeDisable);
  
  PRINT_CH_INFO("Behaviors", "PhotographyManager.EnablePhotoMode.FormatChange",
                "Requesting format: %s", EnumToString(format));

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::IsReadyToSwitchModes() const
{
  return (State::Idle == _state) || (State::InPhotoMode == _state);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::IsReadyToTakePhoto() const
{
  return (State::InPhotoMode == _state);
}
 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::IsPhotoStorageFull()
{ 
  return kDevIsStorageFull;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string PhotographyManager::GetSavePath() const
{
  // TODO: Populate this with real save path
  return "";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string PhotographyManager::GetBasename() const
{
  // TODO: Populate this with real base filename
  return "";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhotographyManager::PhotoHandle PhotographyManager::TakePhoto()
{
  DEV_ASSERT(nullptr != _visionComponent, "PhotographManager.TakePhoto.NullVisionComponent");

  if(!IsReadyToTakePhoto())
  {
    PRINT_NAMED_WARNING("PhotographyManager.TakePhoto.NotReady", "");
    return 0;
  }

  // TODO: Get fron Json config?
  const bool   kRemoveDistortion = true;
  const int8_t kSaveQuality = 95;
  const f32    kThumbnailScale = 0.125f;
  
  const auto photoSize = (kTakePhoto_UseFullSensorResolution ?
                          Vision::ImageCache::Size::Sensor :
                          Vision::ImageCache::Size::Full);
  
  PRINT_CH_INFO("Behavior", "PhotographyManager.TakePhoto.SettingSaveParams", "");
  
  _visionComponent->SetSaveImageParameters(ImageSendMode::SingleShot,
                                           GetSavePath(),
                                           GetBasename(),
                                           kSaveQuality,
                                           photoSize,
                                           kRemoveDistortion,
                                           kThumbnailScale);
  
  _lastRequestedPhotoHandle = _visionComponent->GetLastProcessedImageTimeStamp();
  _state = State::WaitingForTakePhoto;
  
  return _lastRequestedPhotoHandle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::SetLastPhotoTimeStamp(TimeStamp_t timestamp)
{
  static_assert(sizeof(PhotoHandle) >= sizeof(TimeStamp_t), "PhotoHandle alias must be able to accomodate TimeStamps");
  _lastSavedPhotoHandle = timestamp;
  
  if(WasPhotoTaken(_lastRequestedPhotoHandle))
  {
    // Last TakePhoto call is completed, go back to InPhotoMode, meaning we are ready to take more photos
    _state = State::InPhotoMode;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::WasPhotoTaken(const PhotoHandle handle) const
{
  return (_lastSavedPhotoHandle > handle);
}

} // namespace Cozmo
} // namespace Anki
