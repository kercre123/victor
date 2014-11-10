/**
 * File: uiTcpComms.h
 *
 * Author: Kevin Yoon
 * Created: 7/11/2014
 *
 * Description: Interface class to allow the basestation
 * to accept connections from UI client devices on TCP
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_COMMS_UI_TCPCOMMS_H_
#define BASESTATION_COMMS_UI_TCPCOMMS_H_

#include <deque>
#include <anki/messaging/basestation/IComms.h>
#include "anki/messaging/shared/TcpServer.h"
#include "anki/cozmo/robot/cozmoConfig.h"


namespace Anki {
namespace Cozmo {

  
  class UiTCPComms : public Comms::IComms {
  public:
    
    // Default constructor
    UiTCPComms();
    
    // The destructor will automatically cleans up
    virtual ~UiTCPComms();
    
    // Returns true if we are ready to use TCP
    virtual bool IsInitialized();
    
    // Returns 0 if no messages are available.
    virtual u32 GetNumPendingMsgPackets();
  
    virtual size_t Send(const Comms::MsgPacket &p);

    virtual bool GetNextMsgPacket(Comms::MsgPacket &p);
    
    
    // when game is unpaused we need to dump old messages
    virtual void ClearMsgPackets();
    
    // Updates the list of advertising robots
    void Update();
    
    bool HasClient();

    
  private:
    
    TcpServer server_;
    
    void ReadAllMsgPackets();
    
    void PrintRecvBuf();
    
    // 'Queue' of received messages from all connected user devices with their received times.
    std::deque<Comms::MsgPacket> recvdMsgPackets_;

    bool isInitialized_;
    
    static const int MAX_RECV_BUF_SIZE = 1920000;
    u8 recvBuf[MAX_RECV_BUF_SIZE];
    int recvDataSize = 0;
    
  };

}  // namespace Cozmo
}  // namespace Anki

#endif  // #ifndef BASESTATION_COMMS_TCPCOMMS_H_

