/**
 * File: engineMessages.cpp
 *
 * Author: Kevin Yoon
 * Created: 6/30/2017
 *
 * Description: Shuttles messages between engine and robot processes.
 *              Responds to engine messages pertaining to animations
 *              and inserts messages as appropriate into robot-bound stream.
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "engineMessages.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

// For animProcess<->Engine communications
#include "anki/cozmo/transport/IUnreliableTransport.h"
#include "anki/cozmo/transport/IReceiver.h"
#include "anki/cozmo/transport/reliableTransport.h"

// For animProcess<->Robot communications
#include "anki/messaging/shared/UdpClient.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/logging/logging.h"

#include <string.h>
#include <stdio.h>
#include <math.h>


namespace Anki {
  namespace Cozmo {
    
    ReliableConnection connection;
    
    namespace Messages {

      namespace {
        
        const int MAX_PACKET_BUFFER_SIZE = 2048;
        u8 pktBuffer_[MAX_PACKET_BUFFER_SIZE];
      } // private namespace

// #pragma mark --- Messages Method Implementations ---

      Result Init()
      {
        // Setup engine comms
        HAL::InitRadio();
        ReliableTransport_Init();
        ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous

        
        
        return RESULT_OK;
      }

      void ProcessMessage(RobotInterface::EngineToRobot& msg)
      {
        //PRINT_NAMED_WARNING("ProcessMessage.EngineToRobot", "%d", msg.tag);
        
        switch(msg.tag)
        {
          //#include "clad/robotInterface/messageEngineToRobot_switch_group_anim.def"
            
          case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_enableAnimTracks:
          {
            // Do something
            // ...
            
            break;
          }

        }

        // Send message along to robot
        HAL::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());

      } // ProcessMessage()


// #pragma --- Message Dispatch Functions ---


      
      Result MonitorConnectionState(void)
      {
        // Send block connection state when engine connects
        static bool wasConnected = false;
        if (!wasConnected && HAL::RadioIsConnected()) {
          
          // Send RobotAvailable indicating sim robot
          RobotInterface::RobotAvailable idMsg;
          idMsg.robotID = 0;
          idMsg.hwRevision = 0;
          RobotInterface::SendMessage(idMsg);
          
          // send firmware info indicating simulated robot
          {
            std::string firmwareJson{"{\"version\":0,\"time\":0,\"sim\":0}"};
            RobotInterface::FirmwareVersion msg;
            msg.RESRVED = 0;
            msg.json_length = firmwareJson.size() + 1;
            std::memcpy(msg.json, firmwareJson.c_str(), firmwareJson.size() + 1);
            RobotInterface::SendMessage(msg);
          }
          
          wasConnected = true;
        }
        else if (wasConnected && !HAL::RadioIsConnected()) {
          wasConnected = false;
        }
        
        return RESULT_OK;
        
      } // step()

      
      void Update()
      {
        MonitorConnectionState();

        // Process incoming messages from engine
        u32 dataLen;

        //ReliableConnection_printState(&connection);

        while((dataLen = HAL::RadioGetNextPacket(pktBuffer_)) > 0)
        {
          s16 res = ReliableTransport_ReceiveData(&connection, pktBuffer_, dataLen);
          if (res < 0)
          {
            //AnkiWarn( 1205, "ReliableTransport.PacketNotAccepted", 347, "%d", 1, res);
          }
        }

        if (HAL::RadioIsConnected())
        {
          if (ReliableTransport_Update(&connection) == false) // Connection has timed out
          {
            Receiver_OnDisconnect(&connection);
						// Can't print anything because we have no where to send it
					}
        }
        
        
        
        // Process incoming messages from robot
        while ((dataLen = HAL::GetNextPacketFromRobot(pktBuffer_, MAX_PACKET_BUFFER_SIZE)) > 0)
        {
          Anki::Cozmo::RobotInterface::RobotToEngine msgBuf;
          
          // Copy into structured memory
          memcpy(msgBuf.GetBuffer(), pktBuffer_, dataLen);
          if (!msgBuf.IsValid())
          {
            PRINT_NAMED_WARNING("Receiver.ReceiveData.Invalid", "Receiver got %02x[%d] invalid", pktBuffer_[0], dataLen);
          }
          else if (msgBuf.Size() != dataLen)
          {
            PRINT_NAMED_WARNING("Receiver.ReceiveData.SizeError", "Parsed message size error %d != %d (Tag %02x)", dataLen, msgBuf.Size(), msgBuf.tag);
          }
          else
          {
            // Send up to engine
            ::Anki::Cozmo::HAL::RadioSendMessage(msgBuf.GetBuffer()+1, msgBuf.Size()-1, msgBuf.tag);
          }
          
        }

      }


    } // namespace Messages

    
    namespace HAL {

      TimeStamp_t GetTimeStamp()
      {
        return BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      }
      
      
      // TODO: RENAME !
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID)
      {
        const bool reliable = msgID < RobotInterface::TO_ENG_UNREL;
        const bool hot = false;
        if (::Anki::Cozmo::HAL::RadioIsConnected())
        {
          if (reliable)
          {
            if (ReliableTransport_SendMessage((const uint8_t*)buffer, size, &connection, eRMT_SingleReliableMessage, hot, msgID) == false) // failed to queue reliable message!
            {
              // Have to drop the connection
              //PRINT("Dropping connection because can't queue reliable messages\r\n");
              ReliableTransport_Disconnect(&connection);
              Receiver_OnDisconnect(&connection);
              return false;
            }
            else
            {
              return true;
            }
          }
          else
          {
            return ReliableTransport_SendMessage((const uint8_t*)buffer, size, &connection, eRMT_SingleUnreliableMessage, hot, msgID);
          }
        }
        else
        {
          return false;
        }
      }

    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki


// Shim for reliable transport
bool UnreliableTransport_SendPacket(uint8_t* buffer, uint16_t bufferSize)
{
  return Anki::Cozmo::HAL::RadioSendPacket(buffer, bufferSize);
}

void Receiver_ReceiveData(uint8_t* buffer, uint16_t bufferSize, ReliableConnection* connection)
{
  Anki::Cozmo::RobotInterface::EngineToRobot msgBuf;

  // Copy into structured memory
  memcpy(msgBuf.GetBuffer(), buffer, bufferSize);
  if (!msgBuf.IsValid())
  {
    //AnkiWarn( 119, "Receiver.ReceiveData.Invalid", 367, "Receiver got %02x[%d] invalid", 2, buffer[0], bufferSize);
  }
  else if (msgBuf.Size() != bufferSize)
  {
    //AnkiWarn( 120, "Receiver.ReceiveData.SizeError", 368, "Parsed message size error %d != %d", 2, bufferSize, msgBuf.Size());
  }
  else
  {
    Anki::Cozmo::Messages::ProcessMessage(msgBuf);
  }
}

void Receiver_OnConnectionRequest(ReliableConnection* connection)
{
  ReliableTransport_FinishConnection(connection); // Accept the connection
  //AnkiInfo( 121, "Receiver_OnConnectionRequest", 369, "ReliableTransport new connection", 0);
  Anki::Cozmo::HAL::RadioUpdateState(1);
}

void Receiver_OnConnected(ReliableConnection* connection)
{
  //AnkiInfo( 122, "Receiver_OnConnected", 370, "ReliableTransport connection completed", 0);
  Anki::Cozmo::HAL::RadioUpdateState(1);
}

void Receiver_OnDisconnect(ReliableConnection* connection)
{
  Anki::Cozmo::HAL::RadioUpdateState(0);   // Must mark connection disconnected BEFORE trying to print
  //AnkiInfo( 123, "Receiver_OnDisconnect", 371, "ReliableTransport disconnected", 0);
  ReliableConnection_Init(connection, NULL); // Reset the connection
  Anki::Cozmo::HAL::RadioUpdateState(0);
}

int Anki::Cozmo::HAL::RadioQueueAvailable()
{
  return ReliableConnection_GetReliableQueueAvailable(&Anki::Cozmo::connection);
}
