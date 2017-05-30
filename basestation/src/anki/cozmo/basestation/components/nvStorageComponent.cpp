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

#include "anki/common/robot/errorHandling.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#ifdef COZMO_V2
#ifdef SIMULATOR
#include "anki/cozmo/basestation/androidHAL/androidHAL.h"
#include "clad/types/imageTypes.h"
#endif // ifdef SIMULATOR
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/fileUtils/fileUtils.h"
#endif // ifdef COZMO_V2

CONSOLE_VAR(bool, kNoWriteToRobot, "NVStorageComponent", false);

// For some inexplicable reason this needs to be here in the cpp instead of in the header
// because we are including consoleInterface.h
// Maximum size of a single blob
static constexpr u32 _kMaxNvStorageBlobSize = 1024;

#ifdef COZMO_V2
static const char*   _kNVDataFileExtension = ".nvdata";
#endif

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
                                                          {NVEntryTag::NVEntry_NurtureGameData, 0},
                                                          {NVEntryTag::NVEntry_InventoryData,   0},
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

  
NVStorageComponent::NVStorageComponent(Robot& inRobot, const CozmoContext* context)
  : _robot(inRobot)
#ifdef COZMO_V2
  , _kStoragePath((_robot.GetContextDataPlatform() != nullptr ? _robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Persistent, "nvStorage/") : ""))
#else 
  , _backupManager(inRobot)
#endif

{
  #ifdef COZMO_V2
  {
    #ifdef SIMULATOR
    LoadSimData();
    #endif

    LoadDataFromFiles();
  }
  #else
    SetState(NVSCState::IDLE);
  #endif // ifndef COZMO_V2
  
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

    #ifndef COZMO_V2
    {
      // Setup robot message handlers
      RobotInterface::MessageHandler *messageHandler = context->GetRobotManager()->GetMsgHandler();
      RobotID_t robotId = _robot.GetID();
      
      using localHandlerType = void(NVStorageComponent::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
      // Create a helper lambda for subscribing to a tag with a local handler
      auto doRobotSubscribe = [this, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
      {
        _signalHandles.push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
      };
      
      // bind to specific handlers in the NVStorage class
      doRobotSubscribe(RobotInterface::RobotToEngineTag::nvOpResult,        &NVStorageComponent::HandleNVOpResult);
    }
    #endif
    
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
  
  // Sanity check on size
  u32 allowedSize = GetMaxSizeForEntryTag(tag);

# ifndef COZMO_V2
  // If we aren't writing to the factory block then subtract the size of the header
  if(!_writingFactory)
  {
    allowedSize -= _kEntryHeaderSize;
  }
# endif
  
  if ((size > allowedSize) || size <= 0) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidSize",
                        "Tag: %s, %zu bytes (limit %d bytes)",
                        EnumToString(tag), size, allowedSize);
    validArgs = false;
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
  if(kNoWriteToRobot)
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

  
  #ifdef COZMO_V2
  {
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
    
    WriteEntryToFile(entryTag);
  }
  #else
  {
    if(!_writingFactory)
    {
      PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Write.PrecedingWriteWithErase",
                     "Tag: %s", EnumToString(tag));
      
      _requestQueue.emplace(tag, NVStorageWriteEraseCallback(), false);
    }
    
    // Queue write request
    _requestQueue.emplace(tag, callback, new std::vector<u8>(data,data+size), broadcastResultToGame);

    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Write.DataQueued",
                   "%s - numBytes: %zu",
                   EnumToString(tag),
                   size);
  }
  #endif
  
  return true;
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
  
  
  #ifdef COZMO_V2
  {
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
    
    WriteEntryToFile(entryTag);
  }
  #else
  {
    // If we aren't writing to the robot then call the callback immediately
    if(kNoWriteToRobot)
    {
      if(broadcastResultToGame)
      {
        BroadcastNVStorageOpResult(tag, NVResult::NV_OKAY, NVOperation::NVOP_ERASE);
      }
      if(callback)
      {
        callback(NVResult::NV_OKAY);
      }
      return true;
    }
    
    _requestQueue.emplace(tag, callback, broadcastResultToGame, eraseSize);
    
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Erase.Queued",
                   "%s", EnumToString(tag));
  }
  #endif  // ifdef COZMO_V2
  
  return true;
}
  
  
bool NVStorageComponent::WipeAll(NVStorageWriteEraseCallback callback,
                                 bool broadcastResultToGame)
{
  #ifdef COZMO_V2
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
  }
  #else
  {
    // If we aren't writing to the robot then call the callback immediately
    if(kNoWriteToRobot)
    {
      if(broadcastResultToGame)
      {
        BroadcastNVStorageOpResult(NVEntryTag::NVEntry_NEXT_SLOT, NVResult::NV_OKAY, NVOperation::NVOP_WIPEALL);
      }
      if(callback)
      {
        callback(NVResult::NV_OKAY);
      }
      return true;
    }

    _requestQueue.emplace(callback, broadcastResultToGame);
    
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WipeAll.Queued", "");
  }
