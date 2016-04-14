/**
 * File: NVStorageComponent.cpp
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Interface for storing and retrieving data from robot flash
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/logging/logging.h"

#define DEBUG_NVSTORAGE_COMPONENT 0

namespace Anki {
namespace Cozmo {
  
NVStorageComponent::NVStorageComponent(Robot& inRobot, const CozmoContext* context)
  : _robot(inRobot)
{

  if (context) {
    // Setup game message handlers
    IExternalInterface *extInterface = context->GetExternalInterface();
    auto writeCallback = std::bind(&NVStorageComponent::HandleNVStorageWriteEntry, this, std::placeholders::_1);
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::NVStorageWriteEntry, writeCallback));
    auto readCallback = std::bind(&NVStorageComponent::HandleNVStorageReadEntry, this, std::placeholders::_1);
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::NVStorageReadEntry, readCallback));
    auto eraseCallback = std::bind(&NVStorageComponent::HandleNVStorageEraseEntry, this, std::placeholders::_1);
    _signalHandles.push_back(extInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::NVStorageEraseEntry, eraseCallback));

    
    // Setup robot message handlers
    RobotInterface::MessageHandler *messageHandler = context->GetRobotMsgHandler();
    RobotID_t robotId = _robot.GetID();
    
    using localHandlerType = void(NVStorageComponent::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
    // Create a helper lambda for subscribing to a tag with a local handler
    auto doRobotSubscribe = [this, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
    {
      _signalHandles.push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
    };
    
    // bind to specific handlers in the NVStorage class
    doRobotSubscribe(RobotInterface::RobotToEngineTag::nvData,          &NVStorageComponent::HandleNVData);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::nvResult,        &NVStorageComponent::HandleNVOpResult);
    
  } else {
    PRINT_NAMED_WARNING("NVStorageComponent.nullContext", "");
  }

}

bool NVStorageComponent::IsMultiBlobEntryTag(u32 tag) const {
  return (tag & 0x7fff0000) > 0;
}


u32 NVStorageComponent::GetBaseEntryTag(u32 tag) const {
  u32 baseTag = tag & 0xffff0000;
  if (baseTag == 0) {
    // For single blob message, the base tag is the same as tag
    baseTag = tag;
  }
  return baseTag;
}
  
u32 NVStorageComponent::GetTagRangeEnd(u32 startTag) const {
  if (IsMultiBlobEntryTag(startTag)) {
    return startTag | 0xffff;
  } else {
    return startTag;
  }
}

bool NVStorageComponent::Write(NVStorage::NVEntryTag tag, u8* data, size_t size, bool broadcastResultToGame)
{
  // Sanity check on size
  if (size > _kMaxNvStorageEntrySize) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.SizeTooBig",
                        "%zu bytes (limit %d bytes)", size, _kMaxNvStorageEntrySize);
    return false;
  }
  
  // Copy data locally and break up into as many messages as needed
  _dataToSendQueue.emplace(tag, std::vector<u8>(data,data+size), true);

  return true;
}
  
void NVStorageComponent::Erase(NVStorage::NVEntryTag tag, bool broadcastResultToGame)
{
  _dataToSendQueue.emplace(tag, std::vector<u8>(), false);
}
  
bool NVStorageComponent::Read(NVStorage::NVEntryTag tag,
                              NVStorageReadCallback callback,
                              std::vector<u8>* data,
                              bool broadcastResultToGame)
{
  // Check if tag already requested
  u32 entryTag = (u32)tag;
  if (_recvDataMap.find(entryTag) != _recvDataMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.Read.TagAlreadyRequested", "Ignoring tag 0x%x", (u32)tag);
    return false;
  }
  
  // Request message from tag to to endTag
  NVStorage::NVStorageRead readMsg;
  readMsg.tag = entryTag;
  readMsg.tagRangeEnd = GetTagRangeEnd(entryTag);
  readMsg.to = NVStorage::NVReportDest::ENGINE;
  _robot.SendMessage(RobotInterface::EngineToRobot(std::move(readMsg)));
  
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.Read.SendingRead",
                      "StartTag: 0x%x, EndTag: 0x%x, reportTo: %s",
                      readMsg.tag,
                      readMsg.tagRangeEnd,
                      EnumToString(readMsg.to));
  }
  
  // Create RecvDataObject
  if (nullptr == data) {
    _recvDataMap[entryTag].data = new std::vector<u8>();
    _recvDataMap[entryTag].deleteVectorWhenDone = true;
  } else {
    _recvDataMap[entryTag].data = data;
    _recvDataMap[entryTag].deleteVectorWhenDone = false;
  }
  _recvDataMap[entryTag].callback = callback;
  _recvDataMap[entryTag].broadcastResultToGame = broadcastResultToGame;
  
  return true;
}
  
  
void NVStorageComponent::Update()
{
  // If there are things to send, send them.
  // Send up to one blob per Update() call.
  if (!_dataToSendQueue.empty()) {
    WriteDataObject* sendData = &_dataToSendQueue.front();

    // Start constructing write message
    NVStorage::NVStorageWrite writeMsg;
    writeMsg.reportTo = NVStorage::NVReportDest::ENGINE;
    writeMsg.writeNotErase = sendData->writeNotErase;
    writeMsg.reportEach = false;
    writeMsg.reportDone = true;
    writeMsg.rangeEnd = GetTagRangeEnd((u32)sendData->baseTag);
    writeMsg.entry.tag = sendData->nextTag;
    
    
    if (sendData->writeNotErase) {
      // Send the next blob of data to send for this multi-blob message
      u32 bytesLeftToSend = (u32)sendData->data.size() - sendData->sendIndex;
      u32 bytesToSend = MIN(bytesLeftToSend, _kMaxNvStorageBlobSize);
      AnkiAssert(bytesToSend > 0);
      
      writeMsg.entry.blob = std::vector<u8>(sendData->data.begin() + sendData->sendIndex,
                                            sendData->data.begin() + sendData->sendIndex + bytesToSend);

      
      // If this is the first blob of a multi-blob write set the number of confirmed writes that are expected
      if (sendData->sendIndex == 0) {
        _numWriteTagsToConfirm[(u32)sendData->baseTag] = static_cast<u32>(ceilf(static_cast<f32>(bytesLeftToSend) / _kMaxNvStorageBlobSize));
        
        if (DEBUG_NVSTORAGE_COMPONENT) {
          PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingWriteMsg.SetExpectedConfirmations",
                            "%s - numBytes: %zu, numExpectedConfirmations: %d",
                            EnumToString(sendData->baseTag),
                            sendData->data.size(),
                            _numWriteTagsToConfirm[(u32)sendData->baseTag]);
        }
      }
      
      ++sendData->nextTag;
      sendData->sendIndex += bytesToSend;
      
      if (DEBUG_NVSTORAGE_COMPONENT) {
        PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingWriteMsg",
                          "BaseTag: %s, tag: 0x%x, write: %d, bytesSent: %d",
                          EnumToString(sendData->baseTag),
                          writeMsg.entry.tag,
                          writeMsg.writeNotErase,
                          bytesToSend);
      }
      
      // Sent last blob?
      if (bytesToSend < _kMaxNvStorageBlobSize) {
        _dataToSendQueue.pop();
      }

      
    } else {

      if (DEBUG_NVSTORAGE_COMPONENT) {
        PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingEraseMsg",
                          "tag: %s", EnumToString(sendData->baseTag));
      }
      
      _numWriteTagsToConfirm[(u32)sendData->baseTag] = 1;
      
      // This is an erase message so we're definitely done with it
      _dataToSendQueue.pop();
    }
    
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(writeMsg)));

  }
  
}

void NVStorageComponent::HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  RobotInterface::NVReadResultToEngine nvReadResult = message.GetData().Get_nvData();
  NVStorage::NVStorageBlob& nvBlob = nvReadResult.blob;

  
  // Check that this was actually requested data
  u32 baseTag = GetBaseEntryTag(nvBlob.tag);
  if (_recvDataMap.find(baseTag) == _recvDataMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVData.TagNotRequested", "tag 0x%x", nvBlob.tag);
    return;
  }

  u32 blobSize = (u32)nvBlob.blob.size();
  
  if (IsMultiBlobEntryTag(nvBlob.tag)) {
    // Make sure receive vector is large enough to hold data
    u32 blobIndex = nvBlob.tag - baseTag;
    u32 startWriteIndex = blobIndex *_kMaxNvStorageBlobSize;
    u32 newTotalEntrySize = startWriteIndex + blobSize;

    std::vector<u8>* vec = _recvDataMap[baseTag].data;
    if (vec->size() < newTotalEntrySize) {
      vec->resize(newTotalEntrySize);
    }
    
    // Store data at appropriate in receive vector
    std::copy(nvBlob.blob.begin(), nvBlob.blob.end(), vec->begin() + startWriteIndex);
    
    if (DEBUG_NVSTORAGE_COMPONENT) {
      PRINT_NAMED_DEBUG("NVStorageComponent.HandelNVData.ReceivedBlob",
                        "BaseEntryTag: 0x%x, Tag: 0x%x, BlobSize %d, StartWriteIndex: %d",
                        baseTag, nvBlob.tag, blobSize, startWriteIndex);
    }
    
  } else {
    
    // If this is not a multiblob message, execute callback now.
    PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.MsgRecvd",
                     "BaseTag: %s, Tag: 0x%x, size: %u",
                     EnumToString((NVStorage::NVEntryTag)baseTag), baseTag, blobSize);
    if (_recvDataMap[baseTag].callback) {
      PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.ExecutingCallback",
                       "BaseTag: %s, Tag: 0x%x",
                       EnumToString((NVStorage::NVEntryTag)baseTag), baseTag);
      _recvDataMap[baseTag].callback(nvBlob.blob.data(), nvBlob.blob.size());
    }
    _recvDataMap.erase(baseTag);
  }
}

void NVStorageComponent::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  RobotInterface::NVOpResultToEngine payload = message.GetData().Get_nvResult();
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVOpResult.Recvd",
                      "RobotAddr: %u, Tag: 0x%x, Result: %s, write: %d",
                      payload.robotAddress,
                      payload.report.tag,
                      EnumToString(payload.report.result),
                      payload.report.write);
  }
  
  u32 tag = payload.report.tag;
  if (tag == (u32)NVStorage::NVEntryTag::NVEntry_Invalid) {
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.InvalidTag", "");
    return;
  }
  
  
  u32 baseTag = GetBaseEntryTag(tag);
  if (payload.report.write) {
    // Check that this was actually written data
    if (_numWriteTagsToConfirm.find(baseTag) == _numWriteTagsToConfirm.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagBaseTagWasNeverSent",
                          "BaseTag: 0x%x, tag 0x%x", baseTag, tag);
      return;
    }
    
    if (_numWriteTagsToConfirm[baseTag] == 0) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.RecvdMoreThanExpectedWriteConfirmations",
                          "BaseTag: 0x%x, tag 0x%x", baseTag, tag);
      return;
    }
    
    --_numWriteTagsToConfirm[baseTag];
    
    if (payload.report.result != NVStorage::NVResult::NV_OKAY) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteOpFailed",
                          "Tag: 0x%x, write: %d, result: %s",
                          tag, payload.report.write, EnumToString(payload.report.result));
      return;
    }

    if (_numWriteTagsToConfirm[baseTag] == 0) {
      PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.MsgWriteConfirmed",
                       "BaseTag: %s, lastTag: 0x%x",
                       EnumToString((NVStorage::NVEntryTag)baseTag), tag);
    }

    
  } else {
    // Check that this was actually requested data
    if (_recvDataMap.find(baseTag) == _recvDataMap.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagNeverRequested", "tag 0x%x", tag);
      return;
    }
    
    if (payload.report.result != NVStorage::NVResult::NV_OKAY) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.ReadOpFailed",
                          "Tag: 0x%x, write: %d, result: %s",
                          tag, payload.report.write, EnumToString(payload.report.result));
      return;
    }

    
    // Execute callback
    if ( _recvDataMap[baseTag].callback ) {
      PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVOpResult.MultiBlobReadComplete", "Executing callback");
      _recvDataMap[baseTag].callback(_recvDataMap[baseTag].data->data(), _recvDataMap[baseTag].data->size());
    }
    
    // Delete recvData object
    _recvDataMap.erase(baseTag);
  }

}

  
void NVStorageComponent::HandleNVStorageWriteEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  ExternalInterface::NVStorageWriteEntry payload = event.GetData().Get_NVStorageWriteEntry();
  PRINT_NAMED_INFO("NVStorageComponent::HandleNVStorageWriteEntry",
                   "Tag: %s, size %u",
                   EnumToString(payload.tag), payload.data_length);
  
  Write(payload.tag, payload.data.data(), payload.data_length, true);
}
  
void NVStorageComponent::HandleNVStorageReadEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  ExternalInterface::NVStorageReadEntry payload = event.GetData().Get_NVStorageReadEntry();
  PRINT_NAMED_INFO("NVStorageComponent::HandleNVStorageReadEntry", "Tag: %s", EnumToString(payload.tag));
  
  Read(payload.tag, {}, nullptr, true);
}

void NVStorageComponent::HandleNVStorageEraseEntry(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  ExternalInterface::NVStorageEraseEntry payload = event.GetData().Get_NVStorageEraseEntry();
  PRINT_NAMED_INFO("NVStorageComponent::HandleNVStorageEraseEntry", "Tag: %s", EnumToString(payload.tag));

  Erase(payload.tag, true);
}


} // namespace Cozmo
} // namespace Anki
