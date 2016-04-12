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
//#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
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
    //auto callback = std::bind(&NVStorageComponent::HandleGameEvents, this, std::placeholders::_1);
    //_signalHandles.push_back(context->Subscribe(ExternalInterface::MessageGameToEngineTag::GetBlockPoolMessage, callback));

    
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

bool NVStorageComponent::Write(NVStorage::NVEntryTag tag, u8* data, int size)
{
  // Copy data locally and break up into as many messages as needed
  _dataToSendQueue.emplace((u32)tag, std::vector<u8>(data,data+size));

  return true;
}
  
void NVStorageComponent::Erase(NVStorage::NVEntryTag tag)
{
  NVStorage::NVStorageWrite eraseMsg;
  eraseMsg.reportTo = NVStorage::NVReportDest::ENGINE;
  eraseMsg.writeNotErase = false;
  eraseMsg.reportEach = false;
  eraseMsg.reportDone = true;
  eraseMsg.rangeEnd = GetTagRangeEnd((u32)tag);
  eraseMsg.entry.tag = (u32)tag;
  _robot.SendMessage(RobotInterface::EngineToRobot(std::move(eraseMsg)));
}
  
bool NVStorageComponent::Read(NVStorage::NVEntryTag tag, NVStorageReadCallback callback, std::vector<u8>* data)
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
  
  return true;
}
  
  
void NVStorageComponent::Update()
{
  // If there are things to send, send them.
  // Send up to one blob per Update() call.
  if (!_dataToSendQueue.empty()) {
    SendDataObject* sendData = &_dataToSendQueue.front();
    
    u32 bytesLeftToSend = (u32)sendData->data.size() - sendData->sendIndex;
    u32 bytesToSend = MIN(bytesLeftToSend, _kMaxNvStorageBlobSize);
    assert(bytesToSend > 0);
    
    NVStorage::NVStorageWrite writeMsg;
    writeMsg.reportTo = NVStorage::NVReportDest::ENGINE;
    writeMsg.writeNotErase = true;
    writeMsg.reportEach = false;
    writeMsg.reportDone = true;
    writeMsg.rangeEnd = (u32)NVStorage::NVEntryTag::NVEntry_Invalid;
    writeMsg.entry.tag = sendData->nextTag;
    writeMsg.entry.blob = std::vector<u8>(sendData->data.begin() + sendData->sendIndex,
                                          sendData->data.begin() + sendData->sendIndex + bytesToSend);
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(writeMsg)));

    _sentWriteTags[sendData->baseTag].insert(sendData->nextTag);
    ++sendData->nextTag;
    sendData->sendIndex += bytesToSend;


    
    if (DEBUG_NVSTORAGE_COMPONENT) {
      PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingWriteMsg",
                        "tag: 0x%x, write: %d, bytesSent: %d",
                        writeMsg.entry.tag,
                        writeMsg.writeNotErase,
                        bytesToSend);
    }

    
    // Sent last blob?
    if (bytesToSend < _kMaxNvStorageBlobSize) {
      _dataToSendQueue.pop();
    }
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
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVData.DataRecvd",
                      "tag: 0x%x", nvBlob.tag);
  }
  
  if (IsMultiBlobEntryTag(nvBlob.tag)) {
    // Make sure receive vector is large enough to hold data
    u32 blobIndex = nvBlob.tag - baseTag;
    u32 blobSize = (u32)nvBlob.Size();
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
                        "Tag: 0x%x, BaseEntryTag: 0x%x, BlobSize %d, StartWriteIndex: %d",
                        nvBlob.tag, baseTag, blobSize, startWriteIndex);
    }
    
  } else {
    
    // If this is not a multiblob message, execute callback now.
    // TODO: Or should I expect a NvAllBlobsSent message even for single blob messages?
    
    PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.MsgRecvd", "Executing callback");
    _recvDataMap[baseTag].callback(nvBlob.blob.data(), nvBlob.blob.size());
    _recvDataMap.erase(baseTag);
  }
}

void NVStorageComponent::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  RobotInterface::NVOpResultToEngine payload = message.GetData().Get_nvResult();
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVOpResult.Recvd",
                      "RobotAddr: %u.%u.%u.%u, Tag: 0x%x, Result: %s, write: %d",
                      (payload.robotAddress & 0xf000) >> 24,
                      (payload.robotAddress & 0x0f00) >> 16,
                      (payload.robotAddress & 0x00f0) >> 8,
                      (payload.robotAddress & 0x000f),
                      payload.report.tag,
                      EnumToString(payload.report.result),
                      payload.report.write);
  }
  

  
  u32 tag = (u32)payload.report.tag;   // TODO: Make report.tag u32?
  u32 baseTag = GetBaseEntryTag(tag);
  if (payload.report.write) {
    // Check that this was actually written data
    if (_sentWriteTags.find(baseTag) == _sentWriteTags.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagBaseTagWasNeverSent",
                          "tag 0x%x, baseTag 0x%x", tag, baseTag);
      return;
    }
    
    if (_sentWriteTags[baseTag].find(tag) == _sentWriteTags[baseTag].end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagWasNeverSent",
                          "tag 0x%x, baseTag 0x%x", tag, baseTag);
      return;
    }

    _sentWriteTags[baseTag].erase(tag);
    if (_sentWriteTags[baseTag].size() == 0) {
      PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.MsgWriteConfirmed", "%s", EnumToString((NVStorage::NVEntryTag)baseTag));
    }

    
    if (payload.report.result != NVStorage::NVResult::NV_OKAY) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteOpFailed",
                          "Tag: 0x%x, write: %d, result: %s",
                          tag, payload.report.write, EnumToString(payload.report.result));
      return;
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


  
/*
  
void NVStorageComponent::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    default:
    {
      PRINT_NAMED_ERROR("NVStorageComponent::HandleGameEvents unexpected message","%hhu",event.GetData().GetTag());
    }
  }
}
 */

} // namespace Cozmo
} // namespace Anki
