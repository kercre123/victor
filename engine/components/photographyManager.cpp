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
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "engine/vision/imageSaver.h"
#include "webServerProcess/src/webService.h"

#include "proto/external_interface/shared.pb.h"

#include "util/console/consoleInterface.h"

#include <sstream>
#include <iomanip>


#define LOG_CHANNEL "PhotographyManager"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kDevIsStorageFull, "Photography", false);

// If true, requires OS version that supports camera format change
CONSOLE_VAR(bool, kTakePhoto_UseSensorResolution, "Photography", true);
    

static int DeleteAllPhotosWebServerImpl(WebService::WebService::Request* request)
{
  auto* photographyManager = static_cast<PhotographyManager*>(request->_cbdata);
  photographyManager->DeleteAllPhotos();
  
  return 1;
}


// Note that this can be called at any arbitrary time, from a webservice thread
static int DeleteAllPhotosWebServerHandler(struct mg_connection *conn, void *cbdata)
{
  auto* photographyManager = static_cast<PhotographyManager*>(cbdata);
  
  auto ws = photographyManager->GetRobot()->GetContext()->GetWebService();
  const int returnCode = ws->ProcessRequestExternal(conn, cbdata, DeleteAllPhotosWebServerImpl);
  
  return returnCode;
}


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
  static const std::string kPhotoCopiedKey = "Copied";
  static const std::string kThumbCopiedKey = "CopiedThumb";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
PhotographyManager::PhotographyManager()
: IDependencyManagedComponent(this, RobotComponentID::PhotographyManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  _visionComponent = robot->GetComponentPtr<VisionComponent>();
  _gatewayInterface = robot->GetGatewayInterface();

  auto* gi = _gatewayInterface;
  if (gi != nullptr)
  {
    auto commonCallback = std::bind(&PhotographyManager::HandleEvents, this, std::placeholders::_1);
    
    // Subscribe to desired simple events
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPhotosInfoRequest,  commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPhotoRequest,       commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kThumbnailRequest,   commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDeletePhotoRequest, commonCallback));
  }

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

  const auto& webService = robot->GetContext()->GetWebService();
  webService->RegisterRequestHandler("/deleteallphotos", DeleteAllPhotosWebServerHandler, this);
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
    _visionComponent->EnableSensorRes(false);
    _state = State::WaitingForPhotoModeDisable;
    _disableWhenPossible = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result PhotographyManager::EnablePhotoMode(bool enable)
{
  if(!UseSensorResolution())
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

  _visionComponent->EnableSensorRes(enable);
  _state = (enable ? State::WaitingForPhotoModeEnable : State::WaitingForPhotoModeDisable);
  
  LOG_INFO("PhotographyManager.EnablePhotoMode.FormatChange",
           "%s full resolution", (enable ? "Enabling" : "Disabling"));

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
bool PhotographyManager::UseSensorResolution() const
{
  // NOTE: using half-res (aka "Full") for PRDemo until VIC-4077 is fixed
  return (kTakePhoto_UseSensorResolution
          && !_robot->GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::PRDemo));
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

  const bool useSensorRes = UseSensorResolution();
  const auto photoSize = (useSensorRes ?
                          Vision::ImageCache::Size::Sensor :
                          Vision::ImageCache::Size::Full);
  
  // Upsampling half-res to full scale for PRDemo until VIC-4077 is fixed
  const float saveScale = (_robot->GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::PRDemo) ? 2.f : 1.f);
  
  _visionComponent->SetSaveImageParameters(ImageSaverParams(GetSavePath(),
                                                            ImageSendMode::SingleShot,
                                                            kSaveQuality,
                                                            GetBasename(_nextPhotoID),
                                                            photoSize,
                                                            kThumbnailScale,
                                                            saveScale,
                                                            kRemoveDistortion));

  _lastRequestedPhotoHandle = (TimeStamp_t)_visionComponent->GetLastProcessedImageTimeStamp();
  _state = State::WaitingForTakePhoto;
  
  LOG_INFO("PhotographyManager.TakePhoto.SetSaveParams",
           "Resolution: %s, RequestedHandle: %zu",
           (useSensorRes ? "Sensor" : "Full"),
           _lastRequestedPhotoHandle);
  
  return _lastRequestedPhotoHandle;
}

