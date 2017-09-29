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

#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/cozmoAnimComms.h"

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animation/cannedAnimationContainer.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include "clad/robotInterface/messageEngineToRobot_sendToRobot_helper.h"
#include "clad/robotInterface/messageFromAnimProcess.h"

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
    
    // For comms with engine
    const int MAX_PACKET_BUFFER_SIZE = 2048;
    u8 pktBuffer_[MAX_PACKET_BUFFER_SIZE];
    
    AnimationStreamer* _animStreamer = nullptr;
    Audio::EngineRobotAudioInput* _audioInput = nullptr;
    
    
    const u32 kMaxNumAvailableAnimsToReportPerTic = 100;
    
    // The last AnimID that was sent to engine in response to RequestAvailableAnimations.
    // If negative, it means we're not currently doling.
    bool _isDolingAnims = false;
    u32 _nextAnimIDToDole;
    
  } // private namespace


  // Forward declarations
  extern "C" TimeStamp_t GetTimeStamp(void);
  
  void ProcessMessage(RobotInterface::EngineToRobot& msg);
  extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);

  
// #pragma mark --- Messages Method Implementations ---

  Result Init(AnimationStreamer& animStreamer, Audio::EngineRobotAudioInput& audioInput)
  {
    
    // Setup robot and engine sockets
    CozmoAnimComms::InitComms();
    
    // Setup reliable transport
    ReliableTransport_Init();
    ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous

    _animStreamer = &animStreamer;
    _audioInput   = &audioInput;
    
    return RESULT_OK;
  }
  
  
  void DoleAvailableAnimations()
  {
    // If not already doling, dole animations
    if (_isDolingAnims) {
      u32 numAnimsDoledThisTic = 0;
      const auto& animIDToNameMap = _animStreamer->GetCannedAnimationContainer().GetAnimationIDToNameMap();
      auto it = animIDToNameMap.find(_nextAnimIDToDole);
      for (; it != animIDToNameMap.end() && numAnimsDoledThisTic < kMaxNumAvailableAnimsToReportPerTic; ++it) {
        
        RobotInterface::AnimationAvailable msg;
        msg.id = it->first;
        msg.name_length = it->second.length();
        snprintf(msg.name, sizeof(msg.name), "%s", it->second.c_str());
        SendMessageToEngine(msg);
        
        //PRINT_NAMED_INFO("AvailableAnim", "[%d]: %s", msg.id, msg.name);

        ++numAnimsDoledThisTic;
      }
      if (it == animIDToNameMap.end()) {
        PRINT_NAMED_INFO("EngineMessages.DoleAvailableAnimations.Done", "%zu anims doled", animIDToNameMap.size());
        _isDolingAnims = false;
        
        EndOfMessage msg;
        msg.messageType = MessageType::AnimationAvailable;
        RobotInterface::SendMessageToEngine(msg);
      } else {
        _nextAnimIDToDole = it->first;
      }
    }
  }
  
  void ProcessMessage(RobotInterface::EngineToRobot& msg)
  {
    //PRINT_NAMED_WARNING("ProcessMessage.EngineToRobot", "%d", msg.tag);
    
    switch(msg.tag)
    {
      // TODO: If this switch block becomes huge, we can auto-generate it with an emitter.
      //       Most E2R messages will be ignored here though so it probably shouldn't be necessary.
      //       OR the emitter could be smart and only create the switch for messages in a specific range of tags.
      //#include "clad/robotInterface/messageEngineToRobot_switch_group_anim.def"
        
      case Anki::Cozmo::RobotInterface::EngineToRobot::Tag_lockAnimTracks:
      {
        PRINT_NAMED_INFO("EngineMessages.ProcessMessage.LockTracks", "0x%x", msg.lockAnimTracks.whichTracks);
        _animStreamer->SetLockedTracks(msg.lockAnimTracks.whichTracks);
        return;
      }
        
      case Anki::Cozmo::RobotInterface::EngineToRobot::Tag_playAnim:
      {
        PRINT_NAMED_INFO("EngineMesssages.ProcessMessage.PlayAnim",
                         "AnimID: %d, Tag: %d",
                         msg.playAnim.animID, msg.playAnim.tag);
        
        _animStreamer->SetStreamingAnimation(msg.playAnim.animID, msg.playAnim.tag, msg.playAnim.numLoops);
        return;
      }
      case Anki::Cozmo::RobotInterface::EngineToRobot::Tag_abortAnimation:
      {
        PRINT_NAMED_INFO("EngineMessages.ProcessMessage.AbortAnim",
                         "Tag: %d",
                         msg.abortAnimation.tag);
        
        // TODO: Need to hook this up to AnimationStreamer
        //       Maybe _animStreamer->Abort(msg.abortAnimation.tag)?
      }
        
      case Anki::Cozmo::RobotInterface::EngineToRobot::Tag_requestAvailableAnimations:
      {
        PRINT_NAMED_INFO("EngineMessages.RequestAvailableAnimations", "");
        if (!_isDolingAnims) {
          const auto& animIDToNameMap = _animStreamer->GetCannedAnimationContainer().GetAnimationIDToNameMap();
          if (!animIDToNameMap.empty()) {
            _nextAnimIDToDole =  animIDToNameMap.begin()->first;
            _isDolingAnims = true;
          } else {
            PRINT_NAMED_WARNING("EngineMessages.RequestAvailableAnimations.NoAnimsAvailable", "");
          }
        } else {
          PRINT_NAMED_WARNING("EngineMessages.RequestAvailableAnimations.AlreadyDoling", "");
        }
        return;
      }
        
      case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_postAudioEvent:
      case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_stopAllAudioEvents:
      case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_postAudioGameState:
      case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_postAudioSwitchState:
      case (int)Anki::Cozmo::RobotInterface::EngineToRobot::Tag_postAudioParameter:
      {
        _audioInput->HandleEngineToRobotMsg(msg);
        break;
      }
        
      default:
        break;
    }

    // Send message along to robot if it wasn't handled here
    CozmoAnimComms::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());

  } // ProcessMessage()


