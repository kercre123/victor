/**
 * File: bleComms.mm
 *
 * Author: Mark Palatucci (markmp)
 * Created: 7/22/2012

 * Description: Interface class to allow the basestation
 * to utilize Bluetooth low-energy. This is an Objective-C++ class,
 * meaning it's a C++ class that will make objective-C calls.
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#include <string.h>
#include "anki/cozmo/basestation/bleComms.h"
//#import <BaseStation/baseStation.h>
//#import <BLEManager/BLEVehicleManager.h>
#import "BLEManager/BLEVehicleManager.h"
#import <Foundation/Foundation.h>

// TODO: Import DAS.
//       For now, just stubbed out DAS calls
#if(0)
#import <DAS/DAS.h>
#else
#define DASAssert(x) {if (!(x)) {printf("DAS-ASSERT line: %d\n", __LINE__); exit(-1);}}
#define DASInfo(...)
#define DASEvent(...)
#endif


// The maximum number of packets that can be sent per call to Update()
#define MAX_SENT_BLE_PACKETS_PER_TIC 5



namespace Anki {
namespace Cozmo {

// Default constructor
BLEComms::BLEComms()
{
  deltaSentMessages_ = 0;
  deltaReceivedMessages_ = 0;
  deltaSentBytes_ = 0;
  deltaReceivedMessages_ = 0;
  currentTimeStamp_ = 0;
  
  currMessageSize_ = 0;
  currMessageRecvdBytes_ = 0;
}


// The destructor will automatically cleanup
BLEComms::~BLEComms()
{

}


bool BLEComms::IsInitialized()
{
  if ([BLEVehicleManager sharedInstance] != nil)
    return true;
  else 
    return false;
}


u32 BLEComms::GetNumPendingMsgPackets()
{
  return msgQueue_.size();
}


//int BLEComms::Send(int vehicleID, unsigned char *sendBuffer, int numBytes)
size_t BLEComms::Send(const Comms::MsgPacket &p)
{
  // Compute number of BLE packets required to send this message
  int numPackets = p.dataLen / MAX_BLE_MSG_SIZE;
  if (MAX_BLE_MSG_SIZE*numPackets < p.dataLen) {
    numPackets += 1;
  }
  
  // Send now if outgoing packet limit not yet reached.
  // Otherwise queue for sending on next Update() call.
  if (numPacketsSentThisCycle_ + numPackets <= MAX_SENT_BLE_PACKETS_PER_TIC) {
    RealSend(p);
  } else {
    numPacketsSentThisCycle_ = MAX_SENT_BLE_PACKETS_PER_TIC;
    sendMsgQueue_.push_back(p);
  }
  
  return p.dataLen;
}
  
size_t BLEComms::RealSend(const Comms::MsgPacket &p)
{
  if (p.dataLen > MAX_BLE_MSG_SIZE) {
    return Comms::ICOMMS_MESSAGE_TOO_LARGE_ERROR; // We error if we can't send this whole buffer
  }

  if (p.destId < 0) {
    return Comms::ICOMMS_GENERAL_ERROR;
  }

  BLEVehicleConnection *vehicleConn = [[BLEVehicleManager sharedInstance] connectionForVehicleID:p.destId];
  if (vehicleConn == nil || vehicleConn.toCarCharacteristic == nil || vehicleConn.carPeripheral == nil)
    return Comms::ICOMMS_NOCONNECTION_TO_VEHICLE_ERROR;

  NSError *error = nil;

  // Compute number of BLE packets required to send this message
  int numPackets = p.dataLen / MAX_BLE_MSG_SIZE;
  if (MAX_BLE_MSG_SIZE*numPackets < p.dataLen) {
    numPackets += 1;
  }
  int dataByteProgress = 0;
  for (int i=0; i<numPackets; ++i) {
    if ( ![vehicleConn writeData:[NSData dataWithBytes:(p.data+dataByteProgress) length:MIN(p.dataLen-dataByteProgress,MAX_BLE_MSG_SIZE)] error:&error] ) {
      DASInfo("BTLE.SendError", "vehicleID=%d (0x%llx) error=%s", p.destId, vehicleConn.mfgID, [error.description UTF8String]);
      return Comms::ICOMMS_SEND_FAILED_ERROR;
    }
    dataByteProgress += MAX_BLE_MSG_SIZE;
  }

  numPacketsSentThisCycle_ += numPackets;
  deltaSentMessages_++;
  deltaSentBytes_ += p.dataLen;
  return p.dataLen;
}

  /*
void BLEComms::SetCurrentTimestamp(BaseStationTime_t timestamp) {
  currentTimeStamp_ = timestamp;
}
*/
  
