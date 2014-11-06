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

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#define DEBUG_TCPCOMMS 0

namespace Anki {
namespace Cozmo {
  
  const size_t HEADER_AND_TS_SIZE = sizeof(RADIO_PACKET_HEADER) + sizeof(TimeStamp_t);

  
  TCPComms::TCPComms()
  {
    advertisingChannelClient_.Connect(ROBOT_SIM_WORLD_HOST, ROBOT_ADVERTISING_PORT);
    
    // TODO: Should this be done inside the poorly named Connect()?
    advertisingChannelClient_.Send("1", 1);  // Send anything just to get recognized as a client for advertising service.
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
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
  
  size_t TCPComms::Send(const Comms::MsgPacket &p)
  {
    // TODO: Instead of sending immediately, maybe we should queue them and send them all at
    // once to more closely emulate BTLE.

    #if(DO_SIM_COMMS_LATENCY)
    // If no send latency, just send now
    if (SIM_SEND_LATENCY_SEC == 0) {
      return RealSend(p);
    }
    
    // Otherwise add to send queue
    sendMsgPackets_.emplace_back(std::piecewise_construct,
                                 std::forward_as_tuple((f32)(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + SIM_SEND_LATENCY_SEC)),
                                 std::forward_as_tuple(p));
    
    // Fake the number of bytes sent
    size_t numBytesSent = sizeof(RADIO_PACKET_HEADER) + sizeof(u32) + p.dataLen;
    return numBytesSent;
  }
  
  int TCPComms::RealSend(const Comms::MsgPacket &p)
  {
    #endif // #if(DO_SIM_COMMS_LATENCY)
    
    connectedRobotsIt_t it = connectedRobots_.find(p.destId);
    if (it != connectedRobots_.end()) {
      
      // Wrap message in header/footer
      // TODO: Include timestamp too?
      char sendBuf[128];
      int sendBufLen = 0;
      memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
      sendBufLen += sizeof(RADIO_PACKET_HEADER);
      sendBuf[sendBufLen++] = p.dataLen;
      sendBuf[sendBufLen++] = 0;
      sendBuf[sendBufLen++] = 0;
      sendBuf[sendBufLen++] = 0;
      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
      sendBufLen += p.dataLen;

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
    
    f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
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
        advertisingRobots_[advMsg.robotID].lastSeenTime = currTime;
      }
    } while(bytes_recvd > 0);
    
    
    
    // Remove robots from advertising list if they're already connected.
    advertisingRobotsIt_t it = advertisingRobots_.begin();
    while(it != advertisingRobots_.end()) {
      if (currTime - it->second.lastSeenTime > ROBOT_ADVERTISING_TIMEOUT_S) {
        #if(DEBUG_TCPCOMMS)
        printf("Removing robot %d from advertising list. (Last seen: %f, curr time: %f)\n", it->second.robotInfo.robotID, it->second.lastSeenTime, currTime);
        #endif
        advertisingRobots_.erase(it++);
      } else {
        ++it;
      }
    }
    
    // Read all messages from all connected robots
    ReadAllMsgPackets();
    
    #if(DO_SIM_COMMS_LATENCY)
    // Update number of ready to receive messages
    numRecvRdyMsgs_ = 0;
    PacketQueue_t::iterator iter;
    for (iter = recvdMsgPackets_.begin(); iter != recvdMsgPackets_.end(); ++iter) {
      if (iter->first <= currTime) {
        ++numRecvRdyMsgs_;
      } else {
        break;
      }
    }
    
    //printf("TIME %f: Total: %d, rel: %d\n", currTime, recvdMsgPackets_.size(), numRecvRdyMsgs_);
    
    // Send messages that are scheduled to be sent
    while (!sendMsgPackets_.empty()) {
      if (sendMsgPackets_.front().first <= currTime) {
        RealSend(sendMsgPackets_.front().second);
        sendMsgPackets_.pop_front();
      } else {
        break;
      }
    }
    #endif  // #if(DO_SIM_COMMS_LATENCY)
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
      
      int bytes_recvd = c.client->Recv((char*)c.recvBuf + c.recvDataSize,
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
      while (c.recvDataSize >= sizeof(RADIO_PACKET_HEADER)) {
        
        // Look for 0xBEEF
        u8* hPtr = NULL;
        for(int i = 0; i < c.recvDataSize-1; ++i) {
          if (c.recvBuf[i] == RADIO_PACKET_HEADER[0]) {
            if (c.recvBuf[i+1] == RADIO_PACKET_HEADER[1]) {
              hPtr = &(c.recvBuf[i]);
              break;
            }
          }
        }
        
        if (hPtr == NULL) {
          // Header not found at all
          // Delete everything
          c.recvDataSize = 0;
          break;
        }
        
        int n = hPtr - c.recvBuf;
        if (n != 0) {
          // Header was not found at the beginning.
          // Delete everything up until the header.
          PRINT_NAMED_WARNING("TCPComms.PartialMsgRecvd", "Header not found where expected. Dropping preceding %d bytes\n", n);
          c.recvDataSize -= n;
          memcpy(c.recvBuf, hPtr, c.recvDataSize);
        }
        
        // Check if expected number of bytes are in the msg
        if (c.recvDataSize > HEADER_AND_TS_SIZE) {
          u32 dataLen = c.recvBuf[HEADER_AND_TS_SIZE] +
                              (c.recvBuf[HEADER_AND_TS_SIZE + 1] << 8) +
                              (c.recvBuf[HEADER_AND_TS_SIZE + 2] << 16) +
                              (c.recvBuf[HEADER_AND_TS_SIZE + 3] << 24);
          
          if (dataLen > 255) {
            PRINT_NAMED_WARNING("TCPComms.MsgTooBig", "Can't handle messages larger than 255 (dataLen = %d)\n", dataLen);
            dataLen = 255;
          }
          
          if (c.recvDataSize >= HEADER_AND_TS_SIZE + 4 + dataLen) {
            
            /*
             // Get timestamp
             TimeStamp_t *ts = (TimeStamp_t*)&(c.recvBuf[header.length()]);
             
             // Create RobotMsgPacket
             Comms::MsgPacket p;
             p.sourceId = it->first;
             p.dataLen = n - HEADER_AND_TS_SIZE;
             memcpy(p.data, &c.recvBuf[HEADER_AND_TS_SIZE], p.dataLen);
             recvdMsgPackets_.insert(std::pair<TimeStamp_t,Comms::MsgPacket>(*ts, p) );
             */
            
            f32 recvTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
            
            #if(DO_SIM_COMMS_LATENCY)
            recvTime += SIM_RECV_LATENCY_SEC;
            #endif
            
            recvdMsgPackets_.emplace_back(std::piecewise_construct,
                                          std::forward_as_tuple(recvTime),
                                          std::forward_as_tuple((s32)(it->first),
                                                                (s32)-1,
                                                                dataLen,
                                                                (u8*)(&c.recvBuf[HEADER_AND_TS_SIZE+4]),
                                                                BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds())
                                          );
            
            // Shift recvBuf contents down
            const u8 entireMsgSize = HEADER_AND_TS_SIZE + 4 + dataLen;
            memcpy(c.recvBuf, c.recvBuf + entireMsgSize, c.recvDataSize - entireMsgSize);
            c.recvDataSize -= entireMsgSize;
            
          } else {
            break;
          }
        } else {
          break;
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
  
  
  u32 TCPComms::ConnectToAllRobots()
  {
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      ConnectToRobotByID(it->first);
    }
    
    return (u32)connectedRobots_.size();
  }
  
  u32 TCPComms::GetAdvertisingRobotIDs(std::vector<int> &robotIDs)
  {
    robotIDs.clear();
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      robotIDs.push_back(it->first);
    }
    
    return (u32)robotIDs.size();
  }
  
  
  void TCPComms::ClearAdvertisingRobots()
  {
    advertisingRobots_.clear();
  }
  
  void TCPComms::DisconnectAllRobots()
  {
    for(connectedRobotsIt_t it = connectedRobots_.begin(); it != connectedRobots_.end();) {
      it->second.client->Disconnect();
      delete it->second.client;
      it = connectedRobots_.erase(it);
    }
    
    connectedRobots_.clear();
  }
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool TCPComms::GetNextMsgPacket(Comms::MsgPacket& p)
  {
    #if(DO_SIM_COMMS_LATENCY)
    if (numRecvRdyMsgs_ > 0) {
      --numRecvRdyMsgs_;
    #else
    if (!recvdMsgPackets_.empty()) {
    #endif
      p = recvdMsgPackets_.begin()->second;
      recvdMsgPackets_.pop_front();
      return true;
    }
    
    return false;
  }
  
  
  u32 TCPComms::GetNumPendingMsgPackets()
  {
    #if(DO_SIM_COMMS_LATENCY)
    return numRecvRdyMsgs_;
    #else
    return (u32)recvdMsgPackets_.size();
    #endif
  };
  
  void TCPComms::ClearMsgPackets()
  {
    recvdMsgPackets_.clear();
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
  };
  
  
  
}  // namespace Cozmo
}  // namespace Anki