#endif // ifdef COZMO_V2
  
  return true;
}

bool NVStorageComponent::WipeFactory(NVStorageWriteEraseCallback callback)
{
  if(_writingFactory)
  {
    return Erase(NVEntryTag::NVEntry_FactoryBaseTag,
                 callback,
                 false,
                 static_cast<u32>(NVConst::NVConst_FACTORY_BLOCK_SIZE));
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
  
  #ifdef COZMO_V2
  {
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
  }
  #else
  {
    PRINT_CH_INFO("NVStorage", "NVStorageComponent.Read.QueueingReadRequest", "%s", EnumToString(tag));
    _requestQueue.emplace(tag, callback, data, broadcastResultToGame);
  }
  #endif  // ifdef COZMO_V2
  
  return true;
}



#pragma mark --- Start of COZMO 2.0 only methods ---
#ifdef COZMO_V2
  
void NVStorageComponent::WriteEntryToFile(u32 tag)
{
  // Not writing data to disk on Mac OS so as make sure Webots robots load "clean"
  // which is usually the assumption when using Webots robots.
  // If you bypass this it's your own responsibility to delete webotsCtrlGameEngine2/files/output/nvStorage
  // to get a clean system again.
  #if !defined(ANKI_PLATFORM_OSX)
  
  // Create filename from tag
  std::stringstream ss;
  ss << std::hex << tag << _kNVDataFileExtension;

  // If tag doesn't exist in map, then delete the associated file if it exists
  auto iter = _tagDataMap.find(tag);
  if (iter == _tagDataMap.end()) {
    PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Erasing", "Tag 0x%x", tag);
    Util::FileUtils::DeleteFile(_kStoragePath + ss.str());
    return;
  }
  
  // Write data to file
  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.WriteEntryToFile.Writing", "Tag 0x%x, side %zu", tag, iter->second.size());
  Util::FileUtils::CreateDirectory(_kStoragePath);
  if (!Util::FileUtils::WriteFile(_kStoragePath + ss.str(), iter->second)) {
    PRINT_NAMED_ERROR("NVStorageComponent.WriteEntryToFile.Failed", "%s", ss.str().c_str());
  }

  #endif // if !defined(ANKI_PLATFORM_OSX)
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
  const CameraCalibration* camCalib = AndroidHAL::getInstance()->GetHeadCamInfo();
  
  _tagDataMap[static_cast<u32>(NVEntryTag::NVEntry_CameraCalib)].assign(reinterpret_cast<const u8*>(camCalib), reinterpret_cast<const u8*>(camCalib) + sizeof(*camCalib));
}
#endif  // ifdef SIMULATOR
  
#pragma mark --- End of COZMO 2.0 only methods ---
#else  // #ifdef COZMO_V2
#pragma mark --- Start of COZMO 1.x only methods ---
  
void NVStorageComponent::ProcessRequest()
{
  if (_requestQueue.empty()) {
    return;
  }
  NVStorageRequest& req = _requestQueue.front();
  
  u32 entryTag = static_cast<u32>(req.tag);
  
  switch(req.op) {
    case NVOperation::NVOP_WRITE:
    {
      _writeDataObject.ClearData();
      
      // Copy data locally and break up into as many messages as needed
      _writeDataObject.baseTag   = req.tag;
      _writeDataObject.nextTag   = entryTag;
      _writeDataObject.data      = req.data;
      _writeDataObject.sendIndex = 0;      
      _writeDataObject.sending   = true;
      
      DEV_ASSERT_MSG(!_writeDataAckInfo.pending, "NVStorageComponent.ProcessRequest.UnexpectedWritePending1",
                     "0x%x", _writeDataAckInfo.tag);
      
      // Set associated ack info
      _writeDataAckInfo.tag                   = req.tag;
      _writeDataAckInfo.timeoutTimeStamp      = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckInfo.broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckInfo.callback              = req.writeCallback;
      _writeDataAckInfo.pending               = false;

      
      // Let the backup manager know this tag and data have been queued to be written
      _backupManager.QueueDataToWrite(req.tag, *req.data);

      PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.ProcessRequest.SendingWrite",
                     "StartTag: 0x%x (%s), timeoutTime: %d, currTime: %d",
                     entryTag, EnumToString(req.tag), _writeDataAckInfo.timeoutTimeStamp, _robot.GetLastMsgTimestamp());
      
      SetState(NVSCState::SENDING_WRITE_DATA);
      
      break;
    }
    case NVOperation::NVOP_ERASE:
    {
      DEV_ASSERT_MSG(!_writeDataAckInfo.pending, "NVStorageComponent.ProcessRequest.UnexpectedWritePending2",
                     "0x%x", _writeDataAckInfo.tag);

      _writeDataObject.sending = false;
      
      // Set associated ack info
      _writeDataAckInfo.tag                   = req.tag;
      _writeDataAckInfo.timeoutTimeStamp      = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckInfo.broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckInfo.callback              = req.writeCallback;
      _writeDataAckInfo.pending               = true;
      
      // Start constructing erase message
      _lastCommandSent.operation = NVOperation::NVOP_ERASE;
      _lastCommandSent.address = entryTag;
      // If an eraseSize was not specified get the size from the _maxSize table
      _lastCommandSent.length = req.eraseSize > 0 ? req.eraseSize : GetMaxSizeForEntryTag(req.tag);
      PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.ProcessRequest.SendingErase", "%s (Tag: 0x%x) size: %u", EnumToString(req.tag), entryTag, _lastCommandSent.length );
      _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));
      
      // Let the backup manager know this tag and data have been queued to be written
      _backupManager.QueueDataToWrite(req.tag, std::vector<u8>());
      
      SetState(NVSCState::SENDING_WRITE_DATA);
      
      break;
    }
    case NVOperation::NVOP_WIPEALL:
    {
      PRINT_CH_DEBUG("NVStorage", "NVStoageComponent.ProcessRequest.SendingWipeAll", "");
      
      _writeDataObject.sending = false;
      
      _writeDataAckInfo.tag                   = NVEntryTag::NVEntry_NEXT_SLOT;
      _writeDataAckInfo.timeoutTimeStamp      = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckInfo.broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckInfo.callback              = req.writeCallback;
      _writeDataAckInfo.pending               = true;
      
      _lastCommandSent.address                = 0;
      _lastCommandSent.operation              = NVOperation::NVOP_WIPEALL;
      
      _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));
      
      SetState(NVSCState::SENDING_WRITE_DATA);
      
      break;
    }
    case NVOperation::NVOP_READ:
    {
      
      DEV_ASSERT_MSG(!_recvDataInfo.pending, "NVStorageComponent.ProcessRequest.UnexpectedReadPending",
                     "0x%x", _recvDataInfo.tag);
      
      // If factory tag, request entire range
      _lastCommandSent.address   = entryTag;
      _lastCommandSent.operation = NVOperation::NVOP_READ;
      _lastCommandSent.length    = IsFactoryEntryTag(req.tag) ? _maxFactoryEntrySizeTable[req.tag] : _kMaxNvStorageBlobSize;
      _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));

      PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.ProcessRequest.SendingRead",
                    "StartTag: 0x%x, Length: %u",
                    _lastCommandSent.address,
                    _lastCommandSent.length);

      
      // Create RecvDataObject
      _recvDataInfo.ClearData();
      _recvDataInfo.tag                   = req.tag;
      _recvDataInfo.pending               = true;
      _recvDataInfo.dataSizeKnown         = false;
      _recvDataInfo.callback              = req.readCallback;
      _recvDataInfo.broadcastResultToGame = req.broadcastResultToGame;
      _recvDataInfo.timeoutTimeStamp      = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      if (nullptr == req.data) {
        _recvDataInfo.data                 = new std::vector<u8>();
        _recvDataInfo.deleteVectorWhenDone = true;
      } else {
        _recvDataInfo.data                 = req.data;
        _recvDataInfo.data->clear();
        _recvDataInfo.deleteVectorWhenDone = false;
      }

      SetState(NVSCState::RECEIVING_DATA);
      
      break;
    }
    default:
      PRINT_NAMED_WARNING("NVStorageComponent.ProcessRequest.UnsupportOperation", "%s", EnumToString(req.op));
      break;
  }
  
  _requestQueue.pop();
  
  // Retry send attempt counter
  _numSendAttempts = 0;
  
}

