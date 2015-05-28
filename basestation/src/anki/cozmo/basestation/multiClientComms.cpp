/**
 * File: multiClientComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 1/22/2014
 *
 * Description: Interface class that creates multiple TCP or UDP clients to connect
 *              and communicate with advertising devices.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/util/logging/logging.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/basestation/multiClientComms.h"

#define DEBUG_COMMS 0

namespace Anki {
namespace Cozmo {
  
  const size_t HEADER_SIZE = sizeof(RADIO_PACKET_HEADER);

  MultiClientComms::MultiClientComms()
  : isInitialized_(false)
  {
    
  }
  
  Result MultiClientComms::Init(const char* advertisingHostIP, int advertisingPort, unsigned int maxSentBytesPerTic)
  {
    if(isInitialized_) {
      PRINT_NAMED_WARNING("MultiClientComms.Init",
                          "Already initialized, disconnecting all devices and from "
                          "advertisement servier, then will re-initialize.\n");
      
      DisconnectAllDevices();
      advertisingChannelClient_.Disconnect();
      isInitialized_ = false;
    }
    
    maxSentBytesPerTic_ = maxSentBytesPerTic;
    advertisingHostIP_ = advertisingHostIP;
    
    if(false == advertisingChannelClient_.Connect(advertisingHostIP_, advertisingPort)) {
      PRINT_NAMED_ERROR("MultiClientComms.Init", "Failed to connect to advertising host at %s "
                        "on port %d.\n", advertisingHostIP_, advertisingPort);
      return RESULT_FAIL;
    }
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
    
    isInitialized_ = true;
    
    return RESULT_OK;
  }
  
  MultiClientComms::~MultiClientComms()
  {
    DisconnectAllDevices();
  }
 
  
  // Returns true if we are ready to use TCP
  bool MultiClientComms::IsInitialized()
  {
    return true;
  }
  
  size_t MultiClientComms::Send(const Comms::MsgPacket &p)
  {
    // TODO: Instead of sending immediately, maybe we should queue them and send them all at
    // once to more closely emulate BTLE.

    #if(DO_SIM_COMMS_LATENCY)
    /*
    // If no send latency, just send now
    if (SIM_SEND_LATENCY_SEC == 0) {
      if ((maxSentBytesPerTic_ > 0) && (bytesSentThisUpdateCycle_ + p.dataLen > maxSentBytesPerTic_)) {
        #if(DEBUG_COMMS)
        PRINT_NAMED_INFO("MultiClientComms.MaxSendLimitReached", "queueing message\n");
        #endif
      } else {
        bytesSentThisUpdateCycle_ += p.dataLen;
        return RealSend(p);
      }
    }
    */
    // Otherwise add to send queue
    sendMsgPackets_[p.destId].emplace_back(std::piecewise_construct,
                                           std::forward_as_tuple((f32)(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + SIM_SEND_LATENCY_SEC)),
                                           std::forward_as_tuple(p));
    
    // Fake the number of bytes sent
    size_t numBytesSent = sizeof(RADIO_PACKET_HEADER) + sizeof(u32) + p.dataLen;
    return numBytesSent;
  }
  
  int MultiClientComms::RealSend(const Comms::MsgPacket &p)
  {
    #endif // #if(DO_SIM_COMMS_LATENCY)
    
    connectedDevicesIt_t it = connectedDevices_.find(p.destId);
    if (it != connectedDevices_.end()) {
      
      bool isTCP = it->second.protocol == Anki::Comms::TCP;
      
      if (isTCP) {
        // Wrap message in header/footer
        char sendBuf[p.dataLen + 10]; // Extra bytes for TCP header stuff
        int sendBufLen = 0;
        
        memcpy(sendBuf, RADIO_PACKET_HEADER, sizeof(RADIO_PACKET_HEADER));
        sendBufLen += sizeof(RADIO_PACKET_HEADER);
        sendBuf[sendBufLen++] = p.dataLen;
        sendBuf[sendBufLen++] = p.dataLen >> 8;
        sendBuf[sendBufLen++] = 0;
        sendBuf[sendBufLen++] = 0;
        memcpy(sendBuf + sendBufLen, p.data, p.dataLen);
        sendBufLen += p.dataLen;
        return ((TcpClient*)it->second.client)->Send(sendBuf, sendBufLen);
      } else {
        return ((UdpClient*)it->second.client)->Send((char*)p.data, p.dataLen);
      }
    

      /*
      printf("SENDBUF (hex): ");
      PrintBytesHex(sendBuf, sendBufLen);
      printf("\nSENDBUF (uint): ");
      PrintBytesUInt(sendBuf, sendBufLen);
      printf("\n");
      */
    }
    return -1;
    
  }
  
  
  void MultiClientComms::Update(bool send_queued_msgs)
  {
    
    f32 currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // Read datagrams and update advertising device list.
    Comms::AdvertisementMsg advMsg;
    int bytes_recvd = 0;
    do {
      bytes_recvd = advertisingChannelClient_.Recv((char*)&advMsg, sizeof(advMsg));
      if (bytes_recvd == sizeof(advMsg)) {
        
        // Check if already connected to this device.
        // Advertisement may have arrived right after connection.
        // If not already connected, add it to advertisement list.
        if (connectedDevices_.find(advMsg.id) == connectedDevices_.end()) {
          
#if(DEBUG_COMMS)
          if (advertisingDevices_.find(advMsg.id) == advertisingDevices_.end()) {
            printf("Detected advertising device %d on host %s at port %d\n", advMsg.id, advMsg.ip, advMsg.port);
          }
#endif
          
          advertisingDevices_[advMsg.id].devInfo = advMsg;
          advertisingDevices_[advMsg.id].lastSeenTime = currTime;
        }
      }
    } while(bytes_recvd > 0);
    
    
    
    // Remove devices from advertising list if they're already connected.
    advertisingDevicesIt_t it = advertisingDevices_.begin();
    while(it != advertisingDevices_.end()) {
      if (currTime - it->second.lastSeenTime > ROBOT_ADVERTISING_TIMEOUT_S) {
        #if(DEBUG_COMMS)
        printf("Removing device %d from advertising list. (Last seen: %f, curr time: %f)\n", it->second.devInfo.id, it->second.lastSeenTime, currTime);
        #endif
        advertisingDevices_.erase(it++);
      } else {
        ++it;
      }
    }
    
    // Read all messages from all connected devices
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
    if (send_queued_msgs) {
      bytesSentThisUpdateCycle_ = 0;
      std::map<int, PacketQueue_t>::iterator sendQueueIt = sendMsgPackets_.begin();
      while (sendQueueIt != sendMsgPackets_.end()) {
        PacketQueue_t* pQueue = &(sendQueueIt->second);
        while (!pQueue->empty()) {
          if (pQueue->front().first <= currTime) {
            
            if ((maxSentBytesPerTic_ > 0) && (bytesSentThisUpdateCycle_ + pQueue->front().second.dataLen > maxSentBytesPerTic_)) {
              #if(DEBUG_COMMS)
              PRINT_NAMED_INFO("MultiClientComms.MaxSendLimitReached", "%d messages left in queue to send later\n", pQueue->size() - 1);
              #endif
              break;
            }
            bytesSentThisUpdateCycle_ += pQueue->front().second.dataLen;
            if (RealSend(pQueue->front().second) < 0) {
              PRINT_NAMED_WARNING("MultiClientComms.RealSendFail", "");
            }
            pQueue->pop_front();
          } else {
            break;
          }
        }
        ++sendQueueIt;
      }
    }
    #endif  // #if(DO_SIM_COMMS_LATENCY)
    
    // Ping the advertisement channel in case it wasn't present at Init()
    static u8 pingTimer = 0;
    if (pingTimer++ == 10) {
      const char zero = 0;
      advertisingChannelClient_.Send(&zero,1);
      pingTimer = 0;
    }
  }
  
  
  void MultiClientComms::PrintRecvBuf(int devID)
  {
    #if(DEBUG_COMMS)
    if (connectedDevices_.find(devID) != connectedDevices_.end()) {
      int numBytes = connectedDevices_[devID].recvDataSize;
      printf("Device %d recv buffer (%d bytes): ", devID, numBytes);
      for (int i=0; i<numBytes;i++){
        u8 t = connectedDevices_[devID].recvBuf[i];
        printf("0x%x ", t);
      }
      printf("\n");

    }
    #endif
  }
  
  
  void MultiClientComms::ReadAllMsgPackets()
  {
    
    // Read from all connected clients.
    // Enqueue complete messages.
    connectedDevicesIt_t it = connectedDevices_.begin();
    while ( it != connectedDevices_.end() ) {
      
      ConnectedDeviceInfo &c = it->second;
      bool isTCP = c.protocol == Anki::Comms::TCP;

      while(1) { // Keep reading socket until no bytes available
      
//        int bytes_recvd = c.client->Recv((char*)c.recvBuf + c.recvDataSize,
//                                         ConnectedDeviceInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);

        
        int bytes_recvd = 0;
        if (isTCP) {
          bytes_recvd = ((TcpClient*)c.client)->Recv((char*)c.recvBuf + c.recvDataSize,
                                                     ConnectedDeviceInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);
        } else {
          bytes_recvd = ((UdpClient*)c.client)->Recv((char*)c.recvBuf + c.recvDataSize,
                                                     ConnectedDeviceInfo::MAX_RECV_BUF_SIZE - c.recvDataSize);
        }
        
        
        
        if (bytes_recvd == 0) {
          it++;
          break;
        }
        if (bytes_recvd < 0) {
          // Disconnect client
          #if(DEBUG_COMMS)
          printf("MultiClientComms: Recv failed. Disconnecting client\n");
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
          
          connectedDevices_.erase(it++);
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
            
            std::size_t n = hPtr - c.recvBuf;
            if (n != 0) {
              // Header was not found at the beginning.
              // Delete everything up until the header.
              PRINT_STREAM_WARNING("MultiClientComms.PartialMsgRecvd",
                                   "Header not found where expected. Dropping preceding " << n << " bytes");
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
  
  
  bool MultiClientComms::ConnectToDeviceByID(int devID)
  {
    // Check if already connected
    if (connectedDevices_.find(devID) != connectedDevices_.end()) {
      return true;
    }
    
    // Check if the device is available to connect to
    advertisingDevicesIt_t it = advertisingDevices_.find(devID);
    if (it != advertisingDevices_.end()) {
      
      /*
#if(USE_UDP_ROBOT_COMMS)
      UdpClient *client = new UdpClient();
#else
      TcpClient *client = new TcpClient();
#endif
       */
      
      bool isTCP = it->second.devInfo.protocol == Anki::Comms::TCP;
      
      void* client;
      if(isTCP) {
        client = (void*)(new TcpClient());
      } else {
        client = (void*)(new UdpClient());
      }
      
//      if (client->Connect((char*)it->second.devInfo.ip, it->second.devInfo.port)) {
      if ( (isTCP  && ((TcpClient*)client)->Connect((char*)it->second.devInfo.ip, it->second.devInfo.port)) ||
          (!isTCP && ((UdpClient*)client)->Connect((char*)it->second.devInfo.ip, it->second.devInfo.port)) ){
        #if(DEBUG_COMMS)
        printf("Connected to device %d at %s:%d\n", it->second.devInfo.id, it->second.devInfo.ip, it->second.devInfo.port);
        #endif
        connectedDevices_[devID].client = client;
        connectedDevices_[devID].protocol = it->second.devInfo.protocol;
        
        // Remove from advertising list
        advertisingDevices_.erase(it);
        
        return true;
      }
      printf("WARN: Connection attempt to device %d at %s:%d failed\n",
             it->second.devInfo.id, it->second.devInfo.ip, it->second.devInfo.port);
    }
    
    return false;
  }
  
  void MultiClientComms::DisconnectDeviceByID(int devID)
  {
    connectedDevicesIt_t it = connectedDevices_.find(devID);
    if (it != connectedDevices_.end()) {
//      it->second.client->Disconnect();
//      delete it->second.client;
      
      if (it->second.protocol == Anki::Comms::TCP) {
        ((TcpClient*)it->second.client)->Disconnect();
        delete (TcpClient*)(it->second.client);
      } else {
        ((UdpClient*)it->second.client)->Disconnect();
        delete (UdpClient*)(it->second.client);
      }
      
      connectedDevices_.erase(it);
    }
  }
  
  
  u32 MultiClientComms::ConnectToAllDevices()
  {
    for (advertisingDevicesIt_t it = advertisingDevices_.begin(); it != advertisingDevices_.end(); it++)
    {
      ConnectToDeviceByID(it->first);
    }
    
    return (u32)connectedDevices_.size();
  }
  
  u32 MultiClientComms::GetAdvertisingDeviceIDs(std::vector<int> &devIDs)
  {
    devIDs.clear();
    for (advertisingDevicesIt_t it = advertisingDevices_.begin(); it != advertisingDevices_.end(); it++)
    {
      devIDs.push_back(it->first);
    }
    
    return (u32)devIDs.size();
  }
  
  
  void MultiClientComms::ClearAdvertisingDevices()
  {
    advertisingDevices_.clear();
  }
  
  void MultiClientComms::DisconnectAllDevices()
  {
    for(connectedDevicesIt_t it = connectedDevices_.begin(); it != connectedDevices_.end();) {
//      it->second.client->Disconnect();
//      delete it->second.client;
      
      if (it->second.protocol == Anki::Comms::TCP) {
        ((TcpClient*)it->second.client)->Disconnect();
        delete (TcpClient*)(it->second.client);
      } else {
        ((UdpClient*)it->second.client)->Disconnect();
        delete (UdpClient*)(it->second.client);
      }
      
      it = connectedDevices_.erase(it);
    }
    
    connectedDevices_.clear();
  }
  
  
  // Returns true if a MsgPacket was successfully gotten
  bool MultiClientComms::GetNextMsgPacket(Comms::MsgPacket& p)
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
  
  
  u32 MultiClientComms::GetNumPendingMsgPackets()
  {
    #if(DO_SIM_COMMS_LATENCY)
    return numRecvRdyMsgs_;
    #else
    return (u32)recvdMsgPackets_.size();
    #endif
  };
  
  void MultiClientComms::ClearMsgPackets()
  {
    recvdMsgPackets_.clear();
    
    #if(DO_SIM_COMMS_LATENCY)
    numRecvRdyMsgs_ = 0;
    #endif
  };
  
  u32 MultiClientComms::GetNumMsgPacketsInSendQueue(int devID)
  {
    std::map<int, PacketQueue_t>::iterator it = sendMsgPackets_.find(devID);
    if (it != sendMsgPackets_.end()) {
      return static_cast<u32>(it->second.size());
    }
    return 0;
  }
  
}  // namespace Cozmo
}  // namespace Anki


