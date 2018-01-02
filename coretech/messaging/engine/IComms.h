//
//  IComms.h
//  RushHour
//
//  Created by Mark Pauley on 8/17/12.
//  Copyright (c) 2012 Anki, Inc. All rights reserved.
//

#ifndef __IComms__
#define __IComms__

#include <string.h>
#include "coretech/common/shared/types.h"
#include "coretech/common/engine/exceptions.h"
#include <vector>

namespace Anki {
  namespace Comms {

    // The max size in one BLE packet
    #define MAX_BLE_MSG_SIZE        20
    #define MIN_BLE_MSG_SIZE        2

    enum ICommsSendErrorCode {
      ICOMMS_GENERAL_ERROR = -1,
      ICOMMS_NOCONNECTION_TO_VEHICLE_ERROR = -2,
      ICOMMS_MESSAGE_TOO_LARGE_ERROR = -3,
      ICOMMS_SEND_FAILED_ERROR = -4,
    };

      

    class MsgPacket {
    public:
      MsgPacket(){};
      MsgPacket(const s32 sourceId, const s32 destId, const u16 dataLen, const u8* data, double timestamp = 0.0) :
      dataLen(dataLen), sourceId(sourceId), destId(destId), timeStamp_s(timestamp) {
        CopyFrom(dataLen, data);
      }
      
      void CopyFrom(const u16 inDataLen, const u8* inData)
      {
        CORETECH_ASSERT(inDataLen <= MAX_SIZE);
        dataLen = inDataLen;
        memcpy(data, inData, inDataLen);
      }
      
      static const int MAX_SIZE = 2048;
      
      // dataLen MUST be stored immediately before data so that we can send messages as [len][msg] without copying
      u16 dataLen = 0;
      u8 data[MAX_SIZE];
      
      s32 sourceId = -1;
      s32 destId = -1;
      double timeStamp_s = 0.0;
    };
    static_assert(offsetof(MsgPacket, data) == (offsetof(MsgPacket, dataLen) + sizeof(MsgPacket::dataLen)), "Error len and data not contiguous!");
      
    class IComms {
    public:

      virtual ~IComms() {};

      // Returns true if we are ready to use BLE
      virtual bool IsInitialized() = 0;

      // Returns the number of messages ready for processing in the BLEVehicleMgr. Returns 0 if no
      // messages are available.
      virtual u32 GetNumPendingMsgPackets() = 0;
      
      virtual ssize_t Send(const MsgPacket &p) = 0;
      
      virtual bool GetNextMsgPacket(std::vector<uint8_t> &p) = 0;
      
      // virtual void SetCurrentTimestamp(BaseStationTime_t timestamp) = 0;

      // when game is unpaused we need to dump old messages
      virtual void ClearMsgPackets() = 0;

      // Return the number of MsgPackets in the send queue that are bound for devID
      virtual u32 GetNumMsgPacketsInSendQueue(int devID) = 0;
      
      virtual void Update(bool send_queued_msgs = true) = 0;
    };
  }
}

#endif /* defined(__IComms__) */