void NVStorageComponent::Update()
{
  ANKI_CPU_PROFILE("NVStorageComponent::Update");
  
  switch(_state) {
    case NVSCState::IDLE:
    {
      // Send requests if there are any in the queue
      ProcessRequest();
      break;
    }
    case NVSCState::SENDING_WRITE_DATA:
    {
      // If expecting write/erase ack, check timeout.
      if (_writeDataAckInfo.pending) {
        if (_robot.GetLastMsgTimestamp() > _writeDataAckInfo.timeoutTimeStamp) {
          PRINT_NAMED_WARNING("NVStorageComponent.Update.WriteTimeout",
                              "Tag: 0x%x", _writeDataAckInfo.tag);
          
          if(_writeDataAckInfo.callback) {
            _writeDataAckInfo.callback(NVResult::NV_TIMEOUT);
          }
          SetState(NVSCState::IDLE);
        }
      }
      
      // If there are writes to send, send them.
      // Send up to one blob per Update() call.
      else if (_writeDataObject.sending) {
        WriteDataObject* sendData = &_writeDataObject;
        
        // Fill out write message
        _lastCommandSent.operation = NVOperation::NVOP_WRITE;
        _lastCommandSent.address = sendData->nextTag;
        
        
        // Send the next blob of data to send for this multi-blob message
        u32 bytesLeftToSend = static_cast<u32>(sendData->data->size()) - sendData->sendIndex;
        u32 bytesToSend = MIN(bytesLeftToSend, _kMaxNvStorageBlobSize);
        DEV_ASSERT(bytesToSend > 0, "NVStorageComponent.Update.ExpectedPositiveNumBytesToSend");
        
        // If this is the first blob, prepend header
        _lastCommandSent.blob.clear();
        if (sendData->sendIndex == 0 && !_writingFactory) {

          NVStorageHeader header;
          header.dataSize = static_cast<u32>(sendData->data->size());
          
          auto const headerPtr = reinterpret_cast<u8*>(&header);
          _lastCommandSent.blob.insert(_lastCommandSent.blob.end(), headerPtr , headerPtr + _kEntryHeaderSize);
          
          bytesToSend = MIN(_kMaxNvStorageBlobSize - _kEntryHeaderSize, bytesToSend);
        }

        _lastCommandSent.blob.insert(_lastCommandSent.blob.end(),
                                     sendData->data->begin() + sendData->sendIndex,
                                     sendData->data->begin() + sendData->sendIndex + bytesToSend);

        
        sendData->sendIndex += bytesToSend;

        if (IsFactoryEntryTag(sendData->baseTag)) {
          ++sendData->nextTag;
        } else {
          sendData->nextTag += _kMaxNvStorageBlobSize;
        }

        PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.Update.SendingWriteMsg",
                       "BaseTag: %s, tag: 0x%x, bytesSent: %d",
                       EnumToString(sendData->baseTag),
                       _lastCommandSent.address,
                       bytesToSend);

        // Send data
        _lastCommandSent.length = static_cast<u32>(_lastCommandSent.blob.size());
        _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));
        _writeDataAckInfo.pending = true;
        _writeDataAckInfo.timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
        _numSendAttempts = 0;

        // Sent last blob?
        if (sendData->sendIndex >= sendData->data->size()) {
          sendData->sending = false;
        }
        
      } else {
        PRINT_NAMED_WARNING("NVStorageComponent.Update.NoDataToWrite", "");
        SetState(NVSCState::IDLE);
      }
      break;
    }
      
    case NVSCState::RECEIVING_DATA:
    {
      // If expecting read data to arrive, check timeout.
      if (_recvDataInfo.pending) {
        
        if (_robot.GetLastMsgTimestamp() > _recvDataInfo.timeoutTimeStamp) {
          PRINT_NAMED_WARNING("NVStorageComponent.Update.ReadTimeout",
                              "Tag: 0x%x", _recvDataInfo.tag);
          
          if(_recvDataInfo.callback) {
            _recvDataInfo.callback(nullptr, 0, NVResult::NV_TIMEOUT);
          }
          SetState(NVSCState::IDLE);
        }
      } else {
        PRINT_NAMED_WARNING("NVStorageComponent.Update.NoReadPending", "");
        SetState(NVSCState::IDLE);
      }
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("NVStorageComponent.Update.InvalidState", "%d", _state);
      SetState(NVSCState::IDLE);
      break;
  }
  
  
}
  
