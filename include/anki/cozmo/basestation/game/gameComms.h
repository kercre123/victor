/**
 * File: gameComms.h
 *
 * Author: Kevin Yoon
 * Created: 12/16/2014
 *
 * Description: Interface class to allow UI to communicate with game
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef BASESTATION_COMMS_GAME_COMMS_H_
#define BASESTATION_COMMS_GAME_COMMS_H_

#include <deque>
#include <anki/messaging/basestation/IComms.h>
#include "anki/messaging/basestation/advertisementService.h"
#include "anki/messaging/shared/TcpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/robot/cozmoConfig.h"


namespace Anki {
namespace Cozmo {

  
  class GameComms : public Comms::IComms {
  public:
    
    // Default constructor
    GameComms(int serverListenPort, const char* advertisementRegIP_, int advertisementRegPort_);
    
    // The destructor will automatically cleans up
    virtual ~GameComms();
    
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
    void DisconnectClient();
    
  private:
    
    // For connection from game
    TcpServer server_;
    
    // For connecting to advertisement service
    UdpClient regClient_;
    Comms::AdvertisementRegistrationMsg regMsg_;
    void RegisterWithAdvertisementService();
    void DeregisterFromAdvertisementService();
    bool IsRegistered();
    
    
    void ReadAllMsgPackets();
    
    void PrintRecvBuf();
    
    const char* const GetLocalIP();
    
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

