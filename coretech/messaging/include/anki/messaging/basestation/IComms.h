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
#include "anki/common/types.h"

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
      MsgPacket(const s32 sourceId, const s32 destId, const u8 dataLen, const u8* data, const BaseStationTime_t timestamp = 0) :
      dataLen(dataLen), sourceId(sourceId), destId(destId), timeStamp(timestamp) {
        memcpy(this->data, data, dataLen);
      }
      
      u8 data[256];
      u8 dataLen = 0;
      s32 sourceId = -1;
      s32 destId = -1;
      BaseStationTime_t timeStamp = 0;
    };
      
    class IComms {
    public:

      virtual ~IComms() {};

      // Returns true if we are ready to use BLE
      virtual bool IsInitialized() = 0;

      // Returns the number of messages ready for processing in the BLEVehicleMgr. Returns 0 if no
      // messages are available.
      virtual u32 GetNumPendingMsgPackets() = 0;
      
      virtual size_t Send(const MsgPacket &p) = 0;
      
      virtual bool GetNextMsgPacket(MsgPacket &p) = 0;
      
      // virtual void SetCurrentTimestamp(BaseStationTime_t timestamp) = 0;

      // when game is unpaused we need to dump old messages
      virtual void ClearMsgPackets() = 0;

    };
  }
}

#endif /* defined(__IComms__) */
