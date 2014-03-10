/**
 * File: tcpComms.cpp
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

#include "anki/cozmo/basestation/tcpComms.h"
#include "anki/cozmo/basestation/utils/debug.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/common/basestation/utils/timer.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

namespace Anki {
namespace Cozmo {
  
  const std::string header(RADIO_PACKET_HEADER, RADIO_PACKET_HEADER + sizeof(RADIO_PACKET_HEADER));
  const std::string footer(RADIO_PACKET_FOOTER, RADIO_PACKET_FOOTER + sizeof(RADIO_PACKET_FOOTER));
  const int HEADER_AND_TS_SIZE = header.length() + sizeof(TimeStamp_t);
  const int FOOTER_SIZE = footer.length();
  

  
  TCPComms::TCPComms()
  {
    advertisingChannelClient_.Connect(ROBOT_SIM_WORLD_HOST, ROBOT_ADVERTISING_PORT);
    
    // TODO: Should this be done inside the poorly named Connect()?
    advertisingChannelClient_.Send("1", 1);  // Send anything just to get recognized as a client for advertising service.
  }
  
  TCPComms::~TCPComms()
  {
    DisconnectAllRobots();
  }
 
  
  // Returns true if we are ready to use TCP
  bool TCPComms::IsInitialized()
  {
    return true;
  }
  
  int TCPComms::Send(const Comms::MsgPacket &p)
  {
    // TODO: Instead of sending immediately, maybe we should queue them and send them all at
    // once to more closely emulate BTLE.

    
    connectedRobotsIt_t it = connectedRobots_.find(p.destId);
    if (it != connectedRobots_.end()) {
      
      // Wrap message in header/footer
      // TODO: Include timestamp too?
      char sendBuf[128];
      int sendBufLen = 0;
      memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
      sendBufLen += sizeof(RADIO_PACKET_HEADER);
      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
      sendBufLen += p.dataLen;
      memcpy(sendBuf + sendBufLen, RADIO_PACKET_FOOTER, sizeof(RADIO_PACKET_FOOTER));
      sendBufLen += sizeof(RADIO_PACKET_FOOTER);

      /*
      printf("SENDBUF (hex): ");
      PrintBytesHex(sendBuf, sendBufLen);
      printf("\nSENDBUF (uint): ");
      PrintBytesUInt(sendBuf, sendBufLen);
      printf("\n");
      */
      
      return it->second.client->Send(sendBuf, sendBufLen);
    }
    return -1;
    
  }
  
  
  void TCPComms::Update()
  {
    
    // Read datagrams and update advertising robots list.
    RobotAdvertisement advMsg;
    int bytes_recvd = 0;
    do {
      bytes_recvd = advertisingChannelClient_.Recv((char*)&advMsg, sizeof(advMsg));
      if (bytes_recvd == sizeof(advMsg)) {
        
#if(DEBUG_TCPCOMMS)
        if (advertisingRobots_.find(advMsg.robotID) == advertisingRobots_.end()) {
          printf("Detected advertising robot %d on host %s at port %d\n", advMsg.robotID, advMsg.robotAddr, advMsg.port);
        }
#endif
        
        advertisingRobots_[advMsg.robotID].robotInfo = advMsg;
        advertisingRobots_[advMsg.robotID].lastSeenTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    } while(bytes_recvd > 0);
    
    
    
    // Remove robots from advertising list if they're already connected.
    advertisingRobotsIt_t it = advertisingRobots_.begin();
    while(it != advertisingRobots_.end()) {
      if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - it->second.lastSeenTime > ROBOT_ADVERTISING_TIMEOUT_S) {
#if(DEBUG_TCPCOMMS)
        printf("Removing robot %d from advertising list. (Last seen: %f, curr time: %f)\n", it->second.robotInfo.robotID, it->second.lastSeenTime, BaseStationTimer::getInstance()->GetCurrentTimeInSeconds());
#endif
        advertisingRobots_.erase(it++);
      } else {
        ++it;
      }
    }
    
    // Read all messages from all connected robots
    ReadAllMsgPackets();
  }
  
  
  void TCPComms::PrintRecvBuf(int robotID)
  {
#if(DEBUG_TCPCOMMS)
    if (connectedRobots_.find(robotID) != connectedRobots_.end()) {
      int numBytes = connectedRobots_[robotID].recvDataSize;
      printf("Robot %d recv buffer (%d bytes): ", robotID, numBytes);
      for (int i=0; i<numBytes;i++){
        u8 t = connectedRobots_[robotID].recvBuf[i];
        printf("0x%x ", t);
      }
      printf("\n");

    }
#endif
  }
  
  
  void TCPComms::ReadAllMsgPackets()
  {
    
    // Read from all connected clients.
    // Enqueue complete messages.
    connectedRobotsIt_t it = connectedRobots_.begin();
    while ( it != connectedRobots_.end() ) {
      
      ConnectedRobotInfo &c = it->second;
      
      int bytes_recvd = c.client->Recv(c.recvBuf + c.recvDataSize,
                                       ConnectedRobotInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);
      if (bytes_recvd == 0) {
        it++;
        continue;
      }
      if (bytes_recvd < 0) {
        // Disconnect client
#if(DEBUG_TCPCOMMS)
        printf("TcpRobotMgr: Recv failed. Disconnecting client\n");
#endif
        c.client->Disconnect();
        delete c.client;
        connectedRobots_.erase(it++);
        continue;
      }
      c.recvDataSize += bytes_recvd;
      //PrintRecvBuf(it->first);
      
      
      // Look for valid header
      while (1) {
        std::string strBuf(c.recvBuf, c.recvBuf + c.recvDataSize);  // TODO: Just make recvBuf a string
        std::size_t n = strBuf.find(header);
        if (n == std::string::npos) {
          // Header not found at all
          // Delete everything
          c.recvDataSize = 0;
          break;
        } else if (n != 0) {
          // Header was not found at the beginning.
          // Delete everything up until the header.
          strBuf = strBuf.substr(n);
          memcpy(c.recvBuf, strBuf.c_str(), strBuf.length());
          c.recvDataSize = strBuf.length();
        }
        
        
        // Look for footer
        n = strBuf.find(footer);
        if (n == std::string::npos) {
          // Footer not found at all
          break;
        } else {
          // Footer was found.
          // Move complete message to recvdMsgPackets
          
          // Get timestamp
          TimeStamp_t *ts = (TimeStamp_t*)&(c.recvBuf[header.length()]);
          
          // Create RobotMsgPacket
          Comms::MsgPacket p;
          p.sourceId = it->first;
          p.dataLen = n - HEADER_AND_TS_SIZE;
          memcpy(p.data, &c.recvBuf[HEADER_AND_TS_SIZE], p.dataLen);
          
          recvdMsgPackets_.insert(std::pair<TimeStamp_t,Comms::MsgPacket>(*ts, p) );
          
          // Shift recvBuf contents down
          memcpy(c.recvBuf, c.recvBuf + n + FOOTER_SIZE, c.recvDataSize - p.dataLen - HEADER_AND_TS_SIZE - FOOTER_SIZE);
          c.recvDataSize -= p.dataLen + HEADER_AND_TS_SIZE + FOOTER_SIZE;
          
          //PrintRecvBuf(it->first);
          
        }
      } // end while (there are still messages in the recvBuf)
      
      it++;
    } // end for (each robot)
  }
  
  
  bool TCPComms::ConnectToRobotByID(int robotID)
  {
    // Check if already connected
    if (connectedRobots_.find(robotID) != connectedRobots_.end()) {
      return true;
    }
    
    // Check if the robot is available to connect to
    advertisingRobotsIt_t it = advertisingRobots_.find(robotID);
    if (it != advertisingRobots_.end()) {
      
      TcpClient *client = new TcpClient();
      
      if (client->Connect((char*)it->second.robotInfo.robotAddr, it->second.robotInfo.port)) {
#if(DEBUG_TCPCOMMS)
        printf("Connected to robot %d at %s:%d\n", it->second.robotInfo.robotID, it->second.robotInfo.robotAddr, it->second.robotInfo.port);
#endif
        connectedRobots_[robotID].client = client;
        return true;
      }
    }
    
    return false;
  }
  
  void TCPComms::DisconnectRobotByID(int robotID)
  {
    connectedRobotsIt_t it = connectedRobots_.find(robotID);
    if (it != connectedRobots_.end()) {
      it->second.client->Disconnect();
      delete it->second.client;
      connectedRobots_.erase(it);
    }
  }
  
  
  int TCPComms::ConnectToAllRobots()
  {
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      ConnectToRobotByID(it->first);
    }
    
    return connectedRobots_.size();
  }
  
  int TCPComms::GetAdvertisingRobotIDs(vector<int> &robotIDs)
  {
    robotIDs.clear();
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      robotIDs.push_back(it->first);
    }
    
    return robotIDs.size();
  }
  
  
  void TCPComms::ClearAdvertisingRobots()
  {
    advertisingRobots_.clear();
  }
  
  void TCPComms::DisconnectAllRobots()
  {
    for(connectedRobotsIt_t it = connectedRobots_.begin(); it != connectedRobots_.end(); it++) {
      it->second.client->Disconnect();
      delete it->second.client;
      connectedRobots_.erase(it);
    }
    
    connectedRobots_.clear();
  }
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool TCPComms::GetNextMsgPacket(Comms::MsgPacket& p)
  {
    if (!recvdMsgPackets_.empty()) {
      p = recvdMsgPackets_.begin()->second;
      recvdMsgPackets_.erase(recvdMsgPackets_.begin());
      return true;
    }
    
    return false;
  }
  
}  // namespace Cozmo
}  // namespace Anki


