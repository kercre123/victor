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
#include "cozmoAnim/micDataProcessor.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"

#include "anki/common/basestation/math/rect_impl.h"
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

#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

// For drawing images/text to the face
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <string.h>
#include <stdio.h>
#include <math.h>

// For getting our ip address
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

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
    
    
    const u32 kMaxNumAvailableAnimsToReportPerTic = 100;
    
    // The last AnimID that was sent to engine in response to RequestAvailableAnimations.
    // If negative, it means we're not currently doling.
    bool _isDolingAnims = false;
    u32 _nextAnimIDToDole;

    // Whether or not we are currently showing debug info on the screen
    bool _showingDebugInfo = false;
    
  } // private namespace


  // Forward declarations
  extern "C" TimeStamp_t GetTimeStamp(void);
  
  void ProcessMessageFromEngine(const RobotInterface::EngineToRobot& msg);
  void ProcessMessageFromRobot(const RobotInterface::RobotToEngine& msg);
  extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);
  void ProcessBackpackButton(const RobotInterface::BackpackButton& payload);
  void DrawTextOnScreen(const std::vector<std::string>& textVec, 
                        const ColorRGBA& textColor,
                        const ColorRGBA& bgColor,
                        const Point2f& loc = {0, 0},
                        u32 textSpacing_pix = 10,
                        f32 textScale = 3.f);
  std::string ExecCommand(const char* cmd);

  