void PhotographyManager::CancelTakePhoto()
{
  if(_state == State::WaitingForTakePhoto)
  {
    LOG_INFO("PhotographyManager.CancelTakePhoto.SwitchingToIdle", "");
    _state = State::Idle;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhotographyManager::IsWaitingForPhoto() const
{
  return (_lastRequestedPhotoHandle > 0);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::SetLastPhotoTimeStamp(RobotTimeStamp_t timestamp)
{
  static_assert(sizeof(PhotoHandle) >= sizeof(TimeStamp_t), "PhotoHandle alias must be able to accomodate TimeStamps");
  _lastSavedPhotoHandle = (TimeStamp_t)timestamp;
  
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
    static const bool photoCopiedToApp = false;
    static const bool thumbCopiedToApp = false;
    _photoInfos.push_back(PhotoInfo(_nextPhotoID, epochTimestamp, photoCopiedToApp, thumbCopiedToApp));
    const auto index = static_cast<uint32_t>(_photoInfos.size() - 1);
    LOG_INFO("PhotographyManager.SetLastPhotoTimeStamp",
             "Photo with ID %i and epoch date/time %i saved at index %u",
             _nextPhotoID, epochTimestamp, index);

    if(_robot != nullptr && _robot->HasGatewayInterface())
    {
      auto* gi = _robot->GetGatewayInterface();
      auto* msg = new external_interface::PhotoTaken(_nextPhotoID);
      gi->Broadcast(ExternalMessageRouter::Wrap(msg));
    }

    _nextPhotoID++;
    _lastRequestedPhotoHandle = 0;

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
    const bool photoCopied = info[kPhotoCopiedKey].asBool();
    const bool thumbCopied = info.isMember(kThumbCopiedKey) ? info[kThumbCopiedKey].asBool() : false;

    _photoInfos.push_back(PhotoInfo(id, dateTimeTaken, photoCopied, thumbCopied));
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
    infoJson[kIDKey]          = info._id;
    infoJson[kTimeStampKey]   = info._dateTimeTaken;
    infoJson[kPhotoCopiedKey] = info._photoCopiedToApp;
    infoJson[kThumbCopiedKey] = info._thumbCopiedToApp;

    data[kPhotoInfosKey].append(infoJson);
  }

  if (!_platform->writeAsJson(_fullPathPhotoInfoFile, data))
  {
    LOG_ERROR("PhotographyManager.SavePhotosFile.Failed", "Failed to write photos info file");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::DeleteAllPhotos()
{
  LOG_INFO("PhotographyManager.DeleteAllPhotos", "Deleting all photos");
  bool dirty = false;
  static const bool kSavePhotosFile = false;
  while (!_photoInfos.empty())
  {
    DeletePhotoByID(_photoInfos[0]._id, kSavePhotosFile);
    dirty = true;
  }

  if (dirty)
  {
    SavePhotosFile();
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
bool PhotographyManager::ImageHelper(const int id, const bool isThumbnail, std::string& fullpath)
{
  const int index = PhotoIndexFromID(id);
  if (index < 0)
  {
    return false;
  }

  fullpath = Util::FileUtils::FullFilePath({_savePath,
                                            GetBasename(id) + "." +
                                            (isThumbnail ? GetThumbExtension() : GetPhotoExtension())});
  
  if (!Util::FileUtils::FileExists(fullpath))
  {
    LOG_ERROR("PhotographyManager.ImageHelper", "Error finding file %s", fullpath.c_str());
    return false;
  }


  LOG_INFO("PhotographyManager.ImageHelper", "Getting %s with ID %i, at index %i, with path %s",
           (isThumbnail ? "thumbnail" : "photo"), id, index, fullpath.c_str());

  bool prevCopiedToApp = false;
  if (isThumbnail)
  {
    prevCopiedToApp = _photoInfos[index]._thumbCopiedToApp;
    _photoInfos[index]._thumbCopiedToApp = true;
  }
  else
  {
    prevCopiedToApp = _photoInfos[index]._photoCopiedToApp;
    _photoInfos[index]._photoCopiedToApp = true;
  }
  if (!prevCopiedToApp)
  {
    SavePhotosFile();
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
void PhotographyManager::HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  const auto eventData = event.GetData();
  switch(eventData.GetTag())
  {
    case AppToEngineTag::kPhotosInfoRequest:
      OnRequestPhotosInfo(eventData.photos_info_request());
      break;
    case AppToEngineTag::kPhotoRequest:
      OnRequestPhoto(eventData.photo_request());
      break;
    case AppToEngineTag::kThumbnailRequest:
      OnRequestThumbnail(eventData.thumbnail_request());
      break;
    case AppToEngineTag::kDeletePhotoRequest:
      OnRequestDeletePhoto(eventData.delete_photo_request());
      break;
    default:
      LOG_ERROR("PhotographyManager.HandleEvents",
                "HandleEvents called for unknown message");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::OnRequestPhotosInfo(const external_interface::PhotosInfoRequest& photosInfoRequest)
{
  LOG_INFO("PhotographyManager.OnRequestPhotosInfo", "Photos info requested");
  auto* photosInfoResp = new external_interface::PhotosInfoResponse();
  for (auto i = 0; i < _photoInfos.size(); i++)
  {
    const auto& item = _photoInfos[i];
    external_interface::PhotoInfo* photoInfo = photosInfoResp->add_photo_infos();
    photoInfo->set_photo_id(item._id);
    photoInfo->set_timestamp_utc(item._dateTimeTaken);
    photoInfo->set_photo_copied_to_app(item._photoCopiedToApp);
    photoInfo->set_thumb_copied_to_app(item._thumbCopiedToApp);
  }
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(photosInfoResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::OnRequestPhoto(const external_interface::PhotoRequest& photoRequest)
{
  const auto photoId = photoRequest.photo_id();
  LOG_INFO("PhotographyManager.OnRequestPhoto", "Requesting photo with ID %u", photoId);

  std::string fullpath;
  static const bool isThumbnail = false;
  const bool success = ImageHelper(photoId, isThumbnail, fullpath);

  auto* msg = new external_interface::PhotoPathMessage();
  msg->set_success(success);
  msg->set_full_path(fullpath);

  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(msg));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::OnRequestThumbnail(const external_interface::ThumbnailRequest& thumbnailRequest)
{
  const auto photoId = thumbnailRequest.photo_id();
  LOG_INFO("PhotographyManager.OnRequestThumbnail", "Requesting thumbnail with ID %u", photoId);

  std::string fullpath;
  static const bool isThumbnail = true;
  const bool success = ImageHelper(photoId, isThumbnail, fullpath);

  auto* msg = new external_interface::ThumbnailPathMessage();
  msg->set_success(success);
  msg->set_full_path(fullpath);

  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(msg));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhotographyManager::OnRequestDeletePhoto(const external_interface::DeletePhotoRequest& deletePhotoRequest)
{
  const auto photoId = deletePhotoRequest.photo_id();
  LOG_INFO("PhotographyManager.OnRequestDeletePhoto", "Deleting photo with ID %u", photoId);
  static const bool saveChangeToDisk = true;
  const bool success = DeletePhotoByID(photoId, saveChangeToDisk);
  if (!success)
  {
    LOG_ERROR("PhotographyManager.OnRequestDeletePhoto", "Failed to delete photo with ID %u", photoId);
  }


  auto* deletePhotoResp = new external_interface::DeletePhotoResponse();
  deletePhotoResp->set_success(success);
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(deletePhotoResp));
}


} // namespace Cozmo
} // namespace Anki