void NVStorageComponent::SetState(NVSCState s)
{
  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.SetState", "PrevState: %d, NewState: %d", _state, s);
  if (s == NVSCState::IDLE) {
    _writeDataObject.sending  = false;
    _writeDataAckInfo.pending = false;
    _recvDataInfo.pending     = false;
  }
  _state = s;
}

bool NVStorageComponent::HasPendingRequests()
{
  return !_requestQueue.empty() || _recvDataInfo.pending || _writeDataAckInfo.pending;
}
  
bool NVStorageComponent::ResendLastCommand()
{
  if (++_numSendAttempts >= _kMaxNumSendAttempts) {
    // Something must be wrong with the robot to have kMaxNumSendAttempts attempts fail so print an error
    PRINT_NAMED_ERROR("NVStorageComponent.ResendLastCommand.NumRetriesExceeded",
                      "Tag: 0x%x, Op: %s, Attempts: %d",
                      _lastCommandSent.address,
                      EnumToString(_lastCommandSent.operation),
                      _kMaxNumSendAttempts );
    return false;
  }
  
  PRINT_CH_INFO("NVStorage", "NVStorageComponent.ResendLastCommand.Retry",
                "Tag: 0x%x, Op: %s, Attempt: %d",
                _lastCommandSent.address, EnumToString(_lastCommandSent.operation), _numSendAttempts);
  _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));
  return true;
}
  

