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
#include "bleComms.h"
#include "anki/cozmo/shared/cozmoConfig.h"
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


#define DEBUG_BLECOMMS 0

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
  

  // Copy header and data into send buffer
  u8 sendBufSize = sizeof(RADIO_PACKET_HEADER) + 4 + p.dataLen;
  u8 sendBuf[sendBufSize];
  sendBuf[0] = RADIO_PACKET_HEADER[0];
  sendBuf[1] = RADIO_PACKET_HEADER[1];
  sendBuf[2] = p.dataLen;
  sendBuf[3] = 0;
  sendBuf[4] = 0;
  sendBuf[5] = 0;
  memcpy(sendBuf+6, p.data, p.dataLen);
  
  
  // Compute number of BLE packets required to send this message
  int numPackets = sendBufSize / MAX_BLE_MSG_SIZE;
  if (MAX_BLE_MSG_SIZE*numPackets < sendBufSize) {
    numPackets += 1;
  }
  int dataByteProgress = 0;
  
  
  // Send msg
  for (int i=0; i<numPackets; ++i) {
    if ( ![vehicleConn writeData:[NSData dataWithBytes:(sendBuf+dataByteProgress) length:MIN(sendBufSize-dataByteProgress,MAX_BLE_MSG_SIZE)] error:&error] ) {
      DASInfo("BTLE.SendError", "vehicleID=%d (0x%llx) error=%s", p.destId, vehicleConn.mfgID, [error.description UTF8String]);
      return Comms::ICOMMS_SEND_FAILED_ERROR;
    }
    
#if(DEBUG_BLECOMMS)
    int sentBytesThisCycle = MIN(sendBufSize-dataByteProgress,MAX_BLE_MSG_SIZE);
    printf("Packet data sent (%u): 0x", sentBytesThisCycle);
    for (int i=0; i < sentBytesThisCycle; ++i){
      printf("%02x", sendBuf[dataByteProgress + i]);
    }
    printf("\n");
#endif
    
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
  const size_t HEADER_AND_TS_SIZE = sizeof(RADIO_PACKET_HEADER) + sizeof(TimeStamp_t); // BEEF + TimeStamp
  const size_t DATALEN_SIZE = 4;
  
  // While there are BLE packets, pull them into the msgQueue
  while ([[BLEVehicleManager sharedInstance] vehicleMessages].count > 0) {
    BLEVehicleMessage *vehicleMsg = [[[BLEVehicleManager sharedInstance] vehicleMessages] objectAtIndex:0];
    DASAssert(vehicleMsg != nil);

    u8 *origMsgBuffer = (u8*) [vehicleMsg.msgData bytes];

    // Check for header
    bool hasHeader = (origMsgBuffer[0] == RADIO_PACKET_HEADER[0]) && (origMsgBuffer[1] == RADIO_PACKET_HEADER[1]);
    
    // If currMessageSize == 0, we're expecting the start of a message.
    // If this is the start of a message (based on appearance of header) then start a new message,
    // even if it wasn't expected!
    u8 startReadIdx = 0;
    if (currMessageSize_ == 0 || hasHeader) {
      DASAssert(vehicleMsg.msgData.length == MAX_BLE_MSG_SIZE);
      
      // No header found! That packet must've been dropped
      // so send just ignore packets until we see one with a header.
      if (!hasHeader) {
        printf("WARN (bleComms): Expected BEEF packet, but got something else. Ignoring packet.\n");
        [[[BLEVehicleManager sharedInstance] vehicleMessages] removeObjectAtIndex:0];
        continue;
      }

      // Header found, but we were in the middle of reconstituting a message.
      // Nothing to do, but ignore the message we were building and start a new one.
      if (hasHeader && currMessageSize_ != 0) {
        printf("WARN (bleComms): partial msg received. Dropping.\n");
      }
      
      currMsgPacket_.sourceId = (int)vehicleMsg.vehicleID;
      currMsgPacket_.dataLen = origMsgBuffer[HEADER_AND_TS_SIZE];
      //currMsgPacket_.timeStamp = currentTimeStamp_ - vehicleMsg.timestamp;
      currMsgPacket_.timeStamp = vehicleMsg.timestamp;
      startReadIdx = HEADER_AND_TS_SIZE + DATALEN_SIZE;
      
      // If message spans multiple BLE packets, then set currMessageSize_ to the expected size of the message
      currMessageRecvdBytes_ = 0;
      currMessageSize_ = 0;
      if (currMsgPacket_.dataLen > (MAX_BLE_MSG_SIZE - HEADER_AND_TS_SIZE - DATALEN_SIZE)) {
        currMessageSize_ = currMsgPacket_.dataLen;
      }
    
      #if(DEBUG_BLECOMMS)
      printf("Found BEEF (%lu), currMsgSize %u, currMsgPacket.dataLen %u\n", vehicleMsg.msgData.length, currMessageSize_, currMsgPacket_.dataLen);
      #endif
    }
    
    // The index of the last byte in this packet that is valid data. (Actually, the index + 1).
    u8 lastValidByteIdx = MIN(currMsgPacket_.dataLen - currMessageRecvdBytes_ + startReadIdx, MAX_BLE_MSG_SIZE);
    
    #if(DEBUG_BLECOMMS)
    printf("Packet data received (%d): 0x", lastValidByteIdx - startReadIdx);
    #endif

    // Copy all non-header data into currMsgPacket
    for (u8 i = startReadIdx; i < lastValidByteIdx; i++) {
      currMsgPacket_.data[currMessageRecvdBytes_ + i - startReadIdx] = origMsgBuffer[i];
      #if(DEBUG_BLECOMMS)
      printf("%02x", origMsgBuffer[i]);
      #endif
    }
    #if(DEBUG_BLECOMMS)
    printf("\n");
    #endif
    
    
    // Remove the message from the mutable array, releasing it and its contents
    [[[BLEVehicleManager sharedInstance] vehicleMessages] removeObjectAtIndex:0];
    deltaReceivedBytes_ += vehicleMsg.msgData.length;

    if (currMessageSize_ > 0) {
      currMessageRecvdBytes_ += lastValidByteIdx - startReadIdx;
      //printf("currMessageSize_ %d, currMessageRecvdBytes_ %d\n", currMessageSize_, currMessageRecvdBytes_);
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