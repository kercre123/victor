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
#include "util/fileUtils/fileUtils.h"
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

    u32 _serialNumber = 0;

    // Whether or not we are currently showing debug info on the screen
    enum DebugScreen {
      NONE,
      GENERAL_INFO,
      SENSORS1,
      SENSORS2,
      COUNT
    };
    DebugScreen _curDebugScreen = DebugScreen::NONE;

    bool _haveBirthCert = false;
    const u8 kNumTicksToCheckForBC = 60; // ~2seconds
    u8 _bcCheckCount = 0;

    struct DebugScreenInfo {
      std::string ip          = "0.0.0.0";
      std::string serialNo    = "0";
      std::string cliffs      = "0 0 0 0";
      std::string motors      = "0 0 0 0";
      std::string touchAndBat = "0 0.0v";
      std::string prox        = "0 0 0 0";
      std::string accelGyroX  = "0 0";
      std::string accelGyroY  = "0 0";
      std::string accelGyroZ  = "0 0";

      void Update(const RobotState& state)
      {
        char temp[32] = "";
        sprintf(temp, 
                "%u %u %u %u", 
                state.cliffDataRaw[0], 
                state.cliffDataRaw[1], 
                state.cliffDataRaw[2], 
                state.cliffDataRaw[3]);
        cliffs = temp;

        sprintf(temp, 
                "%.2f %.2f %.1f %.1f", 
                state.headAngle, 
                state.liftAngle, 
                state.lwheel_speed_mmps, 
                state.rwheel_speed_mmps);
        motors = temp;

        sprintf(temp, 
                "%u %0.2fv", 
                state.backpackTouchSensorRaw,
                state.batteryVoltage);
        touchAndBat = temp;

        sprintf(temp, 
                "%.1f %.1f %.1f %u", 
                state.proxData.signalIntensity,
                state.proxData.ambientIntensity,
                state.proxData.spadCount,
                state.proxData.distance_mm);
        prox = temp;

        sprintf(temp, 
                "%*.2f %*.2f", 
                8,
                state.accel.x,
                8, 
                state.gyro.x);
        accelGyroX = temp;

        sprintf(temp, 
                "%*.2f %*.2f",
                8, 
                state.accel.y,
                8, 
                state.gyro.y);
        accelGyroY = temp;

        sprintf(temp, 
                "%*.2f %*.2f", 
                8,
                state.accel.z,
                8,
                state.gyro.z);
        accelGyroZ = temp;
      }

      void ToVec(DebugScreen whichScreen, std::vector<std::string>& vec)
      {
        vec.clear();

        switch(whichScreen)
        {
          case DebugScreen::NONE:
          {
            break;
          }
          case DebugScreen::GENERAL_INFO:
          {
            vec.push_back(ip);
            vec.push_back(serialNo);
            break;
          }
          case DebugScreen::SENSORS1:
          {
            vec.push_back(cliffs);
            vec.push_back(motors);
            vec.push_back(prox);
            vec.push_back(touchAndBat);
            break;
          }
          case DebugScreen::SENSORS2:
          {
            vec.push_back(accelGyroX);
            vec.push_back(accelGyroY);
            vec.push_back(accelGyroZ);
            break;
          }
          case DebugScreen::COUNT:
          {
            break;
          }
        }
      }
    } _debugScreenInfo;

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

  std::string GetIPAddress();
  std::string ExecCommand(const char* cmd);

  void UpdateFAC();
  void UpdateDebugScreen();

  
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

    _haveBirthCert = Util::FileUtils::FileExists("/data/persist/factory/80000000.nvdata");

    UpdateFAC();

    std::string serialString = ExecCommand("getprop ro.serialno");
    if(!serialString.empty())
    {
      _serialNumber = static_cast<u32>(std::stoul(serialString, nullptr, 16));
    }
    
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

  void UpdateFAC()
  {
    if(!_haveBirthCert)
    {
      _animStreamer->LockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);

      DrawTextOnScreen({"FAC"},
                       NamedColors::BLACK,
                       NamedColors::RED,
                       { 0, FACE_DISPLAY_HEIGHT-10 });
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
    _curDebugScreen = DebugScreen::NONE;

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
      case RobotInterface::RobotToEngine::Tag_state:
      {
        _debugScreenInfo.Update(msg.state);
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
      idMsg.serialNumber = _serialNumber;
      
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

    // Do after message processing so we draw the latest state info
    UpdateDebugScreen();

    if(++_bcCheckCount >= kNumTicksToCheckForBC)
    {
      _bcCheckCount = 0;
      _haveBirthCert = Util::FileUtils::FileExists("/data/persist/factory/80000000.nvdata");
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

  std::string GetIPAddress()
  {
    // Open a socket to figure out the ip adress of the wlan0 (wifi) interface
      const char* const if_name = "wlan0";
      struct ifreq ifr;
      size_t if_name_len=strlen(if_name);
      if (if_name_len<sizeof(ifr.ifr_name)) {
        memcpy(ifr.ifr_name,if_name,if_name_len);
        ifr.ifr_name[if_name_len]=0;
      } else {
        ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.InvalidInterfaceName", "");
      }

      int fd=socket(AF_INET,SOCK_DGRAM,0);
      if (fd==-1) {
        ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.OpenSocketFail", "");
      }

      if (ioctl(fd,SIOCGIFADDR,&ifr)==-1) {
        int temp_errno=errno;
        close(fd);
        ASSERT_NAMED_EVENT(false, "EngineMessages.GetIPAddress.IoctlError", "%s", strerror(temp_errno));
      }
      close(fd);

      struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
      return std::string(inet_ntoa(ipaddr->sin_addr));
  }

  // Executes the provided command and returns the output as a string
  std::string ExecCommand(const char* cmd) 
  {
    try
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
    catch(...)
    {
      return "";
    }
  }

  void ProcessBackpackButton(const RobotInterface::BackpackButton& payload)
  {
    if((ANKI_DEV_CHEATS || FACTORY_TEST) && payload.depressed)
    {
      _curDebugScreen = (DebugScreen)(((int)_curDebugScreen) + 1);
      
      if(_curDebugScreen == DebugScreen::COUNT)
      {
        _curDebugScreen = DebugScreen::NONE;
      }

      // If we aren't currently showing debug info and the button was pressed then
      // start showing debug info
      if(_curDebugScreen != DebugScreen::NONE)
      {
        // Lock the face image track to prevent animations from drawing over the debug info
        _animStreamer->LockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);

        UpdateDebugScreen();
        _debugScreenInfo.ip = GetIPAddress();

        _debugScreenInfo.serialNo = ExecCommand("getprop ro.serialno");

        // Draw the last three digits of the ip on the screen
        DrawTextOnScreen({_debugScreenInfo.ip, _debugScreenInfo.serialNo}, 
                         NamedColors::WHITE, NamedColors::BLACK, {0, 30}, 20, 0.5f);


      }
      // Otherwise we are currently showing debug info and the button has been pressed
      // so clear the face and unlock the face image track
      else
      {
        _animStreamer->UnlockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);
        FaceDisplay::getInstance()->FaceClear();

        UpdateFAC();
      }
    }
  }

  void UpdateDebugScreen()
  {
    if(_curDebugScreen != DebugScreen::NONE)
    {
      std::vector<std::string> text;
      _debugScreenInfo.ToVec(_curDebugScreen, text);
      DrawTextOnScreen(text, NamedColors::WHITE, NamedColors::BLACK, {0, 30}, 20, 0.5f);
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

    static Array2d<u16> img565(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    _animStreamer->DrawToFace(resultImg, img565);
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