//void BLEComms::GetNextMessage(unsigned char *bleBuffer, int &sourceVehicleID, unsigned char &totalPacketSize, BaseStationTime_t &messageTimeStamp)
bool BLEComms::GetNextMsgPacket(Comms::MsgPacket &p)
{
  DASAssert(msgQueue_.size() > 0);
  p = msgQueue_.front();
  msgQueue_.pop_front();
  return true;
}


// when game is unpaused we need to dump old messages
void BLEComms::ClearMsgPackets()
{
  if ([[BLEVehicleManager sharedInstance] vehicleMessages].count > 0)
    DASEvent("BTLE.ClearMessages", "Cleared car message count %lu", (unsigned long)[[BLEVehicleManager sharedInstance] vehicleMessages].count);
  [[[BLEVehicleManager sharedInstance] vehicleMessages] removeAllObjects];
}

  
void BLEComms::Update()
{
  // While there are BLE packets, pull them into the msgQueue
  while ([[BLEVehicleManager sharedInstance] vehicleMessages].count > 0) {
    BLEVehicleMessage *vehicleMsg = [[[BLEVehicleManager sharedInstance] vehicleMessages] objectAtIndex:0];
    DASAssert(vehicleMsg != nil);

    // If currMessageSize == 0, we're expecting the start of a message
    if (currMessageSize_ == 0) {
      currMsgPacket_.sourceId = (int)vehicleMsg.vehicleID;
      currMsgPacket_.dataLen = vehicleMsg.msgData.length;
      //currMsgPacket_.timeStamp = currentTimeStamp_ - vehicleMsg.timestamp;
      currMsgPacket_.timeStamp = vehicleMsg.timestamp;
      
      if (currMsgPacket_.dataLen > MAX_BLE_MSG_SIZE) {
        currMessageSize_ = currMsgPacket_.dataLen;
      }
    }
      
    unsigned char *origMsgBuffer = (unsigned char *) [vehicleMsg.msgData bytes];
    
    for (int i = 0; i < vehicleMsg.msgData.length; i++) {
      currMsgPacket_.data[currMessageRecvdBytes_ + i] = origMsgBuffer[i];
    }
    
    // Remove the message from the mutable array, releasing it and its contents
    [[[BLEVehicleManager sharedInstance] vehicleMessages] removeObjectAtIndex:0];
    deltaReceivedBytes_ += vehicleMsg.msgData.length;

    if (currMessageSize_ > 0) {
      currMessageRecvdBytes_ += vehicleMsg.msgData.length;
      DASAssert(currMessageSize_ >= currMessageRecvdBytes_);
      if (currMessageSize_ == currMessageRecvdBytes_) {
        currMessageSize_ = 0;
        currMessageRecvdBytes_ = 0;
        deltaReceivedMessages_++;
        msgQueue_.push_back(currMsgPacket_);
      }
    } else {
      msgQueue_.push_back(currMsgPacket_);
    }
  }
  
  
  // Reset outgoing packet counter and send outgoing messages in queue.
  numPacketsSentThisCycle_ = 0;
  
  while (!sendMsgQueue_.empty()) {
    
    // Compute number of BLE packets required to send this message
    Comms::MsgPacket p = sendMsgQueue_.front();
    int numPackets = p.dataLen / MAX_BLE_MSG_SIZE;
    if (MAX_BLE_MSG_SIZE*numPackets < p.dataLen) {
      numPackets += 1;
    }
    
    // Send packets for this message as long as the outgoing packet isn't exceeded
    if (numPacketsSentThisCycle_ + numPackets <= MAX_SENT_BLE_PACKETS_PER_TIC) {
      size_t sentBytes = RealSend(p);
      if (sentBytes != p.dataLen) {
        DASEvent("BLEComms.Update.RealSend", "bytesToSend %d, sentBytes: %d\n", p.dataLen, sentBytes);
      }
      sendMsgQueue_.pop_front();
    } else {
      numPacketsSentThisCycle_ = MAX_SENT_BLE_PACKETS_PER_TIC;
      break;
    }
  }
  
}
  
}  // namespace Comms
}  // namespace Anki