// #pragma mark --- Messages Method Implementations ---

  Result Init(AnimationStreamer& animStreamer, Audio::EngineRobotAudioInput& audioInput, const CozmoAnimContext& context)
  {
    
    // Setup robot and engine sockets
    CozmoAnimComms::InitComms();
    
    // Setup reliable transport
    ReliableTransport_Init();
    ReliableConnection_Init(&connection, NULL); // We only have one connection so dest pointer is superfluous

    _animStreamer = &animStreamer;
    _audioInput   = &audioInput;
    _context      = &context;
    
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
    PRINT_NAMED_INFO("EngineMesssages.Process_playAnim",
                     "AnimID: %d, Tag: %d",
                     msg.animID, msg.tag);
    
    _animStreamer->SetStreamingAnimation(msg.animID, msg.tag, msg.numLoops);
  }

  void Process_abortAnimation(const Anki::Cozmo::RobotInterface::AbortAnimation& msg)
  {
    PRINT_NAMED_WARNING("EngineMessages.Process_abortAnimation.NotHookedup",
                        "Tag: %d",
                        msg.tag);
    
    // TODO: Need to hook this up to AnimationStreamer
    //       Maybe _animStreamer->Abort(msg.abortAnimation.tag)?
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
  
  void Process_requestAvailableAnimations(const Anki::Cozmo::RobotInterface::RequestAvailableAnimations& msg)
  {
    PRINT_NAMED_INFO("EngineMessages.Process_requestAvailableAnimations", "");
    if (!_isDolingAnims) {
      const auto& animIDToNameMap = _animStreamer->GetCannedAnimationContainer().GetAnimationIDToNameMap();
      if (!animIDToNameMap.empty()) {
        _nextAnimIDToDole =  animIDToNameMap.begin()->first;
        _isDolingAnims = true;
      } else {
        PRINT_NAMED_WARNING("EngineMessages.Process_requestAvailableAnimations.NoAnimsAvailable", "");
      }
    } else {
      PRINT_NAMED_WARNING("EngineMessages.Process_requestAvailableAnimations.AlreadyDoling", "");
    }
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

  void Process_startRecordingMics(const Anki::Cozmo::RobotInterface::StartRecordingMics& msg)
  {
    auto* micDataProcessor = _context->GetMicDataProcessor();
    if (micDataProcessor == nullptr)
    {
      return;
    }

    micDataProcessor->RecordRawAudio(msg.duration_ms,
                                     std::string(msg.path, 
                                                 msg.path_length),
                                     msg.runFFT);
  }

  void Process_drawTextOnScreen(const Anki::Cozmo::RobotInterface::DrawTextOnScreen& msg)
  {
    DrawTextOnScreen({std::string(msg.text,
                                  msg.text_length)},
                     ColorRGBA(msg.textColor.r,
                               msg.textColor.g,
                               msg.textColor.b),
                     ColorRGBA(msg.bgColor.r,
                               msg.bgColor.g,
                               msg.bgColor.b),
                     { 0, FACE_DISPLAY_HEIGHT-10 });
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
  
// ========== END OF PROCESSING MESSAGES FROM ENGINE ==========



// ========== START OF PROCESSING MESSAGES FROM ROBOT ==========

  static void ProcessMicDataMessage(const RobotInterface::MicData& payload)
  {
    auto* micDataProcessor = _context->GetMicDataProcessor();
    if (micDataProcessor == nullptr)
    {
      return;
    }

    static uint32_t sLatestSequenceID = 0;
    
    // Since mic data is sent unreliably, make sure the sequence id increases appropriately
    if (payload.sequenceID > sLatestSequenceID ||
        (sLatestSequenceID - payload.sequenceID) > (UINT32_MAX / 2)) // To handle rollover case
    {
      sLatestSequenceID = payload.sequenceID;
      micDataProcessor->ProcessNextAudioChunk(payload.data);
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
      case RobotInterface::RobotToEngine::Tag_backpackButton:
      {
        ProcessBackpackButton(msg.backpackButton);
        // Break and forward message to engine
        break;
      } 
      default:
      {
        break;
      }
    }

    // Send up to engine
    const int tagSize = sizeof(msg.tag);
    SendToEngine(msg.GetBuffer()+tagSize, msg.Size()-tagSize, msg.tag);
  } // ProcessMessageFromRobot()


// ========== END OF PROCESSING MESSAGES FROM ROBOT ==========


// #pragma --- Message Dispatch Functions ---

  Result MonitorConnectionState(void)
  {
    // Send block connection state when engine connects
    static bool wasConnected = false;
    if (!wasConnected && CozmoAnimComms::EngineIsConnected()) {
      
      RobotInterface::RobotAvailable idMsg;
      idMsg.hwRevision = 0;
      idMsg.serialNumber = std::stoul(ExecCommand("getprop ro.serialno"), nullptr, 16);
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

  // Executes the provided command and returns the output as a string
  std::string ExecCommand(const char* cmd) 
  {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
      PRINT_NAMED_WARNING("EngineMessages.ExecCommand.FailedToOpenPipe", "");
      return "";
    }

    while (!feof(pipe.get()))
    {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      {
        result += buffer.data();
      }
    }

    // Remove the last character as it is a newline
    return result.substr(0,result.size()-1);
  }

  void ProcessBackpackButton(const RobotInterface::BackpackButton& payload)
  {
    if((ANKI_DEV_CHEATS || FACTORY_TEST) && payload.depressed)
    {
      // If we aren't currently showing debug info and the button was pressed then
      // start showing debug info
      if(!_showingDebugInfo)
      {
        // Lock the face image track to prevent animations from drawing over the debug info
        _animStreamer->LockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);

        // Open a socket to figure out the ip adress of the wlan0 (wifi) interface
        const char* const if_name = "wlan0";
        struct ifreq ifr;
        size_t if_name_len=strlen(if_name);
        if (if_name_len<sizeof(ifr.ifr_name)) {
          memcpy(ifr.ifr_name,if_name,if_name_len);
          ifr.ifr_name[if_name_len]=0;
        } else {
          ASSERT_NAMED_EVENT(false, "ProcessMessageToEngine.BackpackButton.InvalidInterfaceName", "");
        }

        int fd=socket(AF_INET,SOCK_DGRAM,0);
        if (fd==-1) {
          ASSERT_NAMED_EVENT(false, "ProcessMessageToEngine.BackpackButton.OpenSocketFail", "");
        }

        if (ioctl(fd,SIOCGIFADDR,&ifr)==-1) {
          int temp_errno=errno;
          close(fd);
          ASSERT_NAMED_EVENT(false, "ProcessMessageToEngine.BackpackButton.IoctlError", "%s", strerror(temp_errno));
        }
        close(fd);

        struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
        const std::string ip = std::string(inet_ntoa(ipaddr->sin_addr));

        const std::string serialNo = ExecCommand("getprop ro.serialno");

        // Draw the last three digits of the ip on the screen
        DrawTextOnScreen({ip, serialNo}, NamedColors::WHITE, NamedColors::BLACK, {0, 30}, 20, 0.5f);


      }
      // Otherwise we are currently showing debug info and the button has been pressed
      // so clear the face and unlock the face image track
      else
      {
        _animStreamer->UnlockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);
        FaceDisplay::getInstance()->FaceClear();
      }
      _showingDebugInfo = !_showingDebugInfo;
    }
  }

  // Draws each element of the textVec on a separate line (spacing determined by textSpacing_pix)
  // in textColor with a background of bgColor.
  void DrawTextOnScreen(const std::vector<std::string>& textVec, 
                        const ColorRGBA& textColor,
                        const ColorRGBA& bgColor,
                        const Point2f& loc,
                        u32 textSpacing_pix,
                        f32 textScale)
  {
    Vision::ImageRGB resultImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    
    Anki::Rectangle<f32> rect(0, 0, FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);
    resultImg.DrawFilledRect(rect, bgColor);

    const f32 textLocX = loc.x();
    f32 textLocY = loc.y();
    // TODO: Expose line and location(?) as arguments
    const u8  textLineThickness = 8;

    for(const auto& text : textVec)
    {
      resultImg.DrawText({textLocX, textLocY},
                         text.c_str(),
                         textColor,
                         textScale,
                         textLineThickness);

      textLocY += textSpacing_pix;
    }

    // Draw the word "Factory" in the top right corner if this is a 
    // factory build
    if(FACTORY_TEST)
    {
      const Point2f factoryTextLoc = {0, 10};
      const f32 factoryScale = 0.5f;
      resultImg.DrawText(factoryTextLoc,
                         "Factory",
                         NamedColors::WHITE,
                         factoryScale);
    }

    // Convert the RGB888 image to RGB565
    cv::Mat img565(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH, CV_16U);
    u16* row565;
    const Vision::PixelRGB* rowRGB;
    for(int i = 0; i < img565.rows; ++i)
    {
      row565 = img565.ptr<u16>(i);
      rowRGB = resultImg.get_CvMat_().ptr<Vision::PixelRGB>(i);
      for(int j = 0; j < img565.cols; ++j)
      {
        row565[j] = (((int)(rowRGB[j].r() >> 3) << 11) | 
                     ((int)(rowRGB[j].g() >> 2) << 5)  | 
                     ((int)(rowRGB[j].b() >> 3) << 0));
        // Swap byte order
        row565[j] = ((row565[j]>>8)&0xFF) | ((row565[j]&0xFF)<<8);
      }
    }

    FaceDisplay::getInstance()->FaceDraw(reinterpret_cast<u16*>(img565.ptr()));
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
