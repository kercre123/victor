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
#include "cozmoAnim/cozmoAnimContext.h"
#include "cozmoAnim/faceDisplay/faceDebugDraw.h"
#include "cozmoAnim/micDataProcessor.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_sendToEngine_helper.h"
#include "clad/robotInterface/messageEngineToRobot_sendToRobot_helper.h"

// For animProcess<->Engine communications
#include "anki/cozmo/transport/IUnreliableTransport.h"
#include "anki/cozmo/transport/IReceiver.h"
#include "anki/cozmo/transport/reliableTransport.h"

// For animProcess<->Robot communications
#include "coretech/messaging/shared/UdpClient.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/console/consoleSystem.h"
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
    
    AnimationStreamer*            _animStreamer = nullptr;
    Audio::EngineRobotAudioInput* _audioInput = nullptr;
    const CozmoAnimContext*       _context = nullptr;

  } // private namespace


  // Forward declarations
  extern "C" TimeStamp_t GetTimeStamp(void);
  
  void ProcessMessageFromEngine(const RobotInterface::EngineToRobot& msg);
  void ProcessMessageFromRobot(const RobotInterface::RobotToEngine& msg);
  extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);
  void HandleRobotStateUpdate(const Anki::Cozmo::RobotState& robotState);

  
// #pragma mark --- Messages Method Implementations ---

  Result Init(AnimationStreamer* animStreamer, Audio::EngineRobotAudioInput* audioInput, const CozmoAnimContext* context)
  {
    
    // Setup robot and engine sockets
    CozmoAnimComms::InitComms();
    
    // Setup reliable transport
    ReliableTransport_Init();
    ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous

    _animStreamer = animStreamer;
    _audioInput   = audioInput;
    _context      = context;

    DEV_ASSERT(_animStreamer != nullptr, "EngineMessages.Init.NullAnimStreamer");
    DEV_ASSERT(_audioInput != nullptr, "EngineMessages.Init.NullAudioInput");
    DEV_ASSERT(_context != nullptr, "EngineMessages.Init.NullContext");

    return RESULT_OK;
  }
  
  
// ========== START OF PROCESSING MESSAGES FROM ENGINE ==========  

  void ProcessMessageFromEngine(const RobotInterface::EngineToRobot& msg)
  {
    //PRINT_NAMED_WARNING("ProcessMessage.EngineToRobot", "%d", msg.tag);
    bool forwardToRobot = false;
    switch(msg.tag)
    {
      #include "clad/robotInterface/messageEngineToRobot_switch_from_0x50_to_0xAF.def"
      
      default:
        forwardToRobot = true;
        break;
    }

    if (forwardToRobot) {
      // Send message along to robot if it wasn't handled here
      CozmoAnimComms::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());
    }

  } // ProcessMessageFromEngine()



  void Process_lockAnimTracks(const Anki::Cozmo::RobotInterface::LockAnimTracks& msg)
  {
    //PRINT_NAMED_DEBUG("EngineMessages.Process_lockAnimTracks", "0x%x", msg.whichTracks);
    _animStreamer->SetLockedTracks(msg.whichTracks);
  }
    
  void Process_playAnim(const Anki::Cozmo::RobotInterface::PlayAnim& msg)
  {
    const std::string animName(msg.animName, msg.animName_length);

    PRINT_NAMED_INFO("EngineMesssages.Process_playAnim",
                     "Anim: %s, Tag: %d",
                     animName.c_str(), msg.tag);
    
    _animStreamer->SetStreamingAnimation(animName, msg.tag, msg.numLoops);
  }

  void Process_abortAnimation(const Anki::Cozmo::RobotInterface::AbortAnimation& msg)
  {
    PRINT_NAMED_INFO("EngineMessages.Process_abortAnimation", "");
    _animStreamer->Abort();
  }
  
  void Process_displayProceduralFace(const Anki::Cozmo::RobotInterface::DisplayProceduralFace& msg)
  {
    ProceduralFace procFace;
    procFace.SetFromMessage(msg.faceParams);
    _animStreamer->SetProceduralFace(procFace, msg.duration_ms);
    return;
  }
  
  void Process_setFaceHue(const Anki::Cozmo::RobotInterface::SetFaceHue& msg)
  {
    ProceduralFace::SetHue(msg.hue);
    return;
  }

  void Process_displayFaceImageBinaryChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageBinaryChunk& msg) 
  {
    _animStreamer->Process_displayFaceImageChunk(msg);
  }

  void Process_displayFaceImageRGBChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageRGBChunk& msg) 
  {
    _animStreamer->Process_displayFaceImageChunk(msg);
  }

  void Process_postAudioEvent(const Anki::AudioEngine::Multiplexer::PostAudioEvent& msg)
  {
    _audioInput->HandleMessage(msg);
  }
  void Process_stopAllAudioEvents(const Anki::AudioEngine::Multiplexer::StopAllAudioEvents& msg)
  {
    _audioInput->HandleMessage(msg);
  }
  void Process_postAudioGameState(const Anki::AudioEngine::Multiplexer::PostAudioGameState& msg)
  {
    _audioInput->HandleMessage(msg);
  }
  void Process_postAudioSwitchState(const Anki::AudioEngine::Multiplexer::PostAudioSwitchState& msg)
  {
    _audioInput->HandleMessage(msg);
  }
  void Process_postAudioParameter(const Anki::AudioEngine::Multiplexer::PostAudioParameter& msg)
  {
    _audioInput->HandleMessage(msg);
  }
  
  void Process_setDebugConsoleVarMessage(const Anki::Cozmo::RobotInterface::SetDebugConsoleVarMessage& msg)
  {
    // We are using messages generated by the CppLite emitter here, which does not support
    // variable length arrays. CLAD also doesn't have a char, so the "strings" in this message
    // are actually arrays of uint8's. Thus we need to do this reinterpret cast here.
    // In some future world, ideally we avoid all this and use, for example, a web interface to
    // set/access console vars, instead of passing around via CLAD messages.
    const char* varName  = reinterpret_cast<const char *>(msg.varName);
    const char* tryValue = reinterpret_cast<const char *>(msg.tryValue);
    
    // TODO: Ideally, we'd send back a verify message that we (failed to) set this
    
    Anki::Util::IConsoleVariable* consoleVar = Anki::Util::ConsoleSystem::Instance().FindVariable(varName);
    if (consoleVar && consoleVar->ParseText(tryValue) )
    {
      //SendVerifyDebugConsoleVarMessage(_externalInterface, varName, consoleVar->ToString().c_str(), consoleVar, true);
      PRINT_NAMED_INFO("Process_setDebugConsoleVarMessage.Success", "'%s' set to '%s'", varName, tryValue);
      
    }
    else
    {
      PRINT_NAMED_WARNING("Process_setDebugConsoleVarMessage.Fail", "Error setting '%s' to '%s'",
                          varName, tryValue);
      //      SendVerifyDebugConsoleVarMessage(_externalInterface, msg.varName.c_str(),
      //                                       consoleVar ? "Error: Failed to Parse" : "Error: No such variable",
      //                                       consoleVar, false);
    }
  }
  
  void Process_runDebugConsoleFuncMessage(const Anki::Cozmo::RobotInterface::RunDebugConsoleFuncMessage& msg)
  {
    // We are using messages generated by the CppLite emitter here, which does not support
    // variable length arrays. CLAD also doesn't have a char, so the "strings" in this message
    // are actually arrays of uint8's. Thus we need to do this reinterpret cast here.
    // In some future world, ideally we avoid all this and use, for example, a web interface to
    // set/access console vars, instead of passing around via CLAD messages.
    const char* funcName  = reinterpret_cast<const char *>(msg.funcName);
    const char* funcArgs = reinterpret_cast<const char *>(msg.funcArgs);
    
    // TODO: Ideally, we'd send back a verify message that we (failed to) set this
    Anki::Util::IConsoleFunction* consoleFunc = Anki::Util::ConsoleSystem::Instance().FindFunction(funcName);
    if( consoleFunc ) {
      enum { kBufferSize = 512 };
      char buffer[kBufferSize];
      const uint32_t res = NativeAnkiUtilConsoleCallFunction( funcName, funcArgs, kBufferSize, buffer );
      PRINT_NAMED_INFO("Process_runDebugConsoleFuncMessage", "%s '%s' set to '%s'",
                       (res != 0) ? "Success" : "Failure", funcName, funcArgs);
    }
    else {
      PRINT_NAMED_WARNING("Process_runDebugConsoleFuncMessage.NoConsoleFunc", "No Func named '%s'",funcName);
    }
  }

