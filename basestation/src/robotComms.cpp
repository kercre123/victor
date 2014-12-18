/**
 * File: robotComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 1/22/2014
 *
 * Description: Interface class to allow the basestation
 * to communicate with robots via TCP or UDP
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/cozmo/basestation/robotComms.h"

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/robot/cozmoConfig.h"

// The number of bytes that can be sent out per call to Update(),
// the assumption being Update() is called once per basestation tic.
#define MAX_SENT_BYTES_PER_TIC 100  // Roughly 5 * 20 BLE packets which is the "maximum" amount BLE can send per tic.


#define DEBUG_COMMS 0

namespace Anki {
namespace Cozmo {
  
  const size_t HEADER_SIZE = sizeof(RADIO_PACKET_HEADER);

  
  RobotComms::RobotComms(const char* advertisingHostIP, int advertisingPort)
  : advertisingHostIP_(advertisingHostIP)
  {
    advertisingChannelClient_.Connect(advertisingHostIP_, advertisingPort);
    
    // TODO: Should this be done inside the poorly named Connect()?
    advertisingChannelClient_.Send("1", 1);  // Send anything just to get recognized as a client for advertising service.
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
  }
  
  RobotComms::~RobotComms()
  {
    DisconnectAllRobots();
  }
 
  
  // Returns true if we are ready to use TCP
  bool RobotComms::IsInitialized()
  {
    return true;
  }
  
  size_t RobotComms::Send(const Comms::MsgPacket &p)
  {
    // TODO: Instead of sending immediately, maybe we should queue them and send them all at
    // once to more closely emulate BTLE.

    #if(DO_SIM_COMMS_LATENCY)
    // If no send latency, just send now
    if (SIM_SEND_LATENCY_SEC == 0) {
      if (bytesSentThisUpdateCycle_ + p.dataLen > MAX_SENT_BYTES_PER_TIC) {
        #if(DEBUG_COMMS)
        PRINT_NAMED_INFO("RobotComms.MaxSendLimitReached", "queueing message\n");
        #endif
      } else {
        bytesSentThisUpdateCycle_ += p.dataLen;
        return RealSend(p);
      }
    }
    
    // Otherwise add to send queue
    sendMsgPackets_.emplace_back(std::piecewise_construct,
                                 std::forward_as_tuple((f32)(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + SIM_SEND_LATENCY_SEC)),
                                 std::forward_as_tuple(p));
    
    // Fake the number of bytes sent
    size_t numBytesSent = sizeof(RADIO_PACKET_HEADER) + sizeof(u32) + p.dataLen;
    return numBytesSent;
  }
  
  int RobotComms::RealSend(const Comms::MsgPacket &p)
  {
    #endif // #if(DO_SIM_COMMS_LATENCY)
    
    connectedRobotsIt_t it = connectedRobots_.find(p.destId);
    if (it != connectedRobots_.end()) {
      
      // Wrap message in header/footer
      char sendBuf[128];
      int sendBufLen = 0;

      bool isTCP = it->second.protocol == Anki::Comms::TCP;
      
      if (isTCP) {
        memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
        sendBufLen += sizeof(RADIO_PACKET_HEADER);
        sendBuf[sendBufLen++] = p.dataLen;
        sendBuf[sendBufLen++] = p.dataLen >> 8;
        sendBuf[sendBufLen++] = 0;
        sendBuf[sendBufLen++] = 0;
      }

      memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
      sendBufLen += p.dataLen;

      /*
      printf("SENDBUF (hex): ");
      PrintBytesHex(sendBuf, sendBufLen);
      printf("\nSENDBUF (uint): ");
      PrintBytesUInt(sendBuf, sendBufLen);
      printf("\n");
      */
      
      //return it->second.client->Send(sendBuf, sendBufLen);
      
      if (isTCP) {
        return ((TcpClient*)it->second.client)->Send(sendBuf, sendBufLen);
      } else {
        return ((UdpClient*)it->second.client)->Send(sendBuf, sendBufLen);
      }
      
    }
    return -1;
    
  }
  
  
  void RobotComms::Update()
  {
    
    f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // Read datagrams and update advertising robots list.
    Comms::AdvertisementMsg advMsg;
    int bytes_recvd = 0;
    do {
      bytes_recvd = advertisingChannelClient_.Recv((char*)&advMsg, sizeof(advMsg));
      if (bytes_recvd == sizeof(advMsg)) {
        
        // Check if already connected to this robot.
        // Advertisement may have arrived right after connection.
        // If not already connected, add it to advertisement list.
        if (connectedRobots_.find(advMsg.id) == connectedRobots_.end()) {
          
#if(DEBUG_COMMS)
          if (advertisingRobots_.find(advMsg.id) == advertisingRobots_.end()) {
            printf("Detected advertising robot %d on host %s at port %d\n", advMsg.id, advMsg.ip, advMsg.port);
          }
#endif
          
          advertisingRobots_[advMsg.id].robotInfo = advMsg;
          advertisingRobots_[advMsg.id].lastSeenTime = currTime;
        }
      }
    } while(bytes_recvd > 0);
    
    
    
    // Remove robots from advertising list if they're already connected.
    advertisingRobotsIt_t it = advertisingRobots_.begin();
    while(it != advertisingRobots_.end()) {
      if (currTime - it->second.lastSeenTime > ROBOT_ADVERTISING_TIMEOUT_S) {
        #if(DEBUG_COMMS)
        printf("Removing robot %d from advertising list. (Last seen: %f, curr time: %f)\n", it->second.robotInfo.id, it->second.lastSeenTime, currTime);
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
    
    // Send messages that are scheduled to be sent, up to the outgoing bytes limit.
    bytesSentThisUpdateCycle_ = 0;
    while (!sendMsgPackets_.empty()) {
      if (sendMsgPackets_.front().first <= currTime) {
        
        if (bytesSentThisUpdateCycle_ + sendMsgPackets_.front().second.dataLen > MAX_SENT_BYTES_PER_TIC) {
          #if(DEBUG_COMMS)
          PRINT_NAMED_INFO("RobotComms.MaxSendLimitReached", "%d messages left in queue to send later\n", sendMsgPackets_.size() - 1);
          #endif
          break;
        }
        bytesSentThisUpdateCycle_ += sendMsgPackets_.front().second.dataLen;
        RealSend(sendMsgPackets_.front().second);
        sendMsgPackets_.pop_front();
      } else {
        break;
      }
    }
    #endif  // #if(DO_SIM_COMMS_LATENCY)
    
    static u8 pingTimer = 10;
    if (pingTimer-- == 0) {
      advertisingChannelClient_.Send("1",1);
      pingTimer = 10;
    }
  }
  
  
  void RobotComms::PrintRecvBuf(int robotID)
  {
    #if(DEBUG_COMMS)
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
  
  
  void RobotComms::ReadAllMsgPackets()
  {
    
    // Read from all connected clients.
    // Enqueue complete messages.
    connectedRobotsIt_t it = connectedRobots_.begin();
    while ( it != connectedRobots_.end() ) {
      
      ConnectedRobotInfo &c = it->second;
      bool isTCP = c.protocol == Anki::Comms::TCP;

      while(1) { // Keep reading socket until no bytes available
      
//        int bytes_recvd = c.client->Recv((char*)c.recvBuf + c.recvDataSize,
//                                         ConnectedRobotInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);

        
        int bytes_recvd = 0;
        if (isTCP) {
          bytes_recvd = ((TcpClient*)c.client)->Recv((char*)c.recvBuf + c.recvDataSize,
                                                     ConnectedRobotInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);
        } else {
          bytes_recvd = ((UdpClient*)c.client)->Recv((char*)c.recvBuf + c.recvDataSize,
                                                     ConnectedRobotInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);
        }
        
        
        
        if (bytes_recvd == 0) {
          it++;
          break;
        }
        if (bytes_recvd < 0) {
          // Disconnect client
          #if(DEBUG_COMMS)
          printf("TcpRobotMgr: Recv failed. Disconnecting client\n");
          #endif
//          c.client->Disconnect();
//          delete c.client;
          
          if (isTCP) {
            ((TcpClient*)c.client)->Disconnect();
            delete (TcpClient*)(c.client);
          } else {
            ((UdpClient*)c.client)->Disconnect();
            delete (UdpClient*)(c.client);
          }
          
          connectedRobots_.erase(it++);
          break;
        }

        if (isTCP) {
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
              PRINT_NAMED_WARNING("RobotComms.PartialMsgRecvd", "Header not found where expected. Dropping preceding %d bytes\n", n);
              c.recvDataSize -= n;
              memcpy(c.recvBuf, hPtr, c.recvDataSize);
            }
            
            // Check if expected number of bytes are in the msg
            if (c.recvDataSize > HEADER_SIZE) {
              u32 dataLen = c.recvBuf[HEADER_SIZE] +
                                  (c.recvBuf[HEADER_SIZE + 1] << 8) +
                                  (c.recvBuf[HEADER_SIZE + 2] << 16) +
                                  (c.recvBuf[HEADER_SIZE + 3] << 24);
              
              if (c.recvDataSize >= HEADER_SIZE + 4 + dataLen) {
                
                f32 recvTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                
                #if(DO_SIM_COMMS_LATENCY)
                recvTime += SIM_RECV_LATENCY_SEC;
                #endif
                
                recvdMsgPackets_.emplace_back(std::piecewise_construct,
                                              std::forward_as_tuple(recvTime),
                                              std::forward_as_tuple((s32)(it->first),
                                                                    (s32)-1,
                                                                    dataLen,
                                                                    (u8*)(&c.recvBuf[HEADER_SIZE+4]),
                                                                    BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds())
                                              );
                
                // Shift recvBuf contents down
                const u16 entireMsgSize = HEADER_SIZE + 4 + dataLen;
                memcpy(c.recvBuf, c.recvBuf + entireMsgSize, c.recvDataSize - entireMsgSize);
                c.recvDataSize -= entireMsgSize;
                
              } else {
                break;
              }
            } else {
              break;
            }
          
          } // end while (there are still messages in the recvBuf)
          
        } else { // if (useTCP)

          c.recvDataSize = bytes_recvd;
          
          f32 recvTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          
          #if(DO_SIM_COMMS_LATENCY)
          recvTime += SIM_RECV_LATENCY_SEC;
          #endif
          
          recvdMsgPackets_.emplace_back(std::piecewise_construct,
                                        std::forward_as_tuple(recvTime),
                                        std::forward_as_tuple((s32)(it->first),
                                                              (s32)-1,
                                                              c.recvDataSize,
                                                              (u8*)(c.recvBuf),
                                                              BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds())
                                        );
          
          c.recvDataSize = 0;
        }
        
        
      } // end while(1) // keep reading socket until no bytes
      
    } // end for (each robot)
  }
  
  
  bool RobotComms::ConnectToRobotByID(int robotID)
  {
    // Check if already connected
    if (connectedRobots_.find(robotID) != connectedRobots_.end()) {
      return true;
    }
    
    // Check if the robot is available to connect to
    advertisingRobotsIt_t it = advertisingRobots_.find(robotID);
    if (it != advertisingRobots_.end()) {
      
      /*
#if(USE_UDP_ROBOT_COMMS)
      UdpClient *client = new UdpClient();
#else
      TcpClient *client = new TcpClient();
#endif
       */
      
      bool isTCP = it->second.robotInfo.protocol == Anki::Comms::TCP;
      
      void* client;
      if(isTCP) {
        client = (void*)(new TcpClient());
      } else {
        client = (void*)(new UdpClient());
      }
      
//      if (client->Connect((char*)it->second.robotInfo.ip, it->second.robotInfo.port)) {
      if ( (isTCP  && ((TcpClient*)client)->Connect((char*)it->second.robotInfo.ip, it->second.robotInfo.port)) ||
          (!isTCP && ((UdpClient*)client)->Connect((char*)it->second.robotInfo.ip, it->second.robotInfo.port)) ){
        #if(DEBUG_COMMS)
        printf("Connected to robot %d at %s:%d\n", it->second.robotInfo.id, it->second.robotInfo.ip, it->second.robotInfo.port);
        #endif
        connectedRobots_[robotID].client = client;
        connectedRobots_[robotID].protocol = it->second.robotInfo.protocol;
        
        // Remove from advertising list
        advertisingRobots_.erase(it);
        
        return true;
      }
    }
    
    return false;
  }
  
  void RobotComms::DisconnectRobotByID(int robotID)
  {
    connectedRobotsIt_t it = connectedRobots_.find(robotID);
    if (it != connectedRobots_.end()) {
//      it->second.client->Disconnect();
//      delete it->second.client;
      
      if (it->second.protocol == Anki::Comms::TCP) {
        ((TcpClient*)it->second.client)->Disconnect();
        delete (TcpClient*)(it->second.client);
      } else {
        ((UdpClient*)it->second.client)->Disconnect();
        delete (UdpClient*)(it->second.client);
      }
      
      connectedRobots_.erase(it);
    }
  }
  
  
  u32 RobotComms::ConnectToAllRobots()
  {
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      ConnectToRobotByID(it->first);
    }
    
    return (u32)connectedRobots_.size();
  }
  
  u32 RobotComms::GetAdvertisingRobotIDs(std::vector<int> &robotIDs)
  {
    robotIDs.clear();
    for (advertisingRobotsIt_t it = advertisingRobots_.begin(); it != advertisingRobots_.end(); it++)
    {
      robotIDs.push_back(it->first);
    }
    
    return (u32)robotIDs.size();
  }
  
  
  void RobotComms::ClearAdvertisingRobots()
  {
    advertisingRobots_.clear();
  }
  
  void RobotComms::DisconnectAllRobots()
  {
    for(connectedRobotsIt_t it = connectedRobots_.begin(); it != connectedRobots_.end();) {
//      it->second.client->Disconnect();
//      delete it->second.client;
      
      if (it->second.protocol == Anki::Comms::TCP) {
        ((TcpClient*)it->second.client)->Disconnect();
        delete (TcpClient*)(it->second.client);
      } else {
        ((UdpClient*)it->second.client)->Disconnect();
        delete (UdpClient*)(it->second.client);
      }
      
      it = connectedRobots_.erase(it);
    }
    
    connectedRobots_.clear();
  }
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool RobotComms::GetNextMsgPacket(Comms::MsgPacket& p)
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
  
  
  u32 RobotComms::GetNumPendingMsgPackets()
  {
    #if(DO_SIM_COMMS_LATENCY)
    return numRecvRdyMsgs_;
    #else
    return (u32)recvdMsgPackets_.size();
    #endif
  };
  
  void RobotComms::ClearMsgPackets()
  {
    recvdMsgPackets_.clear();
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
  };
  
  
  
}  // namespace Cozmo
}  // namespace Anki


