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

bool NVStorageComponent::Save(NVStorage::NVEntryTag tag, u8* data, int size) 
{
  // Copy data locally and break up into as many messages as needed
  _dataToSendQueue.emplace((u32)tag, std::vector<u8>(data,data+size));

  return true;
}
  
bool NVStorageComponent::Request(NVStorage::NVEntryTag tag, std::vector<u8>& data, NVStorageBlobReceiveCallback callback)
{
  u32 endTag = (u32)tag;
  if (IsMultiBlobEntryTag((u32)tag)) {
    endTag |= 0x0000ffff;
  }
  
  // TODO: Request message from tag to to endTag
  // ...
  
  
  
  // Save RecvDataObject
  if (_recvDataMap.find(tag) != _recvDataMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.Request.TagAlreadyRequested", "Ignoring tag %d", (u32)tag);
    return false;
  }
  _recvDataMap[tag].data = &data;
  _recvDataMap[tag].callback = callback;
  
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
    
    NVStorage::NVStorageBlob blobMsg;
    blobMsg.tag = sendData->nextTag;
    blobMsg.blob = std::vector<u8>(sendData->data.begin() + sendData->sendIndex,
                                   sendData->data.begin() + sendData->sendIndex + bytesToSend);
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(blobMsg)));
    
    ++sendData->nextTag;
    sendData->sendIndex += bytesToSend;
    
    // Sent last blob?
    if (bytesToSend < _kMaxNvStorageBlobSize) {
      _dataToSendQueue.pop();
    }
  }
  
}

void NVStorageComponent::HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  NVStorage::NVStorageBlob nvBlob = message.GetData().Get_nvData().blob;
  
  // Check that this was actually requested data
  if (_recvDataMap.find((NVStorage::NVEntryTag)nvBlob.tag) == _recvDataMap.end()) {
    PRINT_NAMED_WARNING("NVStorageComponent.HandleNVData.TagNotRequested", "tag %d", nvBlob.tag);
    return;
  }
  
  
  if (IsMultiBlobEntryTag(nvBlob.tag)) {
    // Make sure receive vector is large enough to hold data
    u32 baseTag = GetBaseEntryTag(nvBlob.tag);
    u32 blobIndex = nvBlob.tag - baseTag;
    u32 blobSize = (u32)nvBlob.Size();
    u32 startWriteIndex = blobIndex *_kMaxNvStorageBlobSize;
    u32 newTotalEntrySize = startWriteIndex + blobSize;

    std::vector<u8>* vec = _recvDataMap[(NVStorage::NVEntryTag)baseTag].data;
    if (vec->size() < newTotalEntrySize) {
      vec->resize(newTotalEntrySize);
    }
    
    // Store data
    std::copy(nvBlob.blob.begin(), nvBlob.blob.end(), vec->begin() + startWriteIndex);
    
  } else {
    
    // If this is not a multiblob message, execute callback now.
    // TODO: Or should I expect a NvAllBlobsSent message even for single blob messages?
    
    PRINT_NAMED_INFO("NVStorageComponent.HandleNVData.MsgRecvd", "Executing callback");
    _recvDataMap[(NVStorage::NVEntryTag)nvBlob.tag].callback(nvBlob.blob.data(), nvBlob.blob.size());
  }
  
  
  /*
  switch((NVStorage::NVEntryTag)nvBlob.tag)
  {
    case NVStorage::NVEntryTag::NVEntry_CameraCalibration:
    {
      PRINT_NAMED_INFO("Robot.HandleNVData.CameraCalibration","");
      
      CameraCalibration payload;
      payload.Unpack(nvBlob.blob.data(), nvBlob.blob.size());
      PRINT_NAMED_INFO("RobotMessageHandler.CameraCalibration",
                       "Received new %dx%d camera calibration from robot.", payload.ncols, payload.nrows);
      
      // Convert calibration message into a calibration object to pass to the robot
      Vision::CameraCalibration calib(payload.nrows,
                                      payload.ncols,
                                      payload.focalLength_x,
                                      payload.focalLength_y,
                                      payload.center_x,
                                      payload.center_y,
                                      payload.skew);
      
      _robot.GetVisionComponent().SetCameraCalibration(calib);
      _robot.SetPhysicalRobot(payload.isPhysicalRobots);
      break;
    }
    case NVStorage::NVEntryTag::NVEntry_APConfig:
    case NVStorage::NVEntryTag::NVEntry_StaConfig:
    case NVStorage::NVEntryTag::NVEntry_SDKModeUnlocked:
    case NVStorage::NVEntryTag::NVEntry_Invalid:
      PRINT_NAMED_WARNING("Robot.HandleNVData.UnhandledTag", "TODO: Implement handler for tag %d", nvBlob.tag);
      break;
    default:
      PRINT_NAMED_WARNING("Robot.HandleNVData.InvalidTag", "Invalid tag %d", nvBlob.tag);
      break;
  }
   */
  
  
}

void NVStorageComponent::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  NVStorage::NVOpResult payload = message.GetData().Get_nvResult();
  PRINT_NAMED_INFO("Robot.HandleNVOpResult","Tag: %d, Result: %d, write: %d", payload.tag, (int)payload.result, payload.write);
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
