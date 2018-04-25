/**
 * File: NVStorageComponent.cpp
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Interface for storing and retrieving data from robot's non-volatile storage
 *         
 *              Writes
 *              =========
 *              - Prepends an Erase command (See 'Erases' below). Flash blocks must be erased before they can be written.
 *              - When erase completes, sends NVCommand for each 1K blob of data to send, prepending a NVStorageHeader to the first blob.
 *              - ACKs each blob before sending the next. Retries sending individual blobs as necessary.
 *              - Complete when all blobs have been sent and ACK'd.
 *
 *              Erases
 *              ==========
 *              - Sends a NVCommand to erase the maximum possible size allocated for the given tag.
 *                (For tags with large allocation, like NVEntry_FaceAlbumData, this is not that efficient.
 *                We may consider first reading the amount of data in the tag and then erasing only that amount,
 *                but that makes things a little more complicated.
 *              - Completed when ACK'd
 *
 *              Reads
 *              ==========
 *              - Sends a NVCommand to read 1K for the given tag.
 *              - When the 1K is received, it parses the NVStorageHeader to see how much data there is in total.
 *                If the amount of data is already contained in the 1K that was retrieved, the request is complete.
 *              - If it is not, re-requests data for this tag for the proper amount.
 *              - NVOpResult messages containing data will continue arriving with result NV_MORE until the last one
 *                which should have NV_OKAY.
 *
 *
 *              Factory Entry tags
 *              ==================
 *              - These tags are read-only
 *              - When reading, the NVCommand.length param is the total number of tags to read instead of the number of bytes.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "coretech/common/robot/errorHandling.h"
#include "engine/ankiEventUtil.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/console/consoleInterface.h"
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

// Immediately returns on write/erase commands as if they succeeded.
CONSOLE_VAR(bool, kNoWriteToRobot, "NVStorageComponent", false);

// When enabled this does what kNoWriteToRobot does
// as well as make all read commands fail with NV_NOT_FOUND.
// No nvStorage request is ever sent to robot.
CONSOLE_VAR(bool, kDisableNVStorage, "NVStorageComponent", false);

// For some inexplicable reason this needs to be here in the cpp instead of in the header
// because we are including consoleInterface.h
// Maximum size of a single blob
static constexpr u32 _kMaxNvStorageBlobSize = 1024;

static const char*   _kNVDataFileExtension = ".nvdata";

static const char*   _kFactoryPath = "/factory/nvStorage/";

static const u32     _kCameraCalibMagic = 0x0DDFACE5;

namespace Anki {
namespace Cozmo {

using namespace NVStorage;

// This map must be poulated with all the non-factory tags that you want to be able to access.
// See nvStorageTypes.clad
std::map<NVEntryTag, u32> NVStorageComponent::_maxSizeTable = {
                                                          {NVEntryTag::NVEntry_GameSkillLevels, 0},
                                                          {NVEntryTag::NVEntry_OnboardingData,  0},
                                                          {NVEntryTag::NVEntry_GameUnlocks,     0},
                                                          {NVEntryTag::NVEntry_FaceEnrollData,  0},
                                                          {NVEntryTag::NVEntry_FaceAlbumData,   0},
                                                          {NVEntryTag::NVEntry_InventoryData,   0},
                                                          {NVEntryTag::NVEntry_LabAssignments,  0},
                                                          {NVEntryTag::NVEntry_NEXT_SLOT,       0} };

// This map must be poulated with all the factory tags that you want to be able to read from.
// See nvStorageTypes.clad
std::map<NVEntryTag, u32> NVStorageComponent::_maxFactoryEntrySizeTable = {
                                                          // Single blob manufacturing entries start here
                                                          {NVEntryTag::NVEntry_BirthCertificate,   0},
                                                          {NVEntryTag::NVEntry_CameraCalib,        0},
                                                          {NVEntryTag::NVEntry_ToolCodeInfo,       0},
                                                          {NVEntryTag::NVEntry_CalibPose,          0},
                                                          {NVEntryTag::NVEntry_CalibMetaInfo,      0},
                                                          {NVEntryTag::NVEntry_ObservedCubePose,   0},
                                                          {NVEntryTag::NVEntry_IMUInfo,            0},
                                                          {NVEntryTag::NVEntry_CliffValOnDrop,     0},
                                                          {NVEntryTag::NVEntry_CliffValOnGround,   0},
                                                          {NVEntryTag::NVEntry_PlaypenTestResults, 0},
                                                          {NVEntryTag::NVEntry_FactoryLock,        0},
                                                          {NVEntryTag::NVEntry_VersionMagic,       0},
                                                          
                                                          // Multi-blob manufacturing entires start here
                                                          {NVEntryTag::NVEntry_CalibImage1,        0},
                                                          {NVEntryTag::NVEntry_CalibImage2,        0},
                                                          {NVEntryTag::NVEntry_CalibImage3,        0},
                                                          {NVEntryTag::NVEntry_CalibImage4,        0},
                                                          {NVEntryTag::NVEntry_CalibImage5,        0},
                                                          {NVEntryTag::NVEntry_CalibImage6,        0},
                                                          
                                                          {NVEntryTag::NVEntry_ToolCodeImageLeft,  0},
                                                          {NVEntryTag::NVEntry_ToolCodeImageRight, 0},
                                                          
                                                          // Pre-playpen tags
                                                          {NVEntryTag::NVEntry_PrePlaypenResults,   0},
                                                          {NVEntryTag::NVEntry_PrePlaypenCentroids, 0},
                                                          {NVEntryTag::NVEntry_IMUAverages,         0} };

  
NVStorageComponent::NVStorageComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::NVStorage)
{
}

NVStorageComponent::~NVStorageComponent()
{
  _signalHandles.clear();
}

void NVStorageComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) 
{
  _robot = robot;
  _kStoragePath = (_robot->GetContextDataPlatform() != nullptr ? _robot->GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, "nvStorage/") : "");
  const CozmoContext* context = robot->GetContext();
  #ifdef SIMULATOR
  LoadSimData();
  #endif

  LoadDataFromFiles();

  if (context) {
    // Setup game message handlers
    IExternalInterface *extInterface = context->GetExternalInterface();
    if (extInterface != nullptr) {
      
      auto helper = MakeAnkiEventUtil(*extInterface, *this, _signalHandles);
      
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageWriteEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageReadEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageEraseEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageWipeAll>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageClearPartialPendingWriteEntry>();
    }

    InitSizeTable();
    
  } else {
    PRINT_NAMED_WARNING("NVStorageComponent.nullContext", "");
  }
}


void NVStorageComponent::InitSizeTable() {
  
  // Compute sizes for non-factory entries
  for (auto currIt = _maxSizeTable.begin(); currIt != _maxSizeTable.end(); ++currIt) {
    auto nextIt = currIt;
    nextIt++;
    if (nextIt != _maxSizeTable.end()) {
      DEV_ASSERT_MSG(IsValidEntryTag(currIt->first),
                     "NVStorageComponent.InitSizeTable.InvalidTag", "0x%x",
                     currIt->first);
      
      currIt->second = static_cast<u32>(nextIt->first) - static_cast<u32>(currIt->first);
      PRINT_CH_INFO("NVStorage", "NVStorageComponent.InitSizeTable",
                    "Max size of 0x%x: %u",
                    currIt->first, currIt->second);
    } else if (currIt->first == NVEntryTag::NVEntry_NEXT_SLOT) {
      currIt->second = static_cast<u32>(NVConst::NVConst_MAX_ADDRESS) - static_cast<u32>(currIt->first);
    } else {
      PRINT_NAMED_ERROR("NVStorageComponent.InitSizeTable.TooLargeTagFound", "0x%x", currIt->first);
    }
  }
  
  // Manually add max factory tag sizes
  _maxSizeTable[NVEntryTag::NVEntry_FactoryBaseTag] = static_cast<u32>(NVEntryTag::NVEntry_FactoryBaseTagWithBCOffset) - static_cast<u32>(NVEntryTag::NVEntry_FactoryBaseTag);
  _maxSizeTable[NVEntryTag::NVEntry_FactoryBaseTagWithBCOffset] = static_cast<u32>(NVConst::NVConst_FACTORY_BLOCK_SIZE) - _maxSizeTable[NVEntryTag::NVEntry_FactoryBaseTag];
  
  
  // Compute lengths for factory entries
  for (auto it = _maxFactoryEntrySizeTable.begin(); it != _maxFactoryEntrySizeTable.end(); ) {
    u32 tag = static_cast<u32>(it->first);
    if (IsPotentialFactoryEntryTag(tag)) {
      if (IsMultiBlobEntryTag(it->first)) {
        it->second = 0xffff;
      } else {
        it->second = 1;
      }
      it++;
    } else {
      PRINT_NAMED_WARNING("NVStorageComponent.InitSizeTable.FactoryTagExpected", "0x%x", tag);
      it = _maxFactoryEntrySizeTable.erase(it);
    }
  }
  
}
 
u32 NVStorageComponent::GetMaxSizeForEntryTag(NVEntryTag tag)
{
  auto it = _maxSizeTable.find(tag);
  if (it == _maxSizeTable.end() || it->first == NVEntryTag::NVEntry_NEXT_SLOT) {
    PRINT_NAMED_WARNING("NVStorageComponent.GetMaxSizeForEntryTag.InvalidTag", "0x%x", tag);
    return 0;
  }
  
  return it->second;
}
  
bool NVStorageComponent::IsMultiBlobEntryTag(NVEntryTag tag) const {
  u32 t = static_cast<u32>(tag);
  return (t & 0x7fff0000) > 0 && !IsSpecialEntryTag(t) && IsFactoryEntryTag(tag);
}
  
bool NVStorageComponent::IsValidEntryTag(NVEntryTag tag) const {
  u32 t = static_cast<u32>(tag);
  bool isValidNonFactoryTag = (t >= static_cast<u32>(NVConst::NVConst_MIN_ADDRESS)) &&
                              (t <  static_cast<u32>(NVConst::NVConst_MAX_ADDRESS)) &&
                              ((t & 0x0fff) == 0) &&    // Address must be 4K multiple
                              (tag != NVEntryTag::NVEntry_NEXT_SLOT) &&
                              (_maxSizeTable.find(tag) != _maxSizeTable.end());
  
  return isValidNonFactoryTag || IsFactoryEntryTag(tag) || IsTagInFactoryBlock(t);
}
 
bool NVStorageComponent::IsFactoryEntryTag(NVEntryTag tag) const
{
  return _maxFactoryEntrySizeTable.find(tag) != _maxFactoryEntrySizeTable.end();
}
  
bool NVStorageComponent::IsPotentialFactoryEntryTag(u32 tag) const
{
  return (tag & static_cast<u32>(NVConst::NVConst_FACTORY_DATA_BIT)) > 0;
}

bool NVStorageComponent::IsTagInFactoryBlock(u32 tag) const {
  const u32 diff = tag - static_cast<u32>(NVEntryTag::NVEntry_FactoryBaseTag);
  return tag >= static_cast<u32>(NVEntryTag::NVEntry_FactoryBaseTag) &&
         diff < static_cast<u32>(NVConst::NVConst_FACTORY_BLOCK_SIZE);
}
  
bool NVStorageComponent::IsSpecialEntryTag(u32 tag) const
{
  return (tag & 0xffff0000) == static_cast<u32>(NVConst::NVConst_FIXTURE_DATA_BIT);
}
  
// Returns the base (i.e. lowest) tag of the multi-blob entry range
// in which the given tag falls.
// See NVEntryTag definition for more details.
NVEntryTag NVStorageComponent::GetBaseEntryTag(u32 tag) const {
  
  // Possible factory tag?
  if (IsPotentialFactoryEntryTag(tag)) {

    for(auto it : _maxFactoryEntrySizeTable) {
      u32 potentialBaseTag = static_cast<u32>(it.first);
      if ((tag == potentialBaseTag) ||
          (((tag & 0x7fff0000) > 0) && ((tag & 0xffff0000) == potentialBaseTag) && !IsSpecialEntryTag(tag)))
      {
        return it.first;
      }
    }
    
    PRINT_NAMED_WARNING("NVStorageComponent.GetBaseEntryTag.FactoryTagNotFound", "0x%x", tag);
    return NVEntryTag::NVEntry_NEXT_SLOT;
  }
  
  
  // If this is a non-factory address, query the maxSizeTable to figure out what the baseTag is
  NVEntryTag baseTag = NVEntryTag::NVEntry_NEXT_SLOT;
  if (tag < static_cast<u32>(baseTag)) {
    for (auto it = _maxSizeTable.rbegin(); it != _maxSizeTable.rend(); ++it) {
      if (tag >= static_cast<u32>(it->first)) {
        return it->first;
      }
    }
  }
  
  if (baseTag == NVEntryTag::NVEntry_NEXT_SLOT) {
    PRINT_NAMED_WARNING("NVStorageComponent.GetBaseEntryTag.TagIsTooSmall", "0x%x", tag);
  }
  
  return baseTag;
}


bool NVStorageComponent::Write(NVEntryTag tag,
                               const std::vector<u8>* data,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  // Check for null data
  if (data == nullptr) {
    PRINT_CH_INFO("NVStorage", "NVStorageComponent.Write.NullData", "%s", EnumToString(tag));
    return false;
  }
  
  return Write(tag, data->data(), data->size(), callback, broadcastResultToGame);
}
  
bool NVStorageComponent::Write(NVEntryTag tag,
                               const u8* data, size_t size,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  bool validArgs = true;
  
  // Check for invalid tags
  if (!IsValidEntryTag(tag)) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidTag",
                        "Tag: %s (0x%x)", EnumToString(tag), tag);
    validArgs = false;
  }
  
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
    u32 allowedSize = GetMaxSizeForEntryTag(tag);
    
    if ((size > allowedSize) || size <= 0) {
      PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidSize",
                          "Tag: %s, %zu bytes (limit %d bytes)",
                          EnumToString(tag), size, allowedSize);
      validArgs = false;
    }
  }

  // Fail immediately if invalid args found
  if (!validArgs) {
    if (broadcastResultToGame) {
      BroadcastNVStorageOpResult(tag, NVResult::NV_BAD_ARGS, NVOperation::NVOP_WRITE);
    }
    if (callback) {
      callback(NVResult::NV_BAD_ARGS);
    }
    return false;
  }
  
  // If we aren't writing to the robot then call the callback immediately
  if(kDisableNVStorage || kNoWriteToRobot)
  {
    if(broadcastResultToGame)
    {
      BroadcastNVStorageOpResult(tag, NVResult::NV_OKAY, NVOperation::NVOP_WRITE);
    }
    if(callback)
    {
      callback(NVResult::NV_OKAY);
    }
    return true;
  }
  
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Write.WritingData", "%s", EnumToString(tag));
  u32 entryTag = static_cast<u32>(tag);
  
  _tagDataMap[entryTag] = std::vector<u8>(data,data+size);
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Write.ExecutingCallback", "%s", EnumToString(tag));
    callback(NVResult::NV_OKAY);
  }
  
  if (broadcastResultToGame) {
    BroadcastNVStorageOpResult(tag, NVResult::NV_OKAY, NVOperation::NVOP_WRITE);
  }
  
  return WriteEntryToFile(entryTag);
}

bool NVStorageComponent::Erase(NVEntryTag tag,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  return Erase(tag, callback, broadcastResultToGame, 0);
}
  
bool NVStorageComponent::Erase(NVEntryTag tag,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame,
                               u32 eraseSize)
{
  bool validArgs = true;
  
  // Check if it's a legal tag (in case someone did some casting craziness)
  if (!IsValidEntryTag(tag)) {
    PRINT_NAMED_WARNING("NVStorageComponent.Erase.InvalidEntryTag",
                        "Tag: %s (0x%x)", EnumToString(tag), tag);
    validArgs = false;
  }
  
  if (IsFactoryEntryTag(tag) && !_writingFactory) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.FactoryTagNotAllowed",
                        "Tag: %s (0x%x)", EnumToString(tag), tag);
    validArgs = false;
  }
  
  // Fail immediately if invalid args found
  if (!validArgs) {
    if (broadcastResultToGame) {
      BroadcastNVStorageOpResult(tag, NVResult::NV_BAD_ARGS, NVOperation::NVOP_ERASE);
    }
    if (callback) {
      callback(NVResult::NV_BAD_ARGS);
    }
    return false;
  }

  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Erase.ErasingData", "%s", EnumToString(tag));
  u32 entryTag = static_cast<u32>(tag);
  
  _tagDataMap.erase(entryTag);
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Erase.ExecutingCallback", "%s", EnumToString(tag));
    callback(NVResult::NV_OKAY);
  }
  
  if (broadcastResultToGame) {
    BroadcastNVStorageOpResult(tag, NVResult::NV_OKAY, NVOperation::NVOP_ERASE);
  }
  
  return WriteEntryToFile(entryTag);
}
  
  
bool NVStorageComponent::WipeAll(NVStorageWriteEraseCallback callback,
                                 bool broadcastResultToGame)
{
  _tagDataMap.clear();
  
  if (callback) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WipeAll.ExecutingCallback", "");
    callback(NVResult::NV_OKAY);
  }
  
  if (broadcastResultToGame) {
    BroadcastNVStorageOpResult(NVEntryTag::NVEntry_NEXT_SLOT, NVResult::NV_OKAY, NVOperation::NVOP_WIPEALL);
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
                              std::vector<u8>* data,
                              bool broadcastResultToGame)
{
  bool validArgs = true;
  
  // Check for invalid tags
  if (!IsValidEntryTag(tag)) {
    PRINT_NAMED_WARNING("NVStorageComponent.Read.InvalidTag",
                        "Tag: 0x%x", tag);
    validArgs = false;
  }
  
  // Fail immediately if invalid args found
  if (!validArgs) {
    if (broadcastResultToGame) {
      BroadcastNVStorageOpResult(tag, NVResult::NV_BAD_ARGS, NVOperation::NVOP_READ);
    }
    if (callback) {
      callback(nullptr, 0, NVResult::NV_BAD_ARGS);
    }
    return false;
  }

  PRINT_CH_INFO("NVStorage", "NVStorageComponent.Read.ReadingData", "%s", EnumToString(tag));
  u32 entryTag = static_cast<u32>(tag);
  NVResult result = NVResult::NV_OKAY;
  
  const auto dataIter = _tagDataMap.find(entryTag);
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
  
  if (broadcastResultToGame) {
    BroadcastNVStorageReadResults(tag, dataPtr, dataSize);
  }
  
  return true;
}

bool NVStorageComponent::WriteEntryToFile(u32 tag)
{
  // Not writing data to disk on Mac OS so as make sure Webots robots load "clean"
  // which is usually the assumption when using Webots robots.
  // If you bypass this it's your own responsibility to delete webotsCtrlGameEngine2/files/output/nvStorage
  // to get a clean system again.
  #if !defined(ANKI_PLATFORM_OSX)
  
  // Create filename from tag
  std::stringstream ss;
  ss << std::hex << tag << _kNVDataFileExtension;
  
  std::string path = _kStoragePath;
  // TODO(Al): This probably needs to write somewhere else for actual factory nvStorage
  if(IsFactoryEntryTag(static_cast<NVEntryTag>(tag)))
  {
    path = _kFactoryPath;
  }

  // If tag doesn't exist in map, then delete the associated file if it exists
  auto iter = _tagDataMap.find(tag);
  if (iter == _tagDataMap.end()) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Erasing", "Tag 0x%x", tag);
    Util::FileUtils::DeleteFile(path + ss.str());
    return true;
  }

  // Camera calibration is special since it lives in a block device
  if(tag == static_cast<u32>(NVEntryTag::NVEntry_CameraCalib))
  {
    return WriteCameraCalibFile(iter);
  }
  
  // Write data to file
  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Writing", "Tag 0x%x, size %zu", tag, iter->second.size());
  Util::FileUtils::CreateDirectory(path);
  if (!Util::FileUtils::WriteFile(path + ss.str(), iter->second)) {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.Failed", "%s", ss.str().c_str());
    return false;
  }

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
    if (tagNum <= 0) {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.InvalidFileName", "%s", fileName.c_str());
      continue;
    }
    
    if (!IsValidEntryTag(static_cast<NVEntryTag>(tagNum))) {
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
    
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.LoadDataFromFiles.LoadingData", "Tag 0x%x, data size = %zu bytes", tagNum, file.size());
    _tagDataMap[tagNum].assign(file.data(), file.data() + file.size());
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
    if (tagNum <= 0) {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadFactoryDataFromFiles.InvalidFileName", "%s", fileName.c_str());
      continue;
    }
    
    if (!IsFactoryEntryTag(static_cast<NVEntryTag>(tagNum)) ||
        !IsPotentialFactoryEntryTag(tagNum)) {
      PRINT_NAMED_ERROR("NVStorageComponent.LoadFactoryDataFromFiles.InvalidTagValues", "0x%x", tagNum);
      continue;
    }

    // Camera calibration is special and is stored in a block device so we should not be trying
    // to load it from the normal nvstorage files
    if(tagNum == static_cast<u32>(NVEntryTag::NVEntry_CameraCalib))
    {
      DEV_ASSERT_MSG(false,
                     "NVStorageComponent.LoadFactoryDataFromFiles.CameraCalibFile",
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
    _tagDataMap[tagNum].assign(file.data(), file.data() + file.size());
  }
}
  
void NVStorageComponent::Update()
{
}
  
bool NVStorageComponent::HasPendingRequests()
{
  return false;
}
  
#ifdef SIMULATOR
void NVStorageComponent::LoadSimData()
{
  // Store simulated camera calibration data
  const CameraCalibration* camCalib = CameraService::getInstance()->GetHeadCamInfo();
  
  _tagDataMap[static_cast<u32>(NVEntryTag::NVEntry_CameraCalib)].assign(reinterpret_cast<const u8*>(camCalib), reinterpret_cast<const u8*>(camCalib) + sizeof(*camCalib));
}
#endif  // ifdef SIMULATOR
  
void NVStorageComponent::BroadcastNVStorageOpResult(NVEntryTag tag, NVResult res, NVOperation op, u8 index, const u8* data, size_t data_length)
{
  DEV_ASSERT_MSG(data_length <= _kMaxNvStorageBlobSize,
                 "NVStorageComponent.BroadcastNVStorageOpResult.DataLengthTooLarge", "%zu", data_length);
  
  ExternalInterface::NVStorageOpResult msg;
  msg.tag = tag;
  msg.result = res;
  msg.op = op;
  msg.index = index;
  if (nullptr != data) {
    msg.data.assign(data, data + data_length);
  }
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
}

void NVStorageComponent::BroadcastNVStorageReadResults(NVEntryTag tag, u8* dataPtr, size_t dataSize)
{
  if (dataPtr == nullptr || dataSize == 0) {
    BroadcastNVStorageOpResult(tag, NVResult::NV_NOT_FOUND, NVOperation::NVOP_READ);
    return;
  }
  
  // Send data up to game if result was NV_OKAY
  size_t bytesRemaining = dataSize;
  u8 index = 0;
  BOUNDED_WHILE(1000, bytesRemaining > 0) {
    
    size_t data_length = MIN(bytesRemaining, _kMaxNvStorageBlobSize);
    
    BroadcastNVStorageOpResult(tag,
                               bytesRemaining > data_length ? NVResult::NV_MORE : NVResult::NV_OKAY,
                               NVOperation::NVOP_READ,
                               index,
                               dataPtr, data_length);
    
    bytesRemaining -= data_length;
    dataPtr += data_length;
    index++;
  }
}
  
  
  
bool NVStorageComponent::QueueWriteBlob(const NVEntryTag tag,
                                        const u8* data, u16 dataLength,
                                        u8 blobIndex, u8 numTotalBlobs)
{
  // Check for invalid tags
  if (!IsValidEntryTag(tag)) {
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.InvalidTag",
                        "Tag: 0x%x", static_cast<u32>(tag));
    return false;
  }
  
  u32 maxNumAllowedBlobs = GetMaxSizeForEntryTag(tag) / _kMaxNvStorageBlobSize;
  if (numTotalBlobs > maxNumAllowedBlobs) {
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.TooManyBlobs",
                        "Num total blobs %d exceeds max of %d",
                        numTotalBlobs, maxNumAllowedBlobs);
    return false;
  }
  
  if (_pendingWriteData.tag != tag || !_pendingWriteData.pending) {
    if (!_pendingWriteData.pending) {
      PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.QueueWriteBlob.FirstBlobRecvd", "Tag: %s", EnumToString(tag));
    } else {
      PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.UnexpectedTag",
                          "Deleting old data for %s and starting %s",
                          EnumToString(_pendingWriteData.tag),
                          EnumToString(tag));
      ClearPendingWriteEntry();
    }
    
    _pendingWriteData.tag = tag;
    _pendingWriteData.data.resize(numTotalBlobs * _kMaxNvStorageBlobSize);
    _pendingWriteData.data.shrink_to_fit();
    _pendingWriteData.pending = true;

    for (u8 i=0; i<numTotalBlobs; ++i) {
      _pendingWriteData.remainingIndices.insert(i);
    }
  }

  
  // Remove index
  if (_pendingWriteData.remainingIndices.erase(blobIndex) == 0) {
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.UnexpectedIndex",
                        "index %d, numTotalBlobs: %d",
                        blobIndex, numTotalBlobs);
    return false;
  }
  
  
  // Check that blob size is correct given blobIndex
  if (blobIndex < numTotalBlobs-1) {
    if (dataLength != _kMaxNvStorageBlobSize) {
      PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.NotLastBlobIsTooSmall",
                          "Only the last blob is allowed to be smaller than %d bytes. Ignoring message so this write is definitely going to fail! (Tag: %s, index: %d (total %d), size: %d)",
                          _kMaxNvStorageBlobSize,
                          EnumToString(tag),
                          blobIndex,
                          numTotalBlobs,
                          dataLength);
      return false;
    }
  } else if (blobIndex > numTotalBlobs - 1) {
    
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.IndexTooBig",
                        "Tag: %s, index: %d (total %d)",
                        EnumToString(tag), blobIndex, numTotalBlobs);
    return false;
  } else { // (blobIndex == numTotalBlobs - 1)
    // This is the last blob so we can know what the total size of the entry is now
    _pendingWriteData.data.resize((numTotalBlobs-1) * _kMaxNvStorageBlobSize + dataLength);
  }

  
  // Add blob data
  std::copy(data, data + dataLength, _pendingWriteData.data.begin() + (blobIndex * _kMaxNvStorageBlobSize));

  
  
  // Ready to send to robot?
  bool res = true;
  if (_pendingWriteData.remainingIndices.empty()) {
    PRINT_CH_INFO("NVStorage", "NVStorageComponent.QueueWriteBlob.SendingToRobot",
                 "Tag: %s, Bytes: %zu",
                 EnumToString(tag),
                 _pendingWriteData.data.size());
    res = Write(tag, _pendingWriteData.data.data(), _pendingWriteData.data.size(), {}, true);
    
    // Clear pending write data for next message
    ClearPendingWriteEntry();
  }
  
  return res;
}
  
  

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageWriteEntry& msg)
{
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVStorageWriteEntry.Recvd",
                "Tag: %s, size %zu",
                EnumToString(msg.tag), msg.data.size());

  if (msg.numTotalBlobs > 1) {
    if (!QueueWriteBlob(msg.tag,
                        msg.data.data(), msg.data.size(),
                        msg.index, msg.numTotalBlobs)) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVStorageWriteEntry.FailedToQueue",
                          "Tag: %s, size: %zu, blobIndex: %d, numTotalBlobs: %d",
                          EnumToString(msg.tag), msg.data.size(), msg.index, msg.numTotalBlobs);
      BroadcastNVStorageOpResult(msg.tag, NVResult::NV_ERROR, NVOperation::NVOP_WRITE);
    }
  } else {
    Write(msg.tag, msg.data.data(), msg.data.size(), {}, true);
  }
  
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageReadEntry& msg)
{
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVStorageReadEntry.Recvd", "Tag: %s", EnumToString(msg.tag));
  Read(msg.tag, {}, nullptr, true);
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageEraseEntry& msg)
{
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVStorageEraseEntry.Recvd", "Tag: %s", EnumToString(msg.tag));
  Erase(msg.tag, {}, true);
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageWipeAll& msg)
{
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVStorageWipeAll.Recvd", "key: %s", msg.key.c_str());
  WipeAll({}, true);
}
  
template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageClearPartialPendingWriteEntry& msg)
{
  ClearPendingWriteEntry();
}

void NVStorageComponent::ClearPendingWriteEntry() {
  _pendingWriteData.pending = false;
  _pendingWriteData.remainingIndices.clear();
}
  
  
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
  _tagDataMap[static_cast<u32>(NVEntryTag::NVEntry_CameraCalib)].assign(data.data(), data.data() + data.size());

  int rc = close(fd);
  if(rc == -1)
  {
    PRINT_NAMED_ERROR("NVStorageComponent.LoadDataFromFiles.FailedToCloseCameraCalib",
                      "%d", errno);
  }
}

bool NVStorageComponent::WriteCameraCalibFile(const TagDataMap::iterator& iter)
{
  if(iter->first != static_cast<u32>(NVEntryTag::NVEntry_CameraCalib))
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
                      "Only wrote %d bytes instead of %d",
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

  return ret;
}
  
void NVStorageComponent::Test()
{
  // For development only
#if (ANKI_DEVELOPER_CODE)

  NVEntryTag testMultiBlobTag = NVEntryTag::NVEntry_FaceAlbumData;
  

  static u8 nvAction = 0;
  switch(nvAction) {
    case 0: {
      const u32 BUFSIZE = 1100;
      u8 d[BUFSIZE] = {0};
      d[0] = 1;
      d[1023] = 2;
      d[1024] = 3;
      d[1034] = 4;
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.Writing", "tag: %s, size: %u",
                        EnumToString(testMultiBlobTag), BUFSIZE);
      Write(testMultiBlobTag, d, BUFSIZE,
            [](NVResult res) {
            PRINT_NAMED_DEBUG("NVStorageComponent.Test.WriteResult", "%s", EnumToString(res));
            });
      break;
    }
    case 1:
    case 3:
    {
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.Read", "tag: %s", EnumToString(testMultiBlobTag));
      Read(testMultiBlobTag,
           [](u8* data, size_t size, NVResult res) {
             if (res == NVResult::NV_OKAY) {
               PRINT_NAMED_DEBUG("NVStorageComponent.Test.ReadSUCCESS",
                                 "size: %zu, data[0]: %d, data[1023]: %d, data[1024]: %d, data[1034]: %d",
                                 size, data[0], data[1023], data[1024], data[1034]);
             } else {
               PRINT_NAMED_DEBUG("NVStorageComponent.Test.ReadFAIL", "");
             }
           });
      break;
    }
    case 2:
    {
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.Erase", "tag: %s", EnumToString(testMultiBlobTag));
      Erase(testMultiBlobTag,
            [](NVResult res) {
              PRINT_NAMED_DEBUG("NVStorageComponent.Test.EraseResult", "%s", EnumToString(res));
            });
      break;
    }
  }
  if (++nvAction == 4) {
    nvAction = 0;
  }

  
  
  /*
  static bool write = true;
  static const char* inFile = "in.jpg";
  static const char* outFile = "out.jpg";
  if (write) {
    // Open input image and send data to robot
    FILE* fp = fopen(inFile, "rb");
    if (fp) {
      std::vector<u8> d(30000);
      size_t numBytes = fread(d.data(), 1, d.size(), fp);
      d.resize(numBytes);
  
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.WritingTestImage", "size: %zu", numBytes);
      Write(testMultiBlobTag, d.data(), numBytes);
    } else {
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.InputFileOpenFailed", "File: %s", inFile);
    }
  } else {
    PRINT_NAMED_DEBUG("NVStorageComponent.Test.ReadingTestImage", "");
    Read(testMultiBlobTag,
         [](u8* data, size_t size, NVResult res) {
           
           PRINT_NAMED_DEBUG("NVStorageComponent.Test.TestImageRecvd", "size: %zu", size);
   
           // Write image data to file
           FILE* fp = fopen(outFile, "wb");
           if (fp) {
             fwrite(data,1,size,fp);
             fclose(fp);
           } else {
             PRINT_NAMED_DEBUG("NVStorageComponent.Test.OutputFileOpenFailed", "File: %s", outFile);
           }
         });
  }
  write = !write;
  */

#endif // (ANKI_DEVELOPER_CODE)
}
  
  
} // namespace Cozmo
} // namespace Anki
