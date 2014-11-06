/**
 * File: bleComms.h
 *
 * Author: Mark Palatucci (markmp)
 * Created: 7/22/2012
 *
 * Description: Interface class to allow the basestation
 * to utilize Bluetooth low-energy. This is an Objective-C++ class,
 * meaning it's a C++ class that will make objective-C calls.
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifndef BASESTATION_COMMS_BLECOMMS_H_
#define BASESTATION_COMMS_BLECOMMS_H_

#include "anki/messaging/basestation/IComms.h"
#include <deque>

namespace Anki {
namespace Cozmo {


class BLEComms : public Comms::IComms {
public:

  // Default constructor
  BLEComms();

  // The destructor will automatically cleans up
  virtual ~BLEComms();
  
  // Returns true if we are ready to use BLE
  bool IsInitialized();
  
  // Returns the number of messages ready for processing in the BLEVehicleManager. Returns 0 if no
  // messages are available.
  u32 GetNumPendingMsgPackets();

  // Send up to kMaxSendBufferSize bytes to a given vehicle
  size_t Send(const Comms::MsgPacket &p);
  
  // Gets (and removes) the next message from the BLEVehicleManager. This will copy the data into the 
  // bleBuffer, which must be at least kMaxSendBufferSize (20) bytes long. It will also return the
  // vehicleID of the source, and the totalPacketSize. Note this is different then the msgSize field 
  // that is the first byte of the packet.
  bool GetNextMsgPacket(Comms::MsgPacket &p);

  // when game is unpaused we need to dump old messages
  virtual void ClearMsgPackets();

  //void SetCurrentTimestamp(BaseStationTime_t timestamp);
  
  void Update();
  
private:
  unsigned int deltaSentMessages_;
  unsigned int deltaReceivedMessages_;
  unsigned int deltaSentBytes_;
  unsigned int deltaReceivedBytes_;
  BaseStationTime_t currentTimeStamp_;
  
  // Current message that's being received.
  // Could be a multi-packet message.
  unsigned int currMessageSize_;
  unsigned int currMessageRecvdBytes_;
  Comms::MsgPacket currMsgPacket_;
  
  // Received messages
  std::deque<Comms::MsgPacket> msgQueue_;
  
  // Message to be sent
  std::deque<Comms::MsgPacket> sendMsgQueue_;
  
  // Number of BLE packets sent since the last call to Update()
  unsigned int numPacketsSentThisCycle_;
  
  size_t RealSend(const Comms::MsgPacket &p);
};

}  // namespace Comms
}  // namespace Anki
#endif  // #ifndef BASESTATION_COMMS_BLECOMMS_H_