// #pragma --- Message Dispatch Functions ---

  Result MonitorConnectionState(void)
  {
    // Send block connection state when engine connects
    static bool wasConnected = false;
    if (!wasConnected && CozmoAnimComms::EngineIsConnected()) {
      
      RobotInterface::RobotAvailable idMsg;
      idMsg.hwRevision = 0;
      RobotInterface::SendMessageToEngine(idMsg);
      
      // send firmware info indicating simulated or physical robot type
      {
#ifdef SIMULATOR
        std::string firmwareJson{"{\"version\":0,\"time\":0,\"sim\":0}"};
#else
        std::string firmwareJson{"{\"version\":0,\"time\":0}"};
#endif
        RobotInterface::FirmwareVersion msg;
        msg.RESRVED = 0;
        msg.json_length = firmwareJson.size() + 1;
        std::memcpy(msg.json, firmwareJson.c_str(), firmwareJson.size() + 1);
        RobotInterface::SendMessageToEngine(msg);
      }
      
      wasConnected = true;
    }
    else if (wasConnected && !CozmoAnimComms::EngineIsConnected()) {
      wasConnected = false;
    }
    
    return RESULT_OK;
    
  } // step()

  
  void Update()
  {
    MonitorConnectionState();

    DoleAvailableAnimations();
    
    // Process incoming messages from engine
    u32 dataLen;

    //ReliableConnection_printState(&connection);

    while((dataLen = CozmoAnimComms::GetNextPacketFromEngine(pktBuffer_, MAX_PACKET_BUFFER_SIZE)) > 0)
    {
      s16 res = ReliableTransport_ReceiveData(&connection, pktBuffer_, dataLen);
      if (res < 0)
      {
        //AnkiWarn( 1205, "ReliableTransport.PacketNotAccepted", 347, "%d", 1, res);
      }
    }

    if (CozmoAnimComms::EngineIsConnected())
    {
      if (ReliableTransport_Update(&connection) == false) // Connection has timed out
      {
        Receiver_OnDisconnect(&connection);
        // Can't print anything because we have no where to send it
      }
    }
    
    // Process incoming messages from robot
    while ((dataLen = CozmoAnimComms::GetNextPacketFromRobot(pktBuffer_, MAX_PACKET_BUFFER_SIZE)) > 0)
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
        const int tagSize = sizeof(msgBuf.tag);
        SendToEngine(msgBuf.GetBuffer()+tagSize, msgBuf.Size()-tagSize, msgBuf.tag);
      }
      
    }

  }

  // Required by reliableTransport.c
  TimeStamp_t GetTimeStamp()
  {
    return BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }
  
  bool SendToRobot(const RobotInterface::EngineToRobot& msg)
  {
    return CozmoAnimComms::SendPacketToRobot(msg.GetBuffer(), msg.Size());
  }
  
  bool SendToEngine(const void *buffer, const u16 size, const u8 msgID)
  {
    const bool reliable = msgID < EnumToUnderlyingType(RobotInterface::ToRobotAddressSpace::TO_ENG_UNREL);
    const bool hot = false;
    if (CozmoAnimComms::EngineIsConnected())
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

} // namespace Messages
} // namespace Cozmo
} // namespace Anki


// Shim for reliable transport
bool UnreliableTransport_SendPacket(uint8_t* buffer, uint16_t bufferSize)
{
  return Anki::Cozmo::CozmoAnimComms::SendPacketToEngine(buffer, bufferSize);
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
  Anki::Cozmo::CozmoAnimComms::UpdateEngineCommsState(1);
}

void Receiver_OnConnected(ReliableConnection* connection)
{
  //AnkiInfo( 122, "Receiver_OnConnected", 370, "ReliableTransport connection completed", 0);
  Anki::Cozmo::CozmoAnimComms::UpdateEngineCommsState(1);
}

void Receiver_OnDisconnect(ReliableConnection* connection)
{
  Anki::Cozmo::CozmoAnimComms::UpdateEngineCommsState(0);   // Must mark connection disconnected BEFORE trying to print
  //AnkiInfo( 123, "Receiver_OnDisconnect", 371, "ReliableTransport disconnected", 0);
  ReliableConnection_Init(connection, NULL); // Reset the connection
  Anki::Cozmo::CozmoAnimComms::UpdateEngineCommsState(0);
}