// ========== END OF PROCESSING MESSAGES FROM ENGINE ==========



// ========== START OF PROCESSING MESSAGES FROM ROBOT ==========

  static void ProcessMicDataMessage(const RobotInterface::MicData& payload)
  {
    auto* micDataProcessor = _context->GetMicDataProcessor();
    if (micDataProcessor != nullptr)
    {
      micDataProcessor->ProcessMicDataPayload(payload);
    }
  }

  void ProcessMessageFromRobot(const RobotInterface::RobotToEngine& msg)
  {
    switch(msg.tag)
    {
      case RobotInterface::RobotToEngine::Tag_micData:
      {
        const auto& payload = msg.micData;
        ProcessMicDataMessage(payload);
        return;
      }
      break;
      case RobotInterface::RobotToEngine::Tag_state:
      {
        HandleRobotStateUpdate(msg.state);
      }
      break;
      default:
      {

      }
      break;
    }

    // Send up to engine
    const int tagSize = sizeof(msg.tag);
    SendToEngine(msg.GetBuffer()+tagSize, msg.Size()-tagSize, msg.tag);
  } // ProcessMessageFromRobot()

  void HandleRobotStateUpdate(const Anki::Cozmo::RobotState& robotState)
  {
    static bool buttonWasPressed = false;
    const auto buttonIsPressed = static_cast<bool>(robotState.status & (uint16_t)RobotStatusFlag::IS_BUTTON_PRESSED);
    const auto buttonReleased = buttonWasPressed && !buttonIsPressed;
    buttonWasPressed = buttonIsPressed;

    if (buttonReleased)
    {
      FaceDisplay::GetDebugDraw()->ChangeDrawState();
    }
  }


// ========== END OF PROCESSING MESSAGES FROM ROBOT ==========


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
    
    _context->GetMicDataProcessor()->Update();
    
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
        continue;
      }
      if (msgBuf.Size() != dataLen)
      {
        PRINT_NAMED_WARNING("Receiver.ReceiveData.SizeError", "Parsed message size error %d != %d (Tag %02x)", dataLen, msgBuf.Size(), msgBuf.tag);
        continue;
      }
      ProcessMessageFromRobot(msgBuf);
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
    // TODO: Don't need reliable transport between engine and anim process. Domain sockets should be good enough.
    //       For now, send everything unreliable.
    //const bool reliable = msgID < EnumToUnderlyingType(RobotInterface::ToRobotAddressSpace::TO_ENG_UNREL);
    const bool reliable = false;
    const bool hot = true;

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
    Anki::Cozmo::Messages::ProcessMessageFromEngine(msgBuf);
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

