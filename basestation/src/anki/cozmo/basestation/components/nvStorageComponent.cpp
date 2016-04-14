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
                        "Tag: %s, %zu bytes (limit %d bytes)",
                        EnumToString(tag), size, _kMaxNvStorageEntrySize);
    return false;
  }
  
  // Check if this tag is already in the process of being written
  u32 t = static_cast<u32>(tag);
  if (_writeDataAckMap.find(t) != _writeDataAckMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.AlreadyBeingWritten",
                        "Tag: %s", EnumToString(tag));
    return false;
  }
  
  // Copy data locally and break up into as many messages as needed
  _writeDataQueue.emplace(tag, std::vector<u8>(data,data+size), true);
  
  // Set associated ack info
  _writeDataAckMap[t].broadcastResultToGame = broadcastResultToGame;
  _writeDataAckMap[t].writeNotErase = true;
  _writeDataAckMap[t].numTagsLeftToAck = static_cast<u32>(ceilf(static_cast<f32>(size) / _kMaxNvStorageBlobSize));
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.Write.DataQueued",
                      "%s - numBytes: %zu, numExpectedConfirmations: %d",
                      EnumToString(tag),
                      size,
                      _writeDataAckMap[t].numTagsLeftToAck);
  }

  return true;
}
  
bool NVStorageComponent::Erase(NVStorage::NVEntryTag tag, bool broadcastResultToGame)
{
  // Check if this tag is already in the process of being written/erased
  u32 t = static_cast<u32>(tag);
  if (_writeDataAckMap.find(t) != _writeDataAckMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.Erase.AlreadyBeingWritten",
                        "Tag: %s", EnumToString(tag));
    return false;
  }
  
  _writeDataQueue.emplace(tag, std::vector<u8>(), false);
  
  // Set associated ack info
  _writeDataAckMap[t].broadcastResultToGame = broadcastResultToGame;
  _writeDataAckMap[t].writeNotErase = false;
  _writeDataAckMap[t].numTagsLeftToAck = IsMultiBlobEntryTag(t) ? 2 : 1;  // For multiErase, expect one NvOpResult for the first blob and one at the end when all erases are complete.
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.Erase.Queued",
                      "%s", EnumToString(tag));
  }

  return true;
}
  
bool NVStorageComponent::Read(NVStorage::NVEntryTag tag,
                              NVStorageReadCallback callback,
                              std::vector<u8>* data,
                              bool broadcastResultToGame)
{
  // Check if tag already requested
  u32 entryTag = static_cast<u32>(tag);
  if (_recvDataMap.find(entryTag) != _recvDataMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.Read.TagAlreadyRequested",
                        "Ignoring tag 0x%x", entryTag);
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
  if (!_writeDataQueue.empty()) {
    WriteDataObject* sendData = &_writeDataQueue.front();

    // Start constructing write message
    NVStorage::NVStorageWrite writeMsg;
    writeMsg.reportTo = NVStorage::NVReportDest::ENGINE;
    writeMsg.writeNotErase = sendData->writeNotErase;
    writeMsg.reportEach = true;  // Note: Write is ackd (with NVOpResult) if reportEach || reportDone.
    writeMsg.reportDone = true;  //       Erase is acked iff reportEach.
                                 //       MultiErase is acked per blob if reportEach and at the end if reportDone.
    writeMsg.rangeEnd = GetTagRangeEnd(static_cast<u32>(sendData->baseTag));
    writeMsg.entry.tag = sendData->nextTag;
    
    
    if (sendData->writeNotErase) {
      // Send the next blob of data to send for this multi-blob message
      u32 bytesLeftToSend = static_cast<u32>(sendData->data.size()) - sendData->sendIndex;
      u32 bytesToSend = MIN(bytesLeftToSend, _kMaxNvStorageBlobSize);
      AnkiAssert(bytesToSend > 0);
      
      writeMsg.entry.blob = std::vector<u8>(sendData->data.begin() + sendData->sendIndex,
                                            sendData->data.begin() + sendData->sendIndex + bytesToSend);

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
        _writeDataQueue.pop();
      }
      
    } else {

      if (DEBUG_NVSTORAGE_COMPONENT) {
        PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingEraseMsg",
                          "tag: %s", EnumToString(sendData->baseTag));
      }
      
      // This is an erase message so we're definitely done with it
      _writeDataQueue.pop();
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

  u32 blobSize = static_cast<u32>(nvBlob.blob.size());
  
  // Broadcast msg up to game
  if (_recvDataMap[baseTag].broadcastResultToGame) {
    ExternalInterface::NVStorageData msg;
    msg.tag = static_cast<NVStorage::NVEntryTag>(baseTag);
    msg.index = nvBlob.tag - baseTag;
    msg.data_length = blobSize;
    memcpy(msg.data.data(), nvBlob.blob.data(), blobSize);
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
  }
  
  if (IsMultiBlobEntryTag(nvBlob.tag)) {
    u32 blobIndex = nvBlob.tag - baseTag;
    u32 startWriteIndex = blobIndex *_kMaxNvStorageBlobSize;
    u32 newTotalEntrySize = startWriteIndex + blobSize;

    // Make sure receive vector is large enough to hold data
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
  if (tag == static_cast<u32>(NVStorage::NVEntryTag::NVEntry_Invalid)) {
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.InvalidTag", "");
    return;
  }
  
  
  u32 baseTag = GetBaseEntryTag(tag);
  if (payload.report.write) {
    // Check that this was actually written data
    if (_writeDataAckMap.find(baseTag) == _writeDataAckMap.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagBaseTagWasNeverSent",
                          "BaseTag: 0x%x, tag 0x%x", baseTag, tag);
      return;
    }
    
    AnkiAssert(_writeDataAckMap[baseTag].numTagsLeftToAck > 0);
   

    // On writes, count each one.
    // On erases, only count the ones that match the base tag since we don't know how many
    // to expect in total. We only know that there'll be one for the baseTag if it's a single erase
    // or if it's multiErase.
    if (_writeDataAckMap[baseTag].writeNotErase || baseTag == tag) {
      --_writeDataAckMap[baseTag].numTagsLeftToAck;
      
      // Check if this should be forwarded on to game
      if (_writeDataAckMap[baseTag].broadcastResultToGame) {
        ExternalInterface::NVStorageOpResult msg;
        msg.tag = static_cast<NVStorage::NVEntryTag>(baseTag);
        msg.result = payload.report.result;
        msg.write = payload.report.write;
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
      }
    }
    
    if (payload.report.result != NVStorage::NVResult::NV_OKAY) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteOpFailed",
                          "Tag: 0x%x, write: %d, result: %s",
                          tag, payload.report.write, EnumToString(payload.report.result));
      return;
    }
    
    // Just a printout to indicate all sent writes were ackd
    if (_writeDataAckMap[baseTag].numTagsLeftToAck == 0) {
      PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.MsgWriteConfirmed",
                       "BaseTag: %s, lastTag: 0x%x",
                       EnumToString((NVStorage::NVEntryTag)baseTag), tag);
      _writeDataAckMap.erase(baseTag);
    }

    
  } else {
    // Check that this was actually requested data
    if (_recvDataMap.find(baseTag) == _recvDataMap.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagNeverRequested", "tag 0x%x", tag);
      return;
    }

    // Check if this should be forwarded on to game
    if (_recvDataMap[baseTag].broadcastResultToGame) {
      ExternalInterface::NVStorageOpResult msg;
      msg.tag = static_cast<NVStorage::NVEntryTag>(baseTag);
      msg.result = payload.report.result;
      msg.write = payload.report.write;
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
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