void NVStorageComponent::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ANKI_CPU_PROFILE("Robot::HandleNVOpResult");
  
  NVOpResult payload = message.GetData().Get_nvOpResult();

  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.Recvd",
                 "Tag: 0x%x, Op: %s, Result: %s",
                 payload.address,
                 EnumToString(payload.operation),
                 EnumToString(payload.result));

  
  u32 tag = payload.address;
  NVEntryTag baseTag = NVEntryTag::NVEntry_NEXT_SLOT;

  // Setting a fake tag to allow the rest of the NVOP_WRITE logic work to work for NVOP_WIPEALL
  if (payload.operation == NVOperation::NVOP_WIPEALL) {
    tag = static_cast<u32>(NVEntryTag::NVEntry_NEXT_SLOT);
  } else {
    baseTag = GetBaseEntryTag(tag);
  }

  const char* baseTagStr = EnumToString(static_cast<NVEntryTag>(baseTag));
  
  switch(payload.operation) {
    case NVOperation::NVOP_WRITE:
    case NVOperation::NVOP_ERASE:
    case NVOperation::NVOP_WIPEALL:
    {
      
      // Check that this was actually written data
      if (!_writeDataAckInfo.pending || _writeDataAckInfo.tag != baseTag) {
        PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagBaseTagWasNeverSent",
                            "BaseTag: 0x%x, Tag: 0x%x", baseTag, tag);
        return;
      }
     
      // Possibly increment the failed write count based on the result
      // Negative results == bad
      if (static_cast<s8>(payload.result) < 0) {
        
        // Under these cases, attempt to resend the failed write message.
        // If the allowed number of retries fails
        if (payload.result == NVResult::NV_NO_MEM ||
            payload.result == NVResult::NV_BUSY   ||
            payload.result == NVResult::NV_TIMEOUT||
            payload.result == NVResult::NV_LOOP) {
          if (ResendLastCommand()) {
            PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVOpResult.ResentFailedWrite",
                             "Tag 0x%x resent due to %s, op: %s", tag, EnumToString(payload.result), EnumToString(payload.operation));
            return;
          }
        }

        PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteOpFailed",
                            "Tag: 0x%x, op: %s, result: %s",
                            tag, EnumToString(payload.operation), EnumToString(payload.result));

        // Abort entire write / erase operation
        _writeDataObject.sending = false;
        
        // TODO: Erase the stuff that did write successfully?
        // ...
        
      }

      // ACK the write
      _writeDataAckInfo.pending = false;
        
      
      // Check if all writes have been sent and thus ACK'd for this tag
      if (!_writeDataObject.sending) {
        
        // Print result
        if (payload.result == NVResult::NV_OKAY) {
          PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVOpResult.WriteSuccess",
                           "BaseTag: %s, lastTag: 0x%x, op: %s, result: %s",
                           baseTagStr, tag, EnumToString(payload.operation), EnumToString(payload.result));
        } else {
          PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteFailed",
                              "BaseTag: %s, lastTag: 0x%x, op: %s, result: %s",
                              baseTagStr, tag, EnumToString(payload.operation), EnumToString(payload.result));
        }
        
        // Check if this should be forwarded on to game.
        if (_writeDataAckInfo.broadcastResultToGame) {
          BroadcastNVStorageOpResult(baseTag,
                                     payload.result,
                                     payload.operation);
        }

        
        // Execute write complete callback
        if (_writeDataAckInfo.callback) {
          PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.ExecutingWriteCallback", "%s", baseTagStr);
          _writeDataAckInfo.callback(payload.result);
        }
        
        // Let the backup manager know that the write for this tag has completed
        if (payload.operation == NVOperation::NVOP_WIPEALL) {
          _backupManager.WipeAll();
        } else {
          _backupManager.WriteDataForTag(baseTag,
                                         payload.result,
                                         payload.operation == NVOperation::NVOP_WRITE);
        }
        
        SetState(NVSCState::IDLE);
      }
      break;
    }
    case NVOperation::NVOP_READ:
    {
      u32 blobSize = static_cast<u32>(payload.blob.size());
      bool shouldHaveHeader = !IsFactoryEntryTag(baseTag) && (payload.offset == 0);
      
      // Check that this was actually requested data
      if (!_recvDataInfo.pending || _recvDataInfo.tag != baseTag) {
        PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagNeverRequested",
                            "Tag recvd: 0x%x, BaseTag: 0x%x, ExpectedBaseTag: 0x%x (pending %d), BlobSize: %u, result: %s",
                            tag, baseTag, _recvDataInfo.tag, _recvDataInfo.pending, blobSize, EnumToString(payload.result));
        return;
      }

      // If read was not successful, possibly retry.
      if (static_cast<s8>(payload.result) < 0) {
        
        // Under these cases, attempt to resend the failed read message
        if (payload.result == NVResult::NV_NO_MEM ||
            payload.result == NVResult::NV_BUSY   ||
            payload.result == NVResult::NV_TIMEOUT||
            payload.result == NVResult::NV_LOOP) {
          if (ResendLastCommand()) {
            PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVOpResult.ResentFailedRead",
                          "Tag 0x%x resent due to %s", tag, EnumToString(payload.result));
            return;
          }
        }

        PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.ReadOpFailed",
                            "Tag: 0x%x, op: %s, result: %s",
                            tag, EnumToString(payload.operation), EnumToString(payload.result));
      } else {
        
        // Get header if this is the first blob of the read
        if (shouldHaveHeader && !_recvDataInfo.dataSizeKnown) {
          
          // This is the first 1K. Get the header.
          if (blobSize >= _kEntryHeaderSize) {
            
            // Copy header
            NVStorageHeader header;
            memcpy(&header, payload.blob.data(), _kEntryHeaderSize);
            
            
            // Check header validity
            u32 maxAllowedSize = GetMaxSizeForEntryTag(baseTag);
            if (header.magic != _kHeaderMagic) {
              PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.InvalidHeader",
                             "Tag: 0x%x, Got 0x%x, Expected 0x%x", tag, header.magic, _kHeaderMagic);
              payload.result = NVResult::NV_NOT_FOUND;
              
            } else if (header.dataSize > maxAllowedSize - _kEntryHeaderSize) {
              PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.InvalidDataSize",
                                  "Tag 0x%x, size %u, maxSizeAllowed %u", tag, header.dataSize, maxAllowedSize);
              payload.result = NVResult::NV_NOT_FOUND;
              
            } else {
              
              _recvDataInfo.dataSizeKnown = true;
              
              if (_writeDataAckInfo.pending) {
                
                PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.UnhandledReadForWriteErase", "");
                // TODO: This was the retrieval of data prior to a write just to retrieve the header so that we can know how many bytes to erase.
                // ...
                
                
              } else {
                
                // Request the appropriate amount of data.
                // If we already have the appropriate amount of data, then fall through.
                if (header.dataSize <= blobSize - _kEntryHeaderSize) {
                  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.ResizeBlobToValidData", "Tag: 0x%x, TotalSize: %u", tag, header.dataSize);
                  payload.blob.resize(_kEntryHeaderSize + header.dataSize);
                  blobSize = static_cast<u32>(payload.blob.size());
                } else {
                  PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.ReadingRestOfData", "Tag: 0x%x, TotalSize: %u", tag, header.dataSize);
                  _lastCommandSent.address   = tag;
                  _lastCommandSent.length    = header.dataSize + _kEntryHeaderSize;
                  _lastCommandSent.operation = NVOperation::NVOP_READ;
                  _robot.SendMessage(RobotInterface::EngineToRobot(NVCommand(_lastCommandSent)));
                  return;
                }
              }
            }
            
          } else {
            // The only other time you should get a result with the baseTag is if it is the last OpResult of the read which should have blob length of 0.
            // Otherwise something is wrong!
            PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.TooLittleReadData", "Tag 0x%x, Got %u, Expected %u", tag, blobSize, _kMaxNvStorageBlobSize);
            
            // Clear what incomplete data might be in received data buffer and change result to error
            _recvDataInfo.data->clear();
            payload.result = NVResult::NV_ERROR;
          }
        }

      }
      
      
      // Process the blob data. As long as size > 0, it must contain data.
      if (blobSize > 0 && payload.result >= NVResult::NV_OKAY) {
        u32 payloadOffset = shouldHaveHeader ? _kEntryHeaderSize : 0;
        u32 startWriteIndex = (payload.offset * _kMaxNvStorageBlobSize) - (((payload.offset > 0) && !IsFactoryEntryTag(baseTag)) ? _kEntryHeaderSize : 0);
        u32 newTotalEntrySize = startWriteIndex + blobSize - payloadOffset;
        
        // Make sure receive vector is large enough to hold data
        std::vector<u8>* vec = _recvDataInfo.data;
        if (vec->size() < newTotalEntrySize) {
          vec->resize(newTotalEntrySize);
        }
        

        // Store data at appropriate location in receive vector
        std::copy(payload.blob.begin() + payloadOffset, payload.blob.end(), vec->begin() + startWriteIndex);
        
        // Bump timeout
        _recvDataInfo.timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
        
        PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandelNVData.ReceivedBlob",
                       "BaseEntryTag: 0x%x, Tag: 0x%x, BlobSize %d, offset %d, StartWriteIndex: %d",
                       baseTag, payload.address, blobSize, payload.offset, startWriteIndex);
      }
      
      
      // Operation has failed or succeeded
      if (payload.result != NVResult::NV_MORE) {
        
        if (payload.result == NVResult::NV_NOT_FOUND) {
          PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVOpResult.ReadEntryNotFound",
                        "BaseTag: %s, Tag: 0x%x, result: %s",
                        baseTagStr, tag, EnumToString(payload.result));
        }
        else if (payload.result == NVResult::NV_OKAY) {
          PRINT_CH_INFO("NVStorage", "NVStorageComponent.HandleNVOpResult.ReadSuccess",
                        "BaseTag: %s, result: %s",
                        baseTagStr, EnumToString(payload.result));
        } else {
          PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.ReadFailed",
                              "BaseTag: %s, result: %s",
                              baseTagStr, EnumToString(payload.result));
        }
        
        if (_recvDataInfo.callback ) {
          PRINT_CH_DEBUG("NVStorage", "NVStorageComponent.HandleNVOpResult.ExecutingReadCallback", "%s", baseTagStr);
          _recvDataInfo.callback(_recvDataInfo.data->data(),
                                 _recvDataInfo.data->size(),
                                 payload.result);
        }
        
        // Check if this should be forwarded on to game.
        if (_recvDataInfo.broadcastResultToGame) {
          
          // Send data up to game if result was NV_OKAY
          if (payload.result >= NVResult::NV_OKAY) {
            u32 bytesRemaining = static_cast<u32>(_recvDataInfo.data->size());
            u8* dataPtr = _recvDataInfo.data->data();
            u8 index = 0;
            BOUNDED_WHILE(1000, bytesRemaining > 0) {
              
              u32 data_length = MIN(bytesRemaining, _kMaxNvStorageBlobSize);
              
              BroadcastNVStorageOpResult(static_cast<NVEntryTag>(baseTag),
                                         bytesRemaining > data_length ? NVResult::NV_MORE : payload.result,
                                         NVOperation::NVOP_READ,
                                         index,
                                         dataPtr, data_length);
                                         
              bytesRemaining -= data_length;
              dataPtr += data_length;
              index++;
            }
          } else {
            BroadcastNVStorageOpResult(static_cast<NVEntryTag>(baseTag),
                                       payload.result,
                                       NVOperation::NVOP_READ);
          }
        }
        
        SetState(NVSCState::IDLE);
        
      }
      
      break;
    }
    default:
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.UnhandledOperation", "%s", EnumToString(payload.operation));
      break;
  }
}
#endif // #ifdef COZMO_V2
#pragma mark --- End of COZMO 1.x only methods ---
  
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
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
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
