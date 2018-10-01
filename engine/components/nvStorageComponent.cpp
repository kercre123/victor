/**
 * File: NVStorageComponent.cpp
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Facilities for writing/reading non-volatile files. 
 *              Less useful on Vector than it was on Cozmo, but still used in playpen so we're still hanging onto it.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "coretech/common/robot/errorHandling.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"

#ifdef SIMULATOR
#include "camera/cameraService.h"
#include "clad/types/imageTypes.h"
#endif // ifdef SIMULATOR

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "util/fileUtils/fileUtils.h"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include <fcntl.h>

static const char*   _kNVDataFileExtension = ".nvdata";

static const char*   _kFactoryPath = "/factory/nvStorage/";

static const u32     _kCameraCalibMagic = 0x0DDFACE5;

namespace Anki {
namespace Vector {

using namespace NVStorage;
  
NVStorageComponent::NVStorageComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::NVStorage)
{
}

NVStorageComponent::~NVStorageComponent()
{
}

void NVStorageComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) 
{
  _robot = robot;
  _kStoragePath = (_robot->GetContextDataPlatform() != nullptr ? _robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, "nvStorage/") : "");
  #ifdef SIMULATOR
  LoadSimData();
  #endif

  LoadDataFromFiles();
}

bool NVStorageComponent::IsValidEntryTag(NVEntryTag tag) const {
  return EnumToString(tag) != nullptr;
}

bool NVStorageComponent::IsFactoryEntryTag(NVEntryTag tag) const
{
  return IsValidEntryTag(tag) && (static_cast<u32>(tag) & static_cast<u32>(NVConst::NVConst_FACTORY_DATA_BIT));
}

bool NVStorageComponent::Write(NVEntryTag tag,
                               const std::vector<u8>* data,
                               NVStorageWriteEraseCallback callback)
{
  // Check for null data
  if (data == nullptr) {
    PRINT_CH_INFO("NVStorage", "NVStorageComponent.Write.NullData", "%s", EnumToString(tag));
    return false;
  }
  
  return Write(tag, data->data(), data->size(), callback);
}
  
bool NVStorageComponent::Write(NVEntryTag tag,
                               const u8* data, size_t size,
                               NVStorageWriteEraseCallback callback)
{
  bool validArgs = true;
  
  if (IsFactoryEntryTag(tag) && !_writingFactory) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.FactoryTagNotAllowed",
                        "Tag: %s (0x%x)", EnumToString(tag), tag);
    validArgs = false;
  }
  
  // Check for null data
  if (data == nullptr) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.NullData", "%s", EnumToString(tag));
    validArgs = false;
  }
  
  // All data writes must be word-aligned
  size = MakeWordAligned(size);
  
  if(!_writingFactory)
  {
    // Sanity check on size
    if (size <= 0) {
      PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidSize",
                          "Tag: %s, %zu bytes",
                          EnumToString(tag), size);
      validArgs = false;
    }
  }

  // Fail immediately if invalid args found
  if (!validArgs) {
    if (callback) {
      callback(NVResult::NV_BAD_ARGS);
    }
    return false;
  }
  
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Write.WritingData", "%s", EnumToString(tag));
  
  _tagDataMap[tag] = std::vector<u8>(data,data+size);
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Write.ExecutingCallback", "%s", EnumToString(tag));
    callback(NVResult::NV_OKAY);
  }
  
  return WriteEntryToFile(tag);
}

bool NVStorageComponent::Erase(NVEntryTag tag,
                               NVStorageWriteEraseCallback callback)
{
  return Erase(tag, callback, 0);
}
  
bool NVStorageComponent::Erase(NVEntryTag tag,
                               NVStorageWriteEraseCallback callback,
                               u32 eraseSize)
{
  bool validArgs = true;
  
  if (IsFactoryEntryTag(tag) && !_writingFactory) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.FactoryTagNotAllowed",
                        "Tag: %s (0x%x)", EnumToString(tag), tag);
    validArgs = false;
  }
  
  // Fail immediately if invalid args found
  if (!validArgs) {
    if (callback) {
      callback(NVResult::NV_BAD_ARGS);
    }
    return false;
  }

  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Erase.ErasingData", "%s", EnumToString(tag));
  
  _tagDataMap.erase(tag);
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Erase.ExecutingCallback", "%s", EnumToString(tag));
    callback(NVResult::NV_OKAY);
  }
  
  return WriteEntryToFile(tag);
}
  
  
bool NVStorageComponent::WipeAll(NVStorageWriteEraseCallback callback)
{
  _tagDataMap.clear();
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WipeAll.ExecutingCallback", "");
    callback(NVResult::NV_OKAY);
  }
  
  Util::FileUtils::RemoveDirectory(_kStoragePath);
   
  return true;
}

bool NVStorageComponent::WipeFactory(NVStorageWriteEraseCallback callback)
{
  if(_writingFactory)
  {
    Util::FileUtils::RemoveDirectory(_kFactoryPath);
    if(callback != nullptr)
    {
      callback(NVStorage::NVResult::NV_OKAY);
    }
    return true;
  }
  PRINT_NAMED_ERROR("NVStorageComponent.WipeFactory.NotAllowed",
                    "Must be allowed to write to factory addresses");
  return false;
}
  
bool NVStorageComponent::Read(NVEntryTag tag,
                              NVStorageReadCallback callback,
                              std::vector<u8>* data)
{
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Read.ReadingData", "%s", EnumToString(tag));
  NVResult result = NVResult::NV_OKAY;
  
  const auto dataIter = _tagDataMap.find(tag);
  const bool dataExists = dataIter != _tagDataMap.end();
  
  u8* dataPtr = nullptr;
  size_t dataSize = 0;
  if (dataExists) {
    dataPtr = dataIter->second.data();
    dataSize = dataIter->second.size();
    
    // Copy data to destination
    if (data != nullptr) {
      *data = dataIter->second;
    }
  } else {
    result = NVResult::NV_NOT_FOUND;
  }
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Read.ExecutingCallback", "%s", EnumToString(tag));
    callback(dataPtr, dataSize, result);
  }
  
  return true;
}

bool NVStorageComponent::WriteEntryToFile(NVEntryTag tag)
{
  // Not writing data to disk on Mac OS so as make sure Webots robots load "clean"
  // which is usually the assumption when using Webots robots.
  // If you bypass this it's your own responsibility to delete webotsCtrlGameEngine2/files/output/nvStorage
  // to get a clean system again.
  #if !defined(ANKI_PLATFORM_OSX)
  
  // Create filename from tag
  std::stringstream ss;
  ss << std::hex << static_cast<u32>(tag) << _kNVDataFileExtension;
  
  std::string path = _kStoragePath;
  // TODO(Al): This probably needs to write somewhere else for actual factory nvStorage
  if(IsFactoryEntryTag(tag))
  {
    path = _kFactoryPath;
  }

  // If tag doesn't exist in map, then delete the associated file if it exists
  auto iter = _tagDataMap.find(tag);
  if (iter == _tagDataMap.end()) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Erasing", "Tag %s", EnumToString(tag));
    Util::FileUtils::DeleteFile(path + ss.str());
    return true;
  }

  // Camera calibration is special since it lives in a block device
  if(tag == NVEntryTag::NVEntry_CameraCalib)
  {
    return WriteCameraCalibFile(iter);
  }
  
  // Write data to file
  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Writing", "Tag %s, size %zu", EnumToString(tag), iter->second.size());
  Util::FileUtils::CreateDirectory(path);
  if (!Util::FileUtils::WriteFile(path + ss.str(), iter->second)) {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.Failed", "%s", ss.str().c_str());
    return false;
  }

  sync();

  #endif // if !defined(ANKI_PLATFORM_OSX)
  return true;
}
  
void NVStorageComponent::LoadDataFromFiles()
{
  auto fileList = Util::FileUtils::FilesInDirectory(_kStoragePath, false, _kNVDataFileExtension);
  
  for (auto& fileName : fileList) {
    
    // Remove extension
    const std::string tagStr = fileName.substr(0, fileName.find_last_of("."));
    
    // Check for valid fileName (Must be numeric and a valid tag)
    char *end;
    u32 tagNum = static_cast<u32>(strtol(tagStr.c_str(), &end, 16));
    NVEntryTag tag = static_cast<NVEntryTag>(tagNum);
    if (!IsValidEntryTag(tag)) {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.InvalidTagValues", "0x%x", tagNum);
      continue;
    }
    
    // Read data from file
    std::vector<u8> file = Util::FileUtils::ReadFileAsBinary(_kStoragePath + fileName);
    if(file.empty())
    {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.ReadFileFailed",
                        "Unable to read nvStorage entry file %s", fileName.c_str());
      continue;
    }
    
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.LoadDataFromFiles.LoadingData", "Tag %s, data size = %zu bytes", EnumToString(tag), file.size());
    _tagDataMap[tag].assign(file.data(), file.data() + file.size());
  }

  // Attempt to read camera calibration from the block device
  ReadCameraCalibFile();
 
  // TODO: For now load factory related nvstorage files from the "factory" subdirectory. Will probably
  // be moved to some read only place
  fileList = Util::FileUtils::FilesInDirectory(_kFactoryPath, false, _kNVDataFileExtension);
  
  for (auto& fileName : fileList) {
    
    // Remove extension
    const std::string tagStr = fileName.substr(0, fileName.find_last_of("."));
    
    // Check for valid fileName (Must be numeric and a valid tag)
    char *end;
    u32 tagNum = static_cast<u32>(strtoul(tagStr.c_str(), &end, 16));
    NVEntryTag tag = static_cast<NVEntryTag>(tagNum);
    if (!IsFactoryEntryTag(tag)) {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadFactoryDataFromFiles.InvalidTagValues", "0x%x", tagNum);
      continue;
    }

    // Camera calibration is special and is stored in a block device so we should not be trying
    // to load it from the normal nvstorage files
    if(tag == NVEntryTag::NVEntry_CameraCalib)
    {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadFactoryDataFromFiles.CameraCalibFile",
                        "Trying to load camera calibration from nvStorage file when it should be in the block device");
      continue;
    }

    
    // Read data from file
    std::vector<u8> file = Util::FileUtils::ReadFileAsBinary(_kFactoryPath + fileName);
    if(file.empty())
    {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadFactoryDataFromFiles.ReadFileFailed",
                        "Unable to read nvStorage entry file %s", fileName.c_str());
      continue;
    }
    
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.LoadFactoryDataFromFiles.LoadingData", "Tag 0x%x, data size = %zu bytes", tagNum, file.size());
    _tagDataMap[tag].assign(file.data(), file.data() + file.size());
  }
}
  
void NVStorageComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
}
  
#ifdef SIMULATOR
void NVStorageComponent::LoadSimData()
{
  // Store simulated camera calibration data
  const CameraCalibration* camCalib = CameraService::getInstance()->GetHeadCamInfo();
  
  _tagDataMap[NVEntryTag::NVEntry_CameraCalib].assign(reinterpret_cast<const u8*>(camCalib), reinterpret_cast<const u8*>(camCalib) + sizeof(*camCalib));
}
#endif  // ifdef SIMULATOR
  
size_t NVStorageComponent::MakeWordAligned(size_t size) {
  u8 numBytesToMakeAligned = 4 - (size % 4);
  if (numBytesToMakeAligned < 4) {
    return size + numBytesToMakeAligned;
  }
  return size;
}

int NVStorageComponent::OpenCameraCalibFile(int openFlags)
{
  static const char* kBlockDevice = "/dev/block/bootdevice/by-name/emr";
  int fd = open(kBlockDevice, openFlags);
  if(fd == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.OpenCameraCalibFile.FailedToOpenBlockDevice",
                      "%d", errno);
    return -1;
  }

  // Camera calibration is offest by 4mb in the block device
  const off_t kCalibOffset = 4194304;
  const off_t seek = lseek(fd, kCalibOffset, SEEK_SET);
  if(seek == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.OpenCameraCalibFile.FailedToSeek",
                      "%d", errno);
    return -1;
  }

  return fd;
}

void NVStorageComponent::ReadCameraCalibFile()
{
  #ifdef SIMULATOR
  return;
  #endif
  
  int fd = OpenCameraCalibFile(O_RDONLY);
  if(fd == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.FailedToOpenCameraCalib","");
    return;
  }

  const u32 kSizeOfMagic = sizeof(_kCameraCalibMagic);
  const u32 kSizeOfCalib = sizeof(CameraCalibration);
  const u32 kNumBytesForCalib = kSizeOfMagic + kSizeOfCalib; 
  const u32 kNumBytesToRead = 4096; // Read a whole page due to this being a block device
  static_assert(kNumBytesToRead >= kNumBytesForCalib, "Bytes to read smaller than size of camera calibration");

  u8 buf[kNumBytesToRead];
  ssize_t numBytesRead = 0;
  do
  {
    numBytesRead += read(fd, buf + numBytesRead, kNumBytesToRead - numBytesRead);
    if(numBytesRead == -1)
    {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.FailedToReadCameraCalib",
                      "%d", errno);
      return;
    }
  }
  while(numBytesRead < kNumBytesToRead);

  static_assert(std::is_same<decltype(_kCameraCalibMagic), const u32>::value,
                "CameraCalibMagic must be const u32");
  const u32 dataMagic = (u32)(*(u32*)buf);
  if(dataMagic != _kCameraCalibMagic)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.CameraCalibMagicMismatch",
                      "Read magic 0x%x and expected magic 0x%x do not match",
                      dataMagic,
                      _kCameraCalibMagic);
    return;
  }

  // Copy camera calibration into a vector and assign to tagDataMap
  std::vector<u8> data;
  std::copy(&buf[kSizeOfMagic], &buf[kNumBytesForCalib], std::back_inserter(data));
  _tagDataMap[NVEntryTag::NVEntry_CameraCalib].assign(data.data(), data.data() + data.size());

  int rc = close(fd);
  if(rc == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.FailedToCloseCameraCalib",
                      "%d", errno);
  }
}

bool NVStorageComponent::WriteCameraCalibFile(const TagDataMap::iterator& iter)
{
  if(iter->first != NVEntryTag::NVEntry_CameraCalib)
  {
    PRINT_NAMED_WARNING("NVStorageComponent.WriteCameraCalibFile.BadIter",
                        "Expected iter to point to camera calibration");
    return false;
  }
  
  int fd = OpenCameraCalibFile(O_WRONLY);
  if(fd == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.FailedToOpenCameraCalib","");
    return false;
  }
      
  const u32 kNumBytesForCalib = sizeof(_kCameraCalibMagic) + sizeof(CameraCalibration);
  u8 buf[kNumBytesForCalib];
  *((u32*)buf) = _kCameraCalibMagic;
  std::copy(iter->second.begin(), iter->second.end(), buf + sizeof(_kCameraCalibMagic));

  bool ret = true;
  ssize_t numBytesWritten = write(fd, buf, kNumBytesForCalib);
  if(numBytesWritten == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.FailedToWriteBlockDevice",
                      "%d", errno);
    
    ret = false;
  }
  else if(numBytesWritten != kNumBytesForCalib)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.FailedToWriteBlockDevice",
                      "Only wrote %zd bytes instead of %d",
                      numBytesWritten,
                      kNumBytesForCalib);
    ret = false;
  }
    
  int rc = close(fd);
  if(rc == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.FailedToCloseBlockDevice",
                      "%d", errno);
    ret = false;
  }

  sync();

  return ret;
}  
  
} // namespace Vector
} // namespace Anki
