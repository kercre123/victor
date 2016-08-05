/**
 * File: NVStorageComponent.cpp
 *
 * Author: Kevin Yoon
 * Date:   3/28/2016
 *
 * Description: Interface for storing and retrieving data from robot's non-volatile storage
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/common/robot/errorHandling.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/cpuProfiler/cpuProfiler.h"
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
    if (extInterface != nullptr) {
      
      auto helper = MakeAnkiEventUtil(*extInterface, *this, _signalHandles);
      
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageWriteEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageReadEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageEraseEntry>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageWipeAll>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::NVStorageClearPartialPendingWriteEntry>();
    }
    
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
    doRobotSubscribe(RobotInterface::RobotToEngineTag::nvData,          &NVStorageComponent::HandleNVData);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::nvResult,        &NVStorageComponent::HandleNVOpResult);
    
  } else {
    PRINT_NAMED_WARNING("NVStorageComponent.nullContext", "");
  }

}

bool NVStorageComponent::IsMultiBlobEntryTag(u32 tag) const {
  return (tag & 0x7fff0000) > 0 && !IsSpecialEntryTag(tag);
}

bool NVStorageComponent::IsValidEntryTag(u32 tag) const {
  // If multi-blob tag, lower 16-bits must be zero.
  // If single-blob tag, it can't be NVEntry_Invalid.
  if (IsMultiBlobEntryTag(tag)) {
    return (tag & 0xffff) == 0;
  } else {
    return tag != static_cast<u32>(NVStorage::NVEntryTag::NVEntry_Invalid);
  }
}
 
bool NVStorageComponent::IsFactoryEntryTag(u32 tag) const
{
  return (tag & 0x80000000) > 0;
}
  
  
bool NVStorageComponent::IsSpecialEntryTag(u32 tag) const
{
  return (tag & 0xffff0000) == 0xc0000000;
}
  
// Returns the base (i.e. lowest) tag of the multi-blob entry range
// in which the given tag falls.
// See NVEntryTag definition for more details.
u32 NVStorageComponent::GetBaseEntryTag(u32 tag) const {
  u32 baseTag = tag & 0x7fff0000;
  if (baseTag == 0 || IsSpecialEntryTag(tag)) {
    // For single blob message, the base tag is the same as tag
    return tag;
  }
  return tag & 0xffff0000;
}
  
// Returns the end (i.e. highest) tag of the multi-blob entry range
// in which the given tag falls.
// See NVEntryTag definition for more details.
u32 NVStorageComponent::GetTagRangeEnd(u32 startTag) const {
  if (IsMultiBlobEntryTag(startTag)) {
    return startTag | 0xffff;
  } else {
    return startTag;
  }
}

bool NVStorageComponent::Write(NVStorage::NVEntryTag tag,
                               const std::vector<u8>* data,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  // Check for null data
  if (data == nullptr) {
    PRINT_NAMED_INFO("NVStorageComponent.Write.NullData", "%s", EnumToString(tag));
    return false;
  }
  
  return Write(tag, data->data(), data->size(), callback, broadcastResultToGame);
}
  
bool NVStorageComponent::Write(NVStorage::NVEntryTag tag,
                               const u8* data, size_t size,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  // Check for invalid tags
  if (!IsValidEntryTag(static_cast<u32>(tag)) ||
      tag == NVStorage::NVEntryTag::NVEntry_WipeAll) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidTag",
                        "Tag: 0x%x", static_cast<u32>(tag));
    return false;
  }
  
  // Check for null data
  if (data == nullptr) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.NullData", "%s", EnumToString(tag));
    return false;
  }
  
  // Sanity check on size
  if (size > _kMaxNvStorageEntrySize || size <= 0) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.InvalidSize",
                        "Tag: %s, %zu bytes (limit %d bytes)",
                        EnumToString(tag), size, _kMaxNvStorageEntrySize);
    return false;
  }

  
  // If this is a multi-blob write...
  if (IsMultiBlobEntryTag(static_cast<u32>(tag))) {
    
    if (!IsFactoryEntryTag(static_cast<u32>(tag))) {
      // Queue an erase first in case this write has fewer blobs
      // than what is already stored in the robot.
      PRINT_NAMED_DEBUG("NVStorageComponent.Write.PreceedingMultiBlobWriteWithErase",
                        "Tag: %s", EnumToString(tag));
      _requestQueue.emplace(tag, NVStorageWriteEraseCallback(), false);
    }
  }
  
  // If this is a single blob write
  // check that is has no more than one blob's worth of data
  else if (size > _kMaxNvStorageBlobSize) {
    PRINT_NAMED_WARNING("NVStorageComponent.Write.SingleBlobEntryHasTooMuchData",
                        "Tag: %s, size: %zu",
                        EnumToString(tag), size);
    return false;
  }
  
  // Queue write request
  _requestQueue.emplace(tag, callback, new std::vector<u8>(data,data+size), broadcastResultToGame);
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.Write.DataQueued",
                      "%s - numBytes: %zu",
                      EnumToString(tag),
                      size);
  }
  
  return true;
}
  
bool NVStorageComponent::Erase(NVStorage::NVEntryTag tag,
                               NVStorageWriteEraseCallback callback,
                               bool broadcastResultToGame)
{
  // Check if it's a legal tag (in case someone did some casting craziness)
  if (!IsValidEntryTag(static_cast<u32>(tag))) {
    PRINT_NAMED_WARNING("NVStorageComponent.Erase.InvalidEntryTag",
                        "Tag: 0x%x", static_cast<u32>(tag));
    return false;
  }
  
  // Converting to WipeAll command for convenience
  if (tag == NVStorage::NVEntryTag::NVEntry_WipeAll) {
    return WipeAll(false, callback, broadcastResultToGame);
  }
  
  _requestQueue.emplace(tag, callback, broadcastResultToGame);
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.Erase.Queued",
                      "%s", EnumToString(tag));
  }

  return true;
}
  
  
bool NVStorageComponent::WipeAll(bool includeFactory,
                                 NVStorageWriteEraseCallback callback,
                                 bool broadcastResultToGame)
{
  _requestQueue.emplace(includeFactory, callback, broadcastResultToGame);
  
  if (DEBUG_NVSTORAGE_COMPONENT) {
    PRINT_NAMED_DEBUG("NVStorageComponent.WipeAll.Queued", "includeFactory: %d", includeFactory);
  }
  
  return true;
}
  
bool NVStorageComponent::Read(NVStorage::NVEntryTag tag,
                              NVStorageReadCallback callback,
                              std::vector<u8>* data,
                              bool broadcastResultToGame)
{
  // Check for invalid tags
  if (!IsValidEntryTag(static_cast<u32>(tag)) ||
      tag == NVStorage::NVEntryTag::NVEntry_WipeAll) {
    PRINT_NAMED_WARNING("NVStorageComponent.Read.InvalidTag",
                        "Tag: 0x%x", static_cast<u32>(tag));
    return false;
  }
  
  PRINT_NAMED_INFO("NVStorageComponent.Read.QueueingReadRequest", "%s", EnumToString(tag));
  _requestQueue.emplace(tag, callback, data, broadcastResultToGame);
  
  return true;
}

  
void NVStorageComponent::SendRequest(NVStorageRequest req)
{
  u32 entryTag = static_cast<u32>(req.tag);
  
  switch(req.op) {
    case NVStorage::NVOperation::NVOP_WRITE:
    {
      // Copy data locally and break up into as many messages as needed
      _writeDataQueue.emplace(req.tag, req.data);
      
      // Set associated ack info
      u32 t = static_cast<u32>(req.tag);
      _writeDataAckMap[t].timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckMap[t].broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckMap[t].writeNotErase = true;
      _writeDataAckMap[t].numTagsLeftToAck = static_cast<u32>(ceilf(static_cast<f32>(req.data->size()) / _kMaxNvStorageBlobSize));
      _writeDataAckMap[t].numFailedWrites = 0;
      _writeDataAckMap[t].callback = req.writeCallback;
      
      _wasLastWriteAcked = true;

      if (DEBUG_NVSTORAGE_COMPONENT) {
        PRINT_NAMED_DEBUG("NVStorageComponent.SendRequest.SendingWrite",
                          "StartTag: 0x%x, timeoutTime: %d, currTime: %d",
                          req.tag, _writeDataAckMap[t].timeoutTimeStamp, _robot.GetLastMsgTimestamp() );
      }
      
      break;
    }
    case NVStorage::NVOperation::NVOP_ERASE:
    {
      // Set associated ack info
      u32 t = static_cast<u32>(req.tag);
      _writeDataAckMap[t].timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckMap[t].broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckMap[t].writeNotErase = false;
      _writeDataAckMap[t].callback = req.writeCallback;
      _writeDataAckMap[t].numTagsLeftToAck = 1;
      _writeDataAckMap[t].numFailedWrites = 0;
      
      // Start constructing erase message
      _writeMsg.reportTo = NVStorage::NVReportDest::ENGINE;
      _writeMsg.writeNotErase = false;
      _writeMsg.reportEach = !IsMultiBlobEntryTag(t);
      _writeMsg.reportDone = true;
      _writeMsg.entry.tag = static_cast<u32>(req.tag);
      
      // Set tag range
      PRINT_NAMED_DEBUG("NVStorageComponent.SendRequest.SendingErase", "%s (Tag: 0x%x)", EnumToString(req.tag), t );
      _writeMsg.rangeEnd = GetTagRangeEnd(static_cast<u32>(req.tag));
      
      _robot.SendMessage(RobotInterface::EngineToRobot(NVStorage::NVStorageWrite(_writeMsg)));
      
      break;
    }
    case NVStorage::NVOperation::NVOP_WIPEALL:
    {
      PRINT_NAMED_DEBUG("NVStoageComponent.SendRequest.SendingWipeAll", "");
      
      u32 t = static_cast<u32>(NVStorage::NVEntryTag::NVEntry_WipeAll);
      _writeDataAckMap[t].timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
      _writeDataAckMap[t].broadcastResultToGame = req.broadcastResultToGame;
      _writeDataAckMap[t].writeNotErase = false;
      _writeDataAckMap[t].callback = req.writeCallback;
      _writeDataAckMap[t].numTagsLeftToAck = 1;
      _writeDataAckMap[t].numFailedWrites = 0;
      
      NVStorage::NVWipeAll wipeMsg;
      wipeMsg.key = "Yes I really want to do this!";
      wipeMsg.includeFactory = req.wipeFactory;
      _robot.SendMessage(RobotInterface::EngineToRobot(std::move(wipeMsg)));
      
      break;
    }
    case NVStorage::NVOperation::NVOP_READ:
    {
      // Request message from tag to to endTag
      _readMsg.tag = entryTag;
      _readMsg.tagRangeEnd = GetTagRangeEnd(entryTag);
      _readMsg.to = NVStorage::NVReportDest::ENGINE;
      _robot.SendMessage(RobotInterface::EngineToRobot(NVStorage::NVStorageRead(_readMsg)));
      
      
      if (DEBUG_NVSTORAGE_COMPONENT) {
        PRINT_NAMED_DEBUG("NVStorageComponent.SendRequest.SendingRead",
                          "StartTag: 0x%x, EndTag: 0x%x",
                          _readMsg.tag,
                          _readMsg.tagRangeEnd);
      }
      
      // Create RecvDataObject
      if (nullptr == req.data) {
        _recvDataMap[entryTag].data = new std::vector<u8>();
        _recvDataMap[entryTag].deleteVectorWhenDone = true;
      } else {
        _recvDataMap[entryTag].data = req.data;
        _recvDataMap[entryTag].deleteVectorWhenDone = false;
      }
      _recvDataMap[entryTag].callback = req.readCallback;
      _recvDataMap[entryTag].broadcastResultToGame = req.broadcastResultToGame;
      _recvDataMap[entryTag].timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;

      break;
    }
    default:
      PRINT_NAMED_WARNING("NVStorageComponent.SendRequest.UnsupportOperation", "%s", EnumToString(req.op));
      break;
  }
  
  // Retry send attempt counter
  _numSendAttempts = 0;
  
}
  
bool NVStorageComponent::HasPendingRequests()
{
  return !_requestQueue.empty() || !_recvDataMap.empty() || !_writeDataAckMap.empty();
}
  
void NVStorageComponent::Update()
{
  ANKI_CPU_PROFILE("NVStorageComponent::Update");
  
  // If expecting read data to arrive, check timeout.
  if (!_recvDataMap.empty()) {
    if (_recvDataMap.size() > 1) {
      PRINT_NAMED_WARNING("NVStorageComponent.Update.ExpectingMoreThanOneRead",
                          "Num reads waiting to be ackd: %zu", _recvDataMap.size());
    }
    
    if (_robot.GetLastMsgTimestamp() > _recvDataMap.begin()->second.timeoutTimeStamp) {
      PRINT_NAMED_WARNING("NVStorageComponent.Update.ReadTimeout",
                          "Tag: 0x%x", _recvDataMap.begin()->first);
      
      _recvDataMap.begin()->second.callback(nullptr, 0, NVStorage::NVResult::NV_TIMEOUT);
      _recvDataMap.erase(_recvDataMap.begin());
    }
    return;
  }
  
  // If expecting write/erase ack, check timeout.
  if (!_writeDataAckMap.empty()) {
    if (_writeDataAckMap.size() > 1) {
      PRINT_NAMED_WARNING("NVStorageComponent.Update.ExpectingMoreThanOneWrite",
                          "Num write entries waiting to be ackd: %zu", _recvDataMap.size());
    }
    
    if (_robot.GetLastMsgTimestamp() > _writeDataAckMap.begin()->second.timeoutTimeStamp) {
      PRINT_NAMED_WARNING("NVStorageComponent.Update.WriteTimeout",
                          "Tag: 0x%x", _writeDataAckMap.begin()->first);
      _writeDataAckMap.erase(_writeDataAckMap.begin());
      _wasLastWriteAcked = true;
    }
    // Fall through because we still potentially need to send multi-blob data
  }

  
  // Send requests if there are any in the queue and we're not currently processing a write
  if (_writeDataQueue.empty() && _writeDataAckMap.empty() && !_requestQueue.empty()) {
    SendRequest(_requestQueue.front());
    _requestQueue.pop();
  }
  
  
  // If there are writes to send, send them.
  // Send up to one blob per Update() call.
  if (!_writeDataQueue.empty() && _wasLastWriteAcked) {
    _wasLastWriteAcked = false;
    WriteDataObject* sendData = &_writeDataQueue.front();

    // Fill out write message
    _writeMsg.reportTo = NVStorage::NVReportDest::ENGINE;
    _writeMsg.writeNotErase = true;
    _writeMsg.reportEach = true;  // Note: Write is ackd (with NVOpResult) if reportEach || reportDone.
    _writeMsg.reportDone = true;  //       Erase is acked iff reportEach.
                                 //       MultiErase is acked per blob if reportEach and at the end if reportDone.
    _writeMsg.rangeEnd = GetTagRangeEnd(static_cast<u32>(sendData->baseTag));
    _writeMsg.entry.tag = sendData->nextTag;
    

    // Send the next blob of data to send for this multi-blob message
    u32 bytesLeftToSend = static_cast<u32>(sendData->data->size()) - sendData->sendIndex;
    u32 bytesToSend = MIN(bytesLeftToSend, _kMaxNvStorageBlobSize);
    ASSERT_NAMED(bytesToSend > 0, "NVStorageComponent.Update.ExpectedPositiveNumBytesToSend");
    
    _writeMsg.entry.blob = std::vector<u8>(sendData->data->begin() + sendData->sendIndex,
                                               sendData->data->begin() + sendData->sendIndex + bytesToSend);

    ++sendData->nextTag;
    sendData->sendIndex += bytesToSend;
    
    if (DEBUG_NVSTORAGE_COMPONENT) {
      PRINT_NAMED_DEBUG("NVStorageComponent.Update.SendingWriteMsg",
                        "BaseTag: %s, tag: 0x%x, write: %d, bytesSent: %d",
                        EnumToString(sendData->baseTag),
                        _writeMsg.entry.tag,
                        _writeMsg.writeNotErase,
                        bytesToSend);
    }
    
    // Sent last blob?
    if (sendData->sendIndex >= sendData->data->size()) {
      _writeDataQueue.pop();
    }
    
    _robot.SendMessage(RobotInterface::EngineToRobot(NVStorage::NVStorageWrite(_writeMsg)));
    _numSendAttempts = 0;
  }
  
}
  
bool NVStorageComponent::ResendLastWrite()
{
  if (++_numSendAttempts >= _kMaxNumSendAttempts) {
    PRINT_NAMED_WARNING("NVStorageComponent.ResendLastWrite.NumRetriesExceeded",
                        "Tag: 0x%x, writeNotErase: %d, Attempts: %d",
                        _writeMsg.entry.tag, _writeMsg.writeNotErase, _kMaxNumSendAttempts );
    return false;
  }
  
  PRINT_NAMED_INFO("NVStorageComponent.ResendLastWrite.Retry",
                   "Tag: 0x%x, retryNum: %d", _writeMsg.entry.tag, _numSendAttempts);
  _robot.SendMessage(RobotInterface::EngineToRobot(NVStorage::NVStorageWrite(_writeMsg)));
  return true;
}
  
bool NVStorageComponent::ResendLastRead()
{
  if (++_numSendAttempts >= _kMaxNumSendAttempts) {
    PRINT_NAMED_WARNING("NVStorageComponent.ResendLastRead.NumRetriesExceeded",
                        "Tag: 0x%x, Attempts: %d",
                        _readMsg.tag, _kMaxNumSendAttempts );
    return false;
  }
  
  PRINT_NAMED_INFO("NVStorageComponent.ResendLastRead.Retry",
                   "Tag: 0x%x, retryNum: %d", _readMsg.tag, _numSendAttempts);
  _robot.SendMessage(RobotInterface::EngineToRobot(NVStorage::NVStorageRead(_readMsg)));
  return true;
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
    
    // Bump timeout
    _recvDataMap[baseTag].timeoutTimeStamp = _robot.GetLastMsgTimestamp() + _kAckTimeout_ms;
    
    if (DEBUG_NVSTORAGE_COMPONENT) {
      PRINT_NAMED_DEBUG("NVStorageComponent.HandelNVData.ReceivedBlob",
                        "BaseEntryTag: 0x%x, Tag: 0x%x, BlobSize %d, StartWriteIndex: %d",
                        baseTag, nvBlob.tag, blobSize, startWriteIndex);
    }
    
  } else {
    
    // If this is not a multiblob message, execute callback now.
    PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.MsgRecvd",
                     "BaseTag: %s, Tag: 0x%x, size: %u",
                     EnumToString(static_cast<NVStorage::NVEntryTag>(baseTag)), nvBlob.tag, blobSize);
    if (_recvDataMap[baseTag].callback) {
      PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.ExecutingCallback",
                       "BaseTag: %s, Tag: 0x%x",
                       EnumToString(static_cast<NVStorage::NVEntryTag>(baseTag)), nvBlob.tag);
      _recvDataMap[baseTag].callback(nvBlob.blob.data(), nvBlob.blob.size(), NVStorage::NVResult::NV_OKAY);
    }

    // Sending a result message since the robot doesn't actually send one back
    // for successful single-blob reads.
    if (_recvDataMap[baseTag].broadcastResultToGame) {
      BroadcastNVStorageOpResult(static_cast<NVStorage::NVEntryTag>(baseTag), NVStorage::NVResult::NV_OKAY, NVStorage::NVOperation::NVOP_READ);
    }

    _recvDataMap.erase(baseTag);
  }
}

void NVStorageComponent::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ANKI_CPU_PROFILE("Robot::HandleNVOpResult");
  
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
  const char* baseTagStr = EnumToString(static_cast<NVStorage::NVEntryTag>(baseTag));
  if (payload.report.write) {
    
    // Check that this was actually written data
    if (_writeDataAckMap.find(baseTag) == _writeDataAckMap.end()) {
      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.AckdTagBaseTagWasNeverSent",
                          "BaseTag: 0x%x, tag 0x%x", baseTag, tag);
      return;
    }
    
    AnkiAssert(_writeDataAckMap[baseTag].numTagsLeftToAck > 0);
   
    // Possibly increment the failed write count based on the result
    // Negative results == bad
    if (static_cast<s8>(payload.report.result) < 0) {
      
      // Under these cases, attempt to resend the failed write message.
      // If the allowed number of retries fails
      if (payload.report.result == NVStorage::NVResult::NV_NO_MEM ||
          payload.report.result == NVStorage::NVResult::NV_BUSY   ||
          payload.report.result == NVStorage::NVResult::NV_TIMEOUT) {
        if (ResendLastWrite()) {
          PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.ResentFailedWrite",
                           "Tag 0x%x resent due to %s", tag, EnumToString(payload.report.result));
          return;
        }
      }

      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.WriteOpFailed",
                          "Tag: 0x%x, writeNotErase: %d, result: %s, numWriteFails: %u",
                          tag, _writeDataAckMap[baseTag].writeNotErase, EnumToString(payload.report.result), _writeDataAckMap[baseTag].numFailedWrites);
      ++_writeDataAckMap[baseTag].numFailedWrites;
    }

    // On writes, count each one.
    // On erases, only count the ones that match the base tag since we don't know how many
    // to expect in total. We only know that there'll be one for the baseTag if it's a single erase
    // or two if it's multiErase.
    if (_writeDataAckMap[baseTag].writeNotErase || baseTag == tag) {
      --_writeDataAckMap[baseTag].numTagsLeftToAck;
      _wasLastWriteAcked = true;
      
      // If any not okay results come back as a result of an erase (like if the tag doesn't exist),
      // don't expect anything more to come.
      if (payload.report.result != NVStorage::NVResult::NV_OKAY && !_writeDataAckMap[baseTag].writeNotErase) {
        _writeDataAckMap[baseTag].numTagsLeftToAck = 0;
      }
      
      
      // Check if this should be forwarded on to game.
      // For multi-erase, only forward the final (i.e. 2nd) result
      if (_writeDataAckMap[baseTag].broadcastResultToGame &&
          (_writeDataAckMap[baseTag].writeNotErase || _writeDataAckMap[baseTag].numTagsLeftToAck == 0)) {
        BroadcastNVStorageOpResult(static_cast<NVStorage::NVEntryTag>(baseTag),
                                   payload.report.result,
                                   _writeDataAckMap[baseTag].writeNotErase ? NVStorage::NVOperation::NVOP_WRITE : NVStorage::NVOperation::NVOP_ERASE);
      }
    }
          
    // Check if all writes have been confirmed for this write request
    if (_writeDataAckMap[baseTag].numTagsLeftToAck == 0) {
      
      // If this is a multiblob write, modify the final result based on the number of failures
      if (IsMultiBlobEntryTag(baseTag)) {
        payload.report.result = _writeDataAckMap[baseTag].numFailedWrites > 0 ? NVStorage::NVResult::NV_ERROR : NVStorage::NVResult::NV_OKAY;
      }
      
      // Print result
      if (payload.report.result == NVStorage::NVResult::NV_OKAY) {
        PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.MsgWriteConfirmed",
                         "BaseTag: %s, lastTag: 0x%x, writeNotErase: %d, numWriteFails: %u, result: %s",
                         baseTagStr, tag, _writeDataAckMap[baseTag].writeNotErase, _writeDataAckMap[baseTag].numFailedWrites, EnumToString(payload.report.result));
      } else {
        PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.MsgWriteFailed",
                            "BaseTag: %s, lastTag: 0x%x, writeNotErase: %d, numWriteFails: %u, result: %s",
                            baseTagStr, tag, _writeDataAckMap[baseTag].writeNotErase, _writeDataAckMap[baseTag].numFailedWrites, EnumToString(payload.report.result));
      }
      
      // Execute write complete callback
      if (_writeDataAckMap[baseTag].callback) {
        PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVOpResult.ExecutingWriteCallback", "%s", baseTagStr);
        _writeDataAckMap[baseTag].callback(payload.report.result);
      }
      
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
      BroadcastNVStorageOpResult(static_cast<NVStorage::NVEntryTag>(baseTag),
                                 payload.report.result,
                                 NVStorage::NVOperation::NVOP_READ);
    }

    // If read was not successful, possibly retry.
    if (static_cast<s8>(payload.report.result) < 0) {
      
      // Under these cases, attempt to resend the failed read message
      if (payload.report.result == NVStorage::NVResult::NV_NO_MEM ||
          payload.report.result == NVStorage::NVResult::NV_BUSY   ||
          payload.report.result == NVStorage::NVResult::NV_TIMEOUT) {
        if (ResendLastRead()) {
          PRINT_NAMED_INFO("NVStorageComponent.HandleNVOpResult.ResentFailedRead",
                           "Tag 0x%x resent due to %s", tag, EnumToString(payload.report.result));
          return;
        }
      }

      PRINT_NAMED_WARNING("NVStorageComponent.HandleNVOpResult.ReadOpFailed",
                          "BaseTag: %s, Tag: 0x%x, write: %d, result: %s",
                          baseTagStr, tag, payload.report.write, EnumToString(payload.report.result));
    }

    
    // Execute callback
    if ( _recvDataMap[baseTag].callback ) {
      PRINT_NAMED_DEBUG("NVStorageComponent.HandleNVOpResult.ExecutingReadCallback", "%s", baseTagStr);
      _recvDataMap[baseTag].callback(_recvDataMap[baseTag].data->data(),
                                     _recvDataMap[baseTag].data->size(),
                                     payload.report.result);
    }
    
    // Delete recvData object
    _recvDataMap.erase(baseTag);
  }
}
  
void NVStorageComponent::BroadcastNVStorageOpResult(NVStorage::NVEntryTag tag, NVStorage::NVResult res, NVStorage::NVOperation op)
{
  ExternalInterface::NVStorageOpResult msg;
  msg.tag = tag;
  msg.result = res;
  msg.op = op;
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
}

bool NVStorageComponent::QueueWriteBlob(const NVStorage::NVEntryTag tag,
                                        const u8* data, u16 dataLength,
                                        u8 blobIndex, u8 numTotalBlobs)
{
  AnkiAssert(IsMultiBlobEntryTag(static_cast<u32>(tag)));
 
  // Check for invalid tags
  if (!IsValidEntryTag(static_cast<u32>(tag))) {
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.InvalidTag",
                        "Tag: 0x%x", static_cast<u32>(tag));
    return false;
  }
  
  if (numTotalBlobs > _kMaxNumBlobsInMultiBlobEntry) {
    PRINT_NAMED_WARNING("NVStorageComponent.QueueWriteBlob.TooManyBlobs",
                        "Num total blobs %d exceeds max of %d",
                        numTotalBlobs, _kMaxNumBlobsInMultiBlobEntry);
    return false;
  }
  
  if (_pendingWriteData.tag != tag) {
    if (_pendingWriteData.tag == NVStorage::NVEntryTag::NVEntry_Invalid) {
      PRINT_NAMED_DEBUG("NVStorageComponent.QueueWriteBlob.FirstBlobRecvd", "Tag: %s", EnumToString(tag));
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
    PRINT_NAMED_INFO("NVStorageComponent.QueueWriteBlob.SendingToRobot",
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
  PRINT_NAMED_INFO("NVStorageComponent.HandleNVStorageWriteEntry.Recvd",
                   "Tag: %s, size %u",
                   EnumToString(msg.tag), msg.data_length);

  bool res = true;
  if (IsMultiBlobEntryTag(static_cast<u32>(msg.tag))) {
    res = QueueWriteBlob(msg.tag,
                         msg.data.data(), msg.data_length,
                         msg.index, msg.numTotalBlobs);
  } else {
    res = Write(msg.tag, msg.data.data(), msg.data_length, {}, true);
  }
  
  // If failed before request is even sent to robot, then send OpResult now.
  if (!res) {
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVStorageWriteEntry.FailedToQueueOrSend",
                        "Tag: %s, size: %d, blobIndex: %d, numTotalBlobs: %d",
                        EnumToString(msg.tag), msg.data_length, msg.index, msg.numTotalBlobs);
    BroadcastNVStorageOpResult(msg.tag, NVStorage::NVResult::NV_ERROR, NVStorage::NVOperation::NVOP_WRITE);
  }
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageReadEntry& msg)
{
  PRINT_NAMED_INFO("NVStorageComponent.HandleNVStorageReadEntry.Recvd", "Tag: %s", EnumToString(msg.tag));
  
  if (!Read(msg.tag, {}, nullptr, true)) {
    // If failed before request is even sent to robot, then send OpResult now.
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVStorageReadEntry.FailedToQueueOrSend",
                        "Tag: %s", EnumToString(msg.tag));
    BroadcastNVStorageOpResult(msg.tag, NVStorage::NVResult::NV_ERROR, NVStorage::NVOperation::NVOP_READ);
  }
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageEraseEntry& msg)
{
  PRINT_NAMED_INFO("NVStorageComponent.HandleNVStorageEraseEntry.Recvd", "Tag: %s", EnumToString(msg.tag));

  if (!Erase(msg.tag, {}, true)) {
    // If failed before request is even sent to robot, then send OpResult now.
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVStorageEraseEntry.FailedToQueueOrSend",
                        "Tag: %s", EnumToString(msg.tag));
    BroadcastNVStorageOpResult(msg.tag, NVStorage::NVResult::NV_ERROR, NVStorage::NVOperation::NVOP_ERASE);
  }
}

template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageWipeAll& msg)
{
  PRINT_NAMED_INFO("NVStorageComponent.HandleNVStorageWipeAll.Recvd", "key: %s", msg.key.c_str());
  
  // Note: Not allowing factory wipes from game
  if (!WipeAll(false, {}, true)) {
    // If failed before request is even sent to robot, then send OpResult now.
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVStorageWipeAll.FailedToQueueOrSend", "");
    BroadcastNVStorageOpResult(NVStorage::NVEntryTag::NVEntry_WipeAll, NVStorage::NVResult::NV_ERROR, NVStorage::NVOperation::NVOP_WIPEALL);
  }
}
  
template<>
void NVStorageComponent::HandleMessage(const ExternalInterface::NVStorageClearPartialPendingWriteEntry& msg)
{
  ClearPendingWriteEntry();
}

void NVStorageComponent::ClearPendingWriteEntry() {
  _pendingWriteData.tag = NVStorage::NVEntryTag::NVEntry_Invalid;
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
  if (!ANKI_DEVELOPER_CODE) {
    return;
  }
  
  
  NVStorage::NVEntryTag testMultiBlobTag = NVStorage::NVEntryTag::NVEntry_MultiBlobJunk;
  

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
            [](NVStorage::NVResult res) {
            PRINT_NAMED_DEBUG("NVStorageComponent.Test.WriteResult", "%s", EnumToString(res));
            });
      break;
    }
    case 1:
    case 3:
    {
      PRINT_NAMED_DEBUG("NVStorageComponent.Test.Read", "tag: %s", EnumToString(testMultiBlobTag));
      Read(testMultiBlobTag,
           [](u8* data, size_t size, NVStorage::NVResult res) {
             if (res == NVStorage::NVResult::NV_OKAY) {
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
            [](NVStorage::NVResult res) {
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
         [](u8* data, size_t size, NVStorage::NVResult res) {
           
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

}
  
  
} // namespace Cozmo
} // namespace Anki
