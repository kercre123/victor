/*
 * sim_radio.cpp
 *
 *   Implementation of HAL radio functionality for the simulator.
 *
 *   Author: Andrew Stein
 *
 */

#ifndef SIMULATOR
#error This file (sim_radio.cpp) should not be used without SIMULATOR defined.
#endif

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"
#include <stdio.h>
#include <string>

#include "anki/messaging/shared/TcpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include "anki/messaging/shared/LocalUdpServer.h"
#include "anki/messaging/shared/utilMessaging.h"

#include "clad/types/advertisementTypes.h"
#include "util/helpers/arrayHelpers.h"
#include "util/logging/logging.h"

// For getting local host's IP address
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"
      const size_t RECV_BUFFER_SIZE = 1024 * 4;

      // For communications with animProcess
      LocalUdpServer server;

      // For advertising service
      UdpClient advRegClient;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t recvBufSize_ = 0;

      AdvertisementRegistrationMsg regMsg;
    }

    // Register robot with advertisement service
    void AdvertiseRobot()
    {
      static TimeStamp_t lastAdvertisedTime = 0;
      if (lastAdvertisedTime + ROBOT_ADVERTISING_PERIOD_MS < HAL::GetTimeStamp()) {

        //printf("sim_radio: Sending registration for robot %d at address %s on port %d (time: %u)\n",
        //      regMsg.id, regMsg.ip, regMsg.port, HAL::GetTimeStamp());
        regMsg.enableAdvertisement = 1;
        regMsg.oneShot = 1;

        // Add tag byte(s?) on the front of the message over the socket to validate on the other side
        char msg[64];
        memcpy(msg, &ROBOT_ADVERTISING_HEADER_TAG, sizeof(ROBOT_ADVERTISING_HEADER_TAG));
        uint32_t msgSize = regMsg.Size() + sizeof(ROBOT_ADVERTISING_HEADER_TAG);
        assert(msgSize <= sizeof(msg));
        msgSize = (msgSize <= sizeof(msg)) ? msgSize : sizeof(msg);
        memcpy(&msg[sizeof(ROBOT_ADVERTISING_HEADER_TAG)], &regMsg, msgSize-sizeof(ROBOT_ADVERTISING_HEADER_TAG));

        advRegClient.Send(msg, msgSize);

        lastAdvertisedTime = HAL::GetTimeStamp();
      }

    }

    const char* const GetLocalIP()
    {
      // Get robot's IPv4 address.
      // Looking for (and assuming there is only one) address that starts with 192.
      struct ifaddrs *ifaddr, *ifa;
      if (getifaddrs(&ifaddr) != 0) {
        PRINT_NAMED_ERROR("simHAL.GetLocalIP.getifaddrs_failed", "");
        assert(false);
      }

      int family, s, n;
      static char host[NI_MAXHOST];
      for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
          continue;

        family = ifa->ifa_addr->sa_family;

        // Display IPv4 addresses only
        if (family == AF_INET) {
          s = getnameinfo(ifa->ifa_addr,
                          (family == AF_INET) ? sizeof(struct sockaddr_in) :
                          sizeof(struct sockaddr_in6),
                          host, NI_MAXHOST,
                          NULL, 0, NI_NUMERICHOST);
          if (s != 0) {
            PRINT_NAMED_ERROR("simHAL.GetLocalIP.getnameinfo_failed", "");
            assert(false);
          }

          // Does address start with 192?
          if (strncmp(host, "192.", 4) == 0)
          {
            PRINT_NAMED_INFO("simHAL.GetLocalIP.IP", "%s", host);
            break;
          }
        }
      }
      freeifaddrs(ifaddr);

      return host;
    }

    Result InitSimRadio(const char* advertisementIP)
    {
      PRINT_NAMED_INFO("simHAL.InitSimRadio.StartListening", "Listening on %s", ROBOT_UDP_PATH);
      if (!server.StartListening(ROBOT_UDP_PATH)) {
        PRINT_NAMED_ERROR("HAL.InitSimRadio.ListenFailed", "Unable to listen on %s", ROBOT_UDP_PATH);
        return RESULT_FAIL_IO;
      }

      // Register with advertising service by sending IP and port info
      // NOTE: Since there is no ACK robot_advertisement_controller must be running before this happens!
      //       We also assume that when working with simulated robots on Webots, the advertisement service is running on the same host.
      advRegClient.Connect(advertisementIP, ROBOT_ADVERTISEMENT_REGISTRATION_PORT);

      
      // TODO: The advertisement stuff should probably move to animProcess
      // Fill in advertisement registration message
      regMsg.id = HAL::GetID();
      //regMsg.toEnginePort = ROBOT_RADIO_BASE_PORT + regMsg.id;
      regMsg.toEnginePort = ANIM_PROCESS_SERVER_BASE_PORT + regMsg.id;
      regMsg.fromEnginePort = regMsg.toEnginePort;
      
      strncpy(regMsg.ip, GetLocalIP(), sizeof(regMsg.ip));
      regMsg.ip[ARRAY_SIZE(regMsg.ip)-1] = 0; // ensure null termination in event of strncpy src > dest
      regMsg.ip_length = strlen(regMsg.ip);

      recvBufSize_ = 0;

      return RESULT_OK;
    }

    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }


    void HAL::RadioUpdateState(u8 wifi)
    {
      if (wifi == 0) DisconnectRadio();
    }

    void HAL::DisconnectRadio(void)
    {
      server.Disconnect();
      recvBufSize_ = 0;
    }

    bool HAL::RadioSendPacket(const void *buffer, const u32 length, u8 socket)
    {
      (void)socket;

      if (server.HasClient()) {

        const ssize_t bytesSent = server.Send((const char*)buffer, length);

        if (bytesSent < length) {
          PRINT_NAMED_ERROR("HAL.RadioSendPacket", "Failed to send msg contents (%zd bytes sent)", bytesSent);
          DisconnectRadio();
          return false;
        }

        /*
        printf("SENT: ");
        for (int i=0; i<HEADER_LENGTH;i++){
          u8 t = header[i];
          printf("0x%x ", t);
        }
        for (int i=0; i<size;i++){
          u8 t = ((char*)buffer)[i];
          printf("0x%x ", t);
        }
        for (int i=0; i<sizeof(RADIO_PACKET_FOOTER);i++){
          u8 t = RADIO_PACKET_FOOTER[i];
          printf("0x%x ", t);
        }
        printf("\n");
        */

        return true;
      }
      return false;

    } // RadioSendMessage()


    size_t RadioGetNumBytesAvailable(void)
    {

      // Check for incoming data and add it to receive buffer
      const size_t tempSize = RECV_BUFFER_SIZE - recvBufSize_;
      assert(tempSize < std::numeric_limits<int>::max());
      
      const ssize_t dataSize = server.Recv((char*)&recvBuf_[recvBufSize_], static_cast<int>(tempSize));
      if (dataSize > 0) {
        recvBufSize_ += dataSize;
      } else if (dataSize < 0) {
        // Something went wrong
        PRINT_NAMED_ERROR("HAL.RadioGetNumBytesAvailable", "Failed to receive data");
        HAL::DisconnectRadio();
      }

      return recvBufSize_;

    } // RadioGetNumBytesAvailable()

    /*
    s32 HAL::RadioPeekChar(u32 offset)
    {
      if(RadioGetNumBytesAvailable() <= offset) {
        return -1;
      }

      return static_cast<s32>(recvBuf_[offset]);
    }

    s32 HAL::RadioGetChar(void) { return RadioGetChar(0); }

    s32 HAL::RadioGetChar(u32 timeout)
    {
      u8 c;
      if(RadioGetData(&c, sizeof(u8)) == RESULT_OK) {
        return static_cast<s32>(c);
      }
      else {
        return -1;
      }
    }
     */

    // TODO: would be nice to implement this in a way that is not specific to
    //       hardware vs. simulated radio receivers, and just calls lower-level
    //       radio functions.
    u32 HAL::RadioGetNextPacket(u8* buffer)
    {
      // Read available datagram
      const ssize_t dataLen = server.Recv((char*)recvBuf_, RECV_BUFFER_SIZE);
      if (dataLen < 0) {
        // Something went wrong
        PRINT_NAMED_ERROR("HAL.RadioGetNextPacket", "Failed to receive data");
        DisconnectRadio();
        return 0;
      } else if (dataLen == 0) {
        // Nothing available
        return 0;
      }

      recvBufSize_ = dataLen;
      
      // Copy message contents to buffer
      std::memcpy((void*)buffer, recvBuf_, dataLen);

      return (u32) dataLen;

    } // RadioGetNextPacket()

    void RadioUpdate()
    {
      if (!server.HasClient()) {
        AdvertiseRobot();
      }
    }

  } // namespace Cozmo
} // namespace Anki
