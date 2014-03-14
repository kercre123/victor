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
#include <anki/messaging/basestation/IComms.h>
#include "anki/messaging/shared/TcpClient.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/cozmo/robot/cozmoConfig.h"


namespace Anki {
namespace Cozmo {

  typedef struct {
    RobotAdvertisement robotInfo;
    f32 lastSeenTime;
  } RobotConnectionInfo_t;
  
  
  class ConnectedRobotInfo {
  public:
    static const int MAX_RECV_BUF_SIZE = 1024;
    TcpClient* client;
    char recvBuf[MAX_RECV_BUF_SIZE];
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
    
    // Returns the number of messages ready for processing in the BLEVehicleMgr. Returns 0 if no messages are available.
    virtual int GetNumPendingMsgPackets() { return recvdMsgPackets_.size(); };
  
    virtual int Send(const Comms::MsgPacket &p);

    virtual bool GetNextMsgPacket(Comms::MsgPacket &p);
    
    
    // when game is unpaused we need to dump old messages
    virtual void ClearMsgPackets() { recvdMsgPackets_.clear(); };
    
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
    int ConnectToAllRobots();
    
    // Disconnects from all robots.
    void DisconnectAllRobots();
    
    int GetNumConnectedRobots() const { return connectedRobots_.size(); }
    
    int GetNumAdvertisingRobots() const { return advertisingRobots_.size(); }
    
    int GetAdvertisingRobotIDs(std::vector<int> &robotIDs);
    
    // Clears the list of advertising robots.
    void ClearAdvertisingRobots();

    
  private:
    
    // Connects to "advertising" server to view available unconnected robots.
    UdpClient advertisingChannelClient_;
    
    void ReadAllMsgPackets();
    
    void PrintRecvBuf(int robotID);
    
    // Map of advertising robots (key: robot id)
    typedef std::map<int, RobotConnectionInfo_t>::iterator advertisingRobotsIt_t;
    std::map<int, RobotConnectionInfo_t> advertisingRobots_;
    
    // Map of connected robots (key: robot id)
    typedef std::map<int, ConnectedRobotInfo>::iterator connectedRobotsIt_t;
    std::map<int, ConnectedRobotInfo> connectedRobots_;
    
    // 'Queue' of received messages from all connected robots
    std::multimap<TimeStamp_t, Comms::MsgPacket> recvdMsgPackets_;
    
  };

}  // namespace Cozmo
}  // namespace Anki

#endif  // #ifndef BASESTATION_COMMS_TCPCOMMS_H_

