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
#include "engine/robotDataLoader.h"

#include "util/console/consoleInterface.h"

#include <sstream>
#include <iomanip>

// Log options
#define LOG_CHANNEL "PhotographyManager"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kDevIsStorageFull, "Photography", false);

// If true, requires OS version that supports camera format change
CONSOLE_VAR(bool, kTakePhoto_UseSensorResolution, "Photography", true);
    
namespace
{
  const std::string kPhotoManagerFolder = "photos";
  const std::string kPhotoManagerFilename = "photos.json";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  // Overridden by config file
  int kMaxSlots           = 20;
  bool kRemoveDistortion  = true;
  int8_t kSaveQuality     = 95;
  float kThumbnailScale   = 0.125f;
  
  const char* kModuleName = "PhotographyManager";
  
  static const std::string kNextPhotoIDKey = "NextPhotoID";
  static const std::string kPhotoInfosKey = "PhotoInfos";
  static const std::string kIDKey = "ID";
  static const std::string kTimeStampKey = "TimeStamp";
  static const std::string kCopiedKey = "Copied";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
PhotographyManager::PhotographyManager()
: IDependencyManagedComponent(this, RobotComponentID::PhotographyManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
{
  _visionComponent = robot->GetComponentPtr<VisionComponent>();

  const auto& config = robot->GetContext()->GetDataLoader()->GetPhotographyConfig();
  kMaxSlots = JsonTools::GetValue<s32>(config["MaxSlots"]);
  kRemoveDistortion = JsonTools::ParseBool(config, "RemoveDistortion", kModuleName);
  kSaveQuality = JsonTools::GetValue<int8_t>(config["SaveQuality"]);
  kThumbnailScale = JsonTools::GetValue<float>(config["ThumbnailScale"]);
  
  _platform = robot->GetContextDataPlatform();
  DEV_ASSERT(_platform != nullptr, "PhotographyManager.InitDependent.DataPlatformIsNull");

  _savePath = _platform->pathToResource(Util::Data::Scope::Persistent, kPhotoManagerFolder);
  if (!Util::FileUtils::CreateDirectory(_savePath))
  {
    LOG_ERROR("PhotographyManager.InitDependent.FailedToCreateFolder", "Failed to create folder %s", _savePath.c_str());
    return;
  }
  
  _fullPathPhotoInfoFile = Util::FileUtils::FullFilePath({_savePath, kPhotoManagerFilename});
  if (Util::FileUtils::FileExists(_fullPathPhotoInfoFile))
  {
    if (!LoadPhotosFile())
    {
      LOG_ERROR("PhotographyManager.InitDependent.FailedLoadingPhotosFile", "Error loading photos file");
    }
  }
  else
  {
    LOG_WARNING("PhotographyManager.InitDependent.NoPhotosFile", "Photos file not found; creating new one");
    SavePhotosFile();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string& PhotographyManager::GetStateString() const
{
  static const std::map<State, std::string> LUT{
    {State::Idle,                      "Idle"},
    {State::WaitingForPhotoModeEnable, "WaitingForPhotoModeEnable"},
    {State::InPhotoMode,               "InPhotoMode"},
    {State::WaitingForTakePhoto,       "WaitingForTakePhoto"},
    {State::WaitingForPhotoModeDisable,"WaitingForPhotoModeDisable"},
  };
  
  auto iter = LUT.find(_state);
  DEV_ASSERT(iter != LUT.end(), "PhotographyManager.GetSate.BadState");
  return iter->second;
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
        LOG_INFO("PhotographyManager.UpdateDependent.CompletedPhotoModeDisable",
                 "Was in State %s, switching to Idle", GetStateString().c_str());
        _state = State::Idle;
      }
      else
      {
        LOG_INFO("PhotographyManager.UpdateDependent.CompletedPhotoModeEnable",
                 "Was in State %s, switching to InPhotoMode", GetStateString().c_str());
        _state = State::InPhotoMode;
      }
    }
  }
  
  // Make sure we get photo mode disabled once possible, if requested
  if(_disableWhenPossible && IsReadyToSwitchModes())
  {
    LOG_INFO("PhotographyManager.UpdateDependent.DisablingPhotoMode",
             "Disable was queued. Doing now.");
    _visionComponent->SetCameraCaptureFormat(ImageEncoding::RawRGB);
    _state = State::WaitingForPhotoModeDisable;
    _disableWhenPossible = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PhotographyManager::EnablePhotoMode(bool enable)
{
  if(!kTakePhoto_UseSensorResolution)
  {
    // No format change: immediately in photo mode
    _state = State::InPhotoMode;
    return RESULT_OK;
  }
  
  DEV_ASSERT(nullptr != _visionComponent, "PhotographyManager.EnablePhotoMode.NullVisionComponent");

  if(!IsReadyToSwitchModes())
  {
    // Special case: can't disable photo mode because we're waiting on something else: queue to disable when ready
    if(!enable)
    {
      LOG_INFO("PhotographyManager.EnablePhotoMode.QueuingDisable", "Current State: %s",
               GetStateString().c_str());
      _disableWhenPossible = true;
      return RESULT_OK;
    }
    
    // Should have checked IsReady before calling!
    LOG_WARNING("PhotographyManager.EnablePhotoMode.NotReadyToSwitchModes",
                "Trying to enable. Current State: %s",
                GetStateString().c_str());
    return RESULT_FAIL;
  }

  const ImageEncoding format = (enable ? ImageEncoding::YUV420sp : ImageEncoding::RawRGB);
  _visionComponent->SetCameraCaptureFormat(format);
  _state = (enable ? State::WaitingForPhotoModeEnable : State::WaitingForPhotoModeDisable);
  
  LOG_INFO("PhotographyManager.EnablePhotoMode.FormatChange",
           "Requesting format: %s, New State: %s", EnumToString(format), GetStateString().c_str());

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
  return kDevIsStorageFull || _photoInfos.size() >= kMaxSlots;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string PhotographyManager::GetSavePath() const
{
  return _savePath;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string PhotographyManager::GetBasename(int photoID) const
{
  std::ostringstream ss;
  ss << std::setw(6) << std::setfill('0') << photoID;
  return "photo_" + ss.str();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhotographyManager::PhotoHandle PhotographyManager::TakePhoto()
{
  DEV_ASSERT(nullptr != _visionComponent, "PhotographManager.TakePhoto.NullVisionComponent");

  if(!IsReadyToTakePhoto())
  {
    LOG_WARNING("PhotographyManager.TakePhoto.NotReady", "Current State: %s", GetStateString().c_str());
    return 0;
  }

  const auto photoSize = (kTakePhoto_UseSensorResolution ?
                          Vision::ImageCache::Size::Sensor :
                          Vision::ImageCache::Size::Full);
    
  _visionComponent->SetSaveImageParameters(ImageSendMode::SingleShot,
                                           GetSavePath(),
                                           GetBasename(_nextPhotoID),
                                           kSaveQuality,
                                           photoSize,
                                           kRemoveDistortion,
                                           kThumbnailScale);

  _lastRequestedPhotoHandle = _visionComponent->GetLastProcessedImageTimeStamp();
  _state = State::WaitingForTakePhoto;
  
  LOG_INFO("PhotographyManager.TakePhoto.SetSaveParams",
           "Resolution: %s, RequestedHandle: %zu",
           (kTakePhoto_UseSensorResolution ? "Sensor" : "Full"),
           _lastRequestedPhotoHandle);
  
  return _lastRequestedPhotoHandle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::SetLastPhotoTimeStamp(TimeStamp_t timestamp)
{
  static_assert(sizeof(PhotoHandle) >= sizeof(TimeStamp_t), "PhotoHandle alias must be able to accomodate TimeStamps");
  _lastSavedPhotoHandle = timestamp;
  
  LOG_INFO("PhotographyManager.SetLastPhotoTimeStamp.SettingHandle",
           "Saved Handle: %zu (Last Requested: %zu)",
           _lastSavedPhotoHandle, _lastRequestedPhotoHandle);
  
  if(WasPhotoTaken(_lastRequestedPhotoHandle))
  {
    // Last TakePhoto call is completed, go back to InPhotoMode, meaning we are ready to take more photos
    _state = State::InPhotoMode;

    // Record info about the photo in the 'photos info'
    // Note: When VIC-3649 "Add engine support for getting the current wall time" is done, use it here?
    using namespace std::chrono;
    const auto time_s = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    const auto epochTimestamp = static_cast<int>(time_s);
    static const bool copiedToApp = false;
    _photoInfos.push_back(PhotoInfo(_nextPhotoID, epochTimestamp, copiedToApp));
    const auto index = static_cast<uint32_t>(_photoInfos.size() - 1);
    LOG_INFO("PhotographyManager.SetLastPhotoTimeStamp",
             "Photo with ID %i and epoch date/time %i saved at index %u",
             _nextPhotoID, epochTimestamp, index);

    _nextPhotoID++;

    SavePhotosFile();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::WasPhotoTaken(const PhotoHandle handle) const
{
  return (_lastSavedPhotoHandle > handle);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::LoadPhotosFile()
{
  Json::Value data;
  if (!_platform->readAsJson(_fullPathPhotoInfoFile, data))
  {
    LOG_ERROR("PhotographyManager.LoadPhotosFile.Failed", "Failed to read %s", _fullPathPhotoInfoFile.c_str());
    return false;
  }

  _nextPhotoID = data[kNextPhotoIDKey].asInt();

  _photoInfos.clear();
  for (const auto& info : data[kPhotoInfosKey])
  {
    const int id = info[kIDKey].asInt();
    const TimeStamp_t dateTimeTaken = info[kTimeStampKey].asUInt();
    const bool copied = info[kCopiedKey].asBool();

    _photoInfos.push_back(PhotoInfo(id, dateTimeTaken, copied));
  }
  
  const auto numPhotos = _photoInfos.size();
  if (numPhotos > kMaxSlots)
  {
    LOG_WARNING("PhotographyManager.LoadPhotosFile.DeletingPhotos",
                "Removing some photos because there are now fewer slots (configuration change)");
    // Delete the oldest photo(s)
    for (int i = 0; i < numPhotos - kMaxSlots; i++)
    {
      // Delete the first photo in the list; DeletePhotoByID will compress the list
      static const bool savePhotosFile = false;
      DeletePhotoByID(_photoInfos[0]._id, savePhotosFile);
    }
    SavePhotosFile();
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::SavePhotosFile() const
{
  Json::Value data;

  data[kNextPhotoIDKey] = _nextPhotoID;
  Json::Value emptyArray(Json::arrayValue);
  data[kPhotoInfosKey] = emptyArray;
  for (const auto& info : _photoInfos)
  {
    Json::Value infoJson;
    infoJson[kIDKey]        = info._id;
    infoJson[kTimeStampKey] = info._dateTimeTaken;
    infoJson[kCopiedKey]    = info._copiedToApp;
    
    data[kPhotoInfosKey].append(infoJson);
  }

  if (!_platform->writeAsJson(_fullPathPhotoInfoFile, data))
  {
    LOG_ERROR("PhotographyManager.SavePhotosFile.Failed", "Failed to write photos info file");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::DeletePhotoByID(const int id, const bool savePhotosFile)
{
  int index = PhotoIndexFromID(id);
  if (index < 0)
  {
    return false;
  }

  // Delete the photo itself, and its thumbnail, from disk
  const auto& baseName = GetBasename(id);
  Util::FileUtils::DeleteFile(Util::FileUtils::FullFilePath({_savePath, baseName + "." + GetPhotoExtension()}));
  Util::FileUtils::DeleteFile(Util::FileUtils::FullFilePath({_savePath, baseName + "." + GetThumbExtension()}));

  // Update the 'slots database'
  const auto newSize = _photoInfos.size() - 1;
  for ( ; index < newSize; index++)
  {
    _photoInfos[index] = _photoInfos[index + 1];
  }
  _photoInfos.resize(newSize);

  LOG_INFO("PhotographyManager.DeletePhotoByID", "Photo with ID %i, at index %i, deleted", id, index);
  if (savePhotosFile)
  {
    SavePhotosFile();
  }
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::SendPhotosInfo() const
{
  // TODO: Send the list of photoinfos to the app (via protobuff message)
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::SendPhotoByID(const int id)
{
  static const bool isThumbnail = false;
  return SendImageHelper(id, isThumbnail);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::SendThumbnailByID(const int id)
{
  static const bool isThumbnail = true;
  return SendImageHelper(id, isThumbnail);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::SendImageHelper(const int id, const bool isThumbnail)
{
  const int index = PhotoIndexFromID(id);
  if (index < 0)
  {
    return false;
  }

  const auto fullPath = Util::FileUtils::FullFilePath({_savePath,
                                                       GetBasename(id) + "." +
                                                       (isThumbnail ? GetThumbExtension() : GetPhotoExtension())});

  // TODO: Read jpg file off disk and send contents to the app (via protobuff message)

  LOG_INFO("PhotographyManager.SendImageHelper", "%s with ID %i, at index %i, sent",
           (isThumbnail ? "Thumbnail" : "Photo"), id, index);

  if (!isThumbnail)
  {
    const auto prevCopiedToApp = _photoInfos[index]._copiedToApp;
    _photoInfos[index]._copiedToApp = true;
    if (!prevCopiedToApp)
    {
      SavePhotosFile();
    }
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PhotographyManager::PhotoIndexFromID(const int id) const
{
  int index = 0;
  for ( ; index < _photoInfos.size(); index++)
  {
    if (_photoInfos[index]._id == id)
    {
      break;
    }
  }
  if (index >= _photoInfos.size())
  {
    LOG_ERROR("PhotographyManager.PhotoIndexFromID", "Photo ID %i not found", id);
    return -1;
  }
  return index;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//   TODO:  Handle the following requests that will be coming from the App:
//     GetPhotoInfos{}
//     GetPhotoForID{int id}
//     GetThumbnailForID{int id}
//     DeletePhotoForID{int id}
// (The last 3 of these messages could potentially be "ForIndex", but by ID is safer.
//  Imagine several Delete requests coming quickly; each delete can change the indexes of other photos.)



  
} // namespace Cozmo
} // namespace Anki
