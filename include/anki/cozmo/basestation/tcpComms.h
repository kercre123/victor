/**
 * File: tcpComms.h
 *
 * Author: Kevin Yoon
 * Created: 1/22/2014
 *
 * Description: Interface class to allow the basestation
 * to utilize TCP socket in place of BTLE.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_COMMS_TCPCOMMS_H_
#define BASESTATION_COMMS_TCPCOMMS_H_

#include <map>
#include <vector>
#include <deque>
#include <anki/messaging/basestation/IComms.h>
#include "anki/messaging/shared/TcpClient.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/robot/cozmoConfig.h"

// Set to 1 to simulate a send/receive latencies
// beyond the actual latency of TCP.
// Note that the resolution of these latencies is currently equal to
// the Basestation frequency since that's what defines how often Update() is called.
#define DO_SIM_COMMS_LATENCY 1
#define SIM_RECV_LATENCY_SEC 0 // 0.03
#define SIM_SEND_LATENCY_SEC 0 // 0.03



namespace Anki {
namespace Cozmo {

  typedef struct {
    RobotAdvertisement robotInfo;
    f32 lastSeenTime;
  } RobotConnectionInfo_t;
  
  
  class ConnectedRobotInfo {
  public:
    static const int MAX_RECV_BUF_SIZE = 1920000; // TODO: Shrink this?
#if(USE_UDP_ROBOT_COMMS)
    UdpClient* client;
#else
    TcpClient* client;
#endif
    u8 recvBuf[MAX_RECV_BUF_SIZE];
    int recvDataSize = 0;
  };
  
  
  
  class TCPComms : public Comms::IComms {
  public:
    
    // Default constructor
    TCPComms();
    
    // The destructor will automatically cleans up
    virtual ~TCPComms();
    
    // Returns true if we are ready to use TCP
    virtual bool IsInitialized();
    
    // Returns the number of messages ready for processing in the BLEVehicleMgr.
    // Returns 0 if no messages are available.
    virtual u32 GetNumPendingMsgPackets();
  
    virtual size_t Send(const Comms::MsgPacket &p);

    virtual bool GetNextMsgPacket(Comms::MsgPacket &p);
    
    
    // when game is unpaused we need to dump old messages
    virtual void ClearMsgPackets();
    
    //virtual void SetCurrentTimestamp(BaseStationTime_t timestamp);
  
    
    // Updates the list of advertising robots
    void Update();
    
    // Connect to a robot.
    // Returns true if successfully connected
    bool ConnectToRobotByID(int robotID);
    
    // Disconnect from a robot
    void DisconnectRobotByID(int robotID);
    
    // Connect to all advertising robots.
    // Returns the total number of robots that are connected.
    u32 ConnectToAllRobots();
    
    // Disconnects from all robots.
    void DisconnectAllRobots();
    
    u32 GetNumConnectedRobots() const { return (u32)connectedRobots_.size(); }
    
    u32 GetNumAdvertisingRobots() const { return (u32)advertisingRobots_.size(); }
    
    u32 GetAdvertisingRobotIDs(std::vector<int> &robotIDs);
    
    // Clears the list of advertising robots.
    void ClearAdvertisingRobots();

    
  private:
    
    // Connects to "advertising" server to view available unconnected robots.
    UdpClient advertisingChannelClient_;
    
    void ReadAllMsgPackets();
    
    void PrintRecvBuf(int robotID);
    
    // Map of advertising robots (key: robot id)
    using advertisingRobotsIt_t = std::map<int, RobotConnectionInfo_t>::iterator;
    std::map<int, RobotConnectionInfo_t> advertisingRobots_;
    
    // Map of connected robots (key: robot id)
    using connectedRobotsIt_t = std::map<int, ConnectedRobotInfo>::iterator;
    std::map<int, ConnectedRobotInfo> connectedRobots_;
    
    // 'Queue' of received messages from all connected robots with their received times.
    //std::multimap<TimeStamp_t, Comms::MsgPacket> recvdMsgPackets_;
    //std::deque<Comms::MsgPacket> recvdMsgPackets_;
    using PacketQueue_t = std::deque< std::pair<f32, Comms::MsgPacket> >;
    PacketQueue_t recvdMsgPackets_;
    
#if(DO_SIM_COMMS_LATENCY)
    // The number of messages that have been in recvdMsgPackets for at least
    // SIM_RECV_LATENCY_SEC and are now available for reading.
    u32 numRecvRdyMsgs_;
    
    // Queue of messages to be sent with the times they should be sent at
    PacketQueue_t sendMsgPackets_;

    // The actual function that does the sending when we're simulating latency
    int RealSend(const Comms::MsgPacket &p);
    
    // Outgoing bytes sent since last call to Update()
    int bytesSentThisUpdateCycle_;
#endif
    
  };

}  // namespace Cozmo
}  // namespace Anki

#endif  // #ifndef BASESTATION_COMMS_TCPCOMMS_H_

