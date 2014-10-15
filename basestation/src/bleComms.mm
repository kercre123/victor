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
  NSInteger numPendingMessages = [[[BLEVehicleManager sharedInstance] vehicleMessages] count];
  return (int)numPendingMessages;
}


//int BLEComms::Send(int vehicleID, unsigned char *sendBuffer, int numBytes)
size_t BLEComms::Send(const Comms::MsgPacket &p)
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
  if ( ![vehicleConn writeData:[NSData dataWithBytes:p.data length:p.dataLen] error:&error] ) {
    DASInfo("BTLE.SendError", "vehicleID=%d (0x%llx) error=%s", p.destId, vehicleConn.mfgID, [error.description UTF8String]);
    return Comms::ICOMMS_SEND_FAILED_ERROR;
  }
  
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
  DASAssert([[BLEVehicleManager sharedInstance] vehicleMessages].count > 0);
  BLEVehicleMessage *vehicleMsg = [[[BLEVehicleManager sharedInstance] vehicleMessages] objectAtIndex:0];
  DASAssert(vehicleMsg != nil);
  
  p.sourceId = (int)vehicleMsg.vehicleID;
  p.dataLen = vehicleMsg.msgData.length;
  p.timeStamp = currentTimeStamp_ - vehicleMsg.timestamp;
  DASAssert(p.dataLen >= 0 && p.dataLen <= MAX_BLE_MSG_SIZE);
  unsigned char *origMsgBuffer = (unsigned char *) [vehicleMsg.msgData bytes];
  
  // Zero out buffer and copy msgs
  bzero(p.data,MAX_BLE_MSG_SIZE);
  
  for (int i = 0; i < p.dataLen; i++) {
    p.data[i] = origMsgBuffer[i];
  }
  
  // Remove the message from the mutable array, releasing it and its contents
  [[[BLEVehicleManager sharedInstance] vehicleMessages] removeObjectAtIndex:0];
  deltaReceivedMessages_++;
  deltaReceivedBytes_ += vehicleMsg.msgData.length;
  
  return true;
}

// when game is unpaused we need to dump old messages
void BLEComms::ClearMsgPackets()
{
  if ([[BLEVehicleManager sharedInstance] vehicleMessages].count > 0)
    DASEvent("BTLE.ClearMessages", "Cleared car message count %lu", (unsigned long)[[BLEVehicleManager sharedInstance] vehicleMessages].count);
  [[[BLEVehicleManager sharedInstance] vehicleMessages] removeAllObjects];
}

}  // namespace Comms
}  // namespace Anki