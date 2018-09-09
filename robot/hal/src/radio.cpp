/*
 * radio.cpp
 *
 *   Implemenation of HAL "radio" functionality for Cozmo V2
 *   This is actually the robot interface to the animation process
 *
 *   Author: Andrew Stein
 *
 */

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include <assert.h>
#include <stdio.h>
#include <string>

#include "coretech/messaging/shared/LocalUdpServer.h"
#include "coretech/messaging/shared/socketConstants.h"

#define ARRAY_SIZE(inArray)   (sizeof(inArray) / sizeof((inArray)[0]))


namespace Anki {
  namespace Vector {

    namespace { // "Private members"
      const size_t RECV_BUFFER_SIZE = 1024 * 4;

      // For communications with basestation
      LocalUdpServer server;

      u8 recvBuf_[RECV_BUFFER_SIZE];
      size_t recvBufSize_ = 0;
    }


    Result InitRadio()
    {
      const RobotID_t robotID = HAL::GetID();
      const std::string & server_path = std::string(Victor::ANIM_ROBOT_SERVER_PATH) + std::to_string(robotID);

      AnkiInfo("HAL.InitRadio.StartListening", "Start listening at %s", server_path.c_str());
      if (!server.StartListening(server_path.c_str())) {
        AnkiError("HAL.InitRadio.UDPServerFailed", "Unable to listen at %s", server_path.c_str());
        return RESULT_FAIL_IO;
      }

      return RESULT_OK;
    }

    void StopRadio()
    {
      AnkiInfo("HAL.RadioStop", "");
      server.StopListening();
    }

    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }

    void HAL::DisconnectRadio(bool sendDisconnectMsg)
    {
      if (sendDisconnectMsg && RadioIsConnected()) {
        RobotInterface::RobotServerDisconnect msg;
        RobotInterface::SendMessage(msg);
      }
      
      server.Disconnect();
      recvBufSize_ = 0;
    }

    bool HAL::RadioSendPacket(const void *buffer, const size_t length)
    {
      if (server.HasClient()) {
        const ssize_t bytesSent = server.Send((char*)buffer, length);
        if (bytesSent < (ssize_t) length) {
          AnkiError("HAL.RadioSendPacket.FailedToSend", "Failed to send msg contents (%zd/%zu sent)", bytesSent, length);
          DisconnectRadio(false);
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


    u32 HAL::RadioGetNextPacket(u8* buffer)
    {
      // Read available datagram
      const ssize_t dataLen = server.Recv((char*)recvBuf_, RECV_BUFFER_SIZE);
      if (dataLen > 0) {
        recvBufSize_ = dataLen;
      }
      else if (dataLen < 0) {
        // Something went wrong
        AnkiError("HAL.RadioGetNextPacket", "Receive failed");
        DisconnectRadio(false);
        return 0;
      }
      else {
        return 0;
      }

      // Copy message contents to buffer
      std::memcpy((void*)buffer, recvBuf_, dataLen);
      
      return static_cast<u32>(dataLen);

    } // RadioGetNextMessage()
  } // namespace Vector
} // namespace Anki
