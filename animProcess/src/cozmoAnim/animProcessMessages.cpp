/**
 * File: animProcessMessages.cpp
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

#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/animComms.h"
#include "cozmoAnim/robotDataLoader.h"

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animation/trackLayerComponent.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/audio/proceduralAudioClient.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animEngine.h"
#include "cozmoAnim/connectionFlow.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine_sendAnimToEngine_helper.h"
#include "clad/robotInterface/messageEngineToRobot_sendAnimToRobot_helper.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/factory/emrHelper.h"

#include "osState/osState.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include <unistd.h>

// Log options
#define LOG_CHANNEL    "AnimProcessMessages"

// Trace options
// #define LOG_TRACE(name, format, ...) LOG_DEBUG(name, format, ##__VA_ARGS__)
#define LOG_TRACE(name, format, ...) {}

// Anonymous namespace for private declarations
namespace {

  // For comms with engine
  constexpr int MAX_PACKET_BUFFER_SIZE = 2048;
  u8 pktBuffer_[MAX_PACKET_BUFFER_SIZE];

  Anki::Cozmo::AnimEngine*                   _animEngine = nullptr;
  Anki::Cozmo::AnimationStreamer*            _animStreamer = nullptr;
  Anki::Cozmo::Audio::EngineRobotAudioInput* _engAudioInput = nullptr;
  Anki::Cozmo::Audio::ProceduralAudioClient* _proceduralAudioClient = nullptr;
  const Anki::Cozmo::AnimContext*            _context = nullptr;

  CONSOLE_VAR(bool, kDebugFaceDraw_CycleWithButton, "DebugFaceDraw", true);

  static void ListAnimations(ConsoleFunctionContextRef context)
  {
    context->channel->WriteLog("<html>\n");
    context->channel->WriteLog("<h1>Animations</h1>\n");
    std::vector<std::string> names = _context->GetDataLoader()->GetAnimationNames();
    for(const auto& name : names) {
      std::string url = "consolefunccall?func=playanimation&args="+name+"+1";
      std::string html = "<a href=\""+url+"\">"+name+"</a>&nbsp\n";
      context->channel->WriteLog(html.c_str());
    }
    context->channel->WriteLog("</html>\n");
  }

  static void PlayAnimation(ConsoleFunctionContextRef context)
  {
    const char* name = ConsoleArg_Get_String(context, "name");
    if (name) {
      int numLoops = ConsoleArg_GetOptional_Int(context, "numLoops", 1);
      _animStreamer->SetStreamingAnimation(name, /*tag*/ 1, numLoops, /*interruptRunning*/ true);

      char numLoopsStr[4+1];
      snprintf(numLoopsStr, sizeof(numLoopsStr), "%d", (numLoops > 9999) ? 9999 : numLoops);
      std::string text = std::string("Playing ")+name+" "+numLoopsStr+" times<br>";
      context->channel->WriteLog(text.c_str());
    } else {
      context->channel->WriteLog("PlayAnimation name not specified.");
    }
  }

  static void AddAnimation(ConsoleFunctionContextRef context)
  {
    const char* path = ConsoleArg_Get_String(context, "path");
    if (path) {
      const std::string animationFolder = _context->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources, "/assets/animations/");
      std::string animationPath = animationFolder + path;

      _context->GetDataLoader()->LoadAnimationFile(animationPath.c_str());

      std::string text = "Adding animation ";
      text += animationPath;
      context->channel->WriteLog(text.c_str());
    } else {
      context->channel->WriteLog("PlayAnimation name not specified.");
    }
  }

  CONSOLE_FUNC(ListAnimations, "Animations");
  CONSOLE_FUNC(PlayAnimation, "Animations", const char* name, optional int numLoops);
  CONSOLE_FUNC(AddAnimation, "Animations", const char* path);

  void RecordMicDataClip(ConsoleFunctionContextRef context)
  {
    auto * micDataSystem = _context->GetMicDataSystem();
    if (micDataSystem != nullptr)
    {
      micDataSystem->SetForceRecordClip(true);
    }
  }
  CONSOLE_FUNC(RecordMicDataClip, "MicData");
}

namespace Anki {
namespace Cozmo {

// ========== START OF PROCESSING MESSAGES FROM ENGINE ==========
// #pragma mark "EngineToRobot Handlers"

void Process_lockAnimTracks(const Anki::Cozmo::RobotInterface::LockAnimTracks& msg)
{
  //LOG_DEBUG("AnimProcessMessages.Process_lockAnimTracks", "0x%x", msg.whichTracks);
  _animStreamer->SetLockedTracks(msg.whichTracks);
}

void Process_addAnim(const Anki::Cozmo::RobotInterface::AddAnim& msg)
{
  const std::string path(msg.animPath, msg.animPath_length);

  LOG_INFO("AnimProcessMessages.Process_playAnim",
           "Animation File: %s", path.c_str());

  _context->GetDataLoader()->LoadAnimationFile(path);
}

void Process_playAnim(const Anki::Cozmo::RobotInterface::PlayAnim& msg)
{
  const std::string animName(msg.animName, msg.animName_length);

  LOG_INFO("AnimProcessMessages.Process_playAnim",
           "Anim: %s, Tag: %d",
           animName.c_str(), msg.tag);

  _animStreamer->SetStreamingAnimation(animName, msg.tag, msg.numLoops);
}

void Process_abortAnimation(const Anki::Cozmo::RobotInterface::AbortAnimation& msg)
{
  LOG_INFO("AnimProcessMessages.Process_abortAnimation", "Abort animation");
  _animStreamer->Abort();
}

void Process_displayProceduralFace(const Anki::Cozmo::RobotInterface::DisplayProceduralFace& msg)
{
  ProceduralFace procFace;
  procFace.SetFromMessage(msg.faceParams);
  _animStreamer->SetProceduralFace(procFace, msg.duration_ms);
}

void Process_setFaceHue(const Anki::Cozmo::RobotInterface::SetFaceHue& msg)
{
  ProceduralFace::SetHue(msg.hue);
}

void Process_setFaceSaturation(const Anki::Cozmo::RobotInterface::SetFaceSaturation& msg)
{
  ProceduralFace::SetSaturation(msg.saturation);
}

void Process_displayFaceImageBinaryChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageBinaryChunk& msg)
{
  _animStreamer->Process_displayFaceImageChunk(msg);
}

void Process_displayFaceImageGrayscaleChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageGrayscaleChunk& msg)
{
  _animStreamer->Process_displayFaceImageChunk(msg);
}

void Process_displayFaceImageRGBChunk(const Anki::Cozmo::RobotInterface::DisplayFaceImageRGBChunk& msg)
{
  _animStreamer->Process_displayFaceImageChunk(msg);
}

void Process_displayCompositeImageChunk(const Anki::Cozmo::RobotInterface::DisplayCompositeImageChunk& msg)
{
  _animStreamer->Process_displayCompositeImageChunk(msg);
}

void Process_updateCompositeImageAsset(const Anki::Cozmo::RobotInterface::UpdateCompositeImageAsset& msg)
{
  _animStreamer->Process_updateCompositeImageAsset(msg);
}

void Process_enableKeepFaceAlive(const Anki::Cozmo::RobotInterface::EnableKeepFaceAlive& msg)
{
  _animStreamer->EnableKeepFaceAlive(msg.enable, msg.disableTimeout_ms);
}

void Process_setDefaultKeepFaceAliveParameters(const Anki::Cozmo::RobotInterface::SetDefaultKeepFaceAliveParameters& msg)
{
  _animStreamer->SetDefaultKeepFaceAliveParams();
}

void Process_setKeepFaceAliveParameter(const Anki::Cozmo::RobotInterface::SetKeepFaceAliveParameter& msg)
{
  if (msg.setToDefault) {
    _animStreamer->SetParamToDefault(msg.param);
  } else {
    _animStreamer->SetParam(msg.param, msg.value);
  }
}

void Process_addOrUpdateEyeShift(const Anki::Cozmo::RobotInterface::AddOrUpdateEyeShift& msg)
{
  const std::string layerName(msg.name, msg.name_length);
  _animStreamer->GetTrackLayerComponent()->AddOrUpdateEyeShift(layerName,
                                                               msg.xPix,
                                                               msg.yPix,
                                                               msg.duration_ms,
                                                               msg.xMax,
                                                               msg.yMax,
                                                               msg.lookUpMaxScale,
                                                               msg.lookDownMinScale,
                                                               msg.outerEyeScaleIncrease);
}

void Process_removeEyeShift(const Anki::Cozmo::RobotInterface::RemoveEyeShift& msg)
{
  const std::string layerName(msg.name, msg.name_length);
  _animStreamer->GetTrackLayerComponent()->RemoveEyeShift(layerName, msg.disableTimeout_ms);
}

void Process_addSquint(const Anki::Cozmo::RobotInterface::AddSquint& msg)
{
  const std::string layerName(msg.name, msg.name_length);
  _animStreamer->GetTrackLayerComponent()->AddSquint(layerName, msg.squintScaleX, msg.squintScaleY, msg.upperLidAngle);
}

void Process_removeSquint(const Anki::Cozmo::RobotInterface::RemoveSquint& msg)
{
  const std::string layerName(msg.name, msg.name_length);
  _animStreamer->GetTrackLayerComponent()->RemoveSquint(layerName, msg.disableTimeout_ms);
}

void Process_postAudioEvent(const Anki::AudioEngine::Multiplexer::PostAudioEvent& msg)
{
  _engAudioInput->HandleMessage(msg);
}

void Process_stopAllAudioEvents(const Anki::AudioEngine::Multiplexer::StopAllAudioEvents& msg)
{
  _engAudioInput->HandleMessage(msg);
}

void Process_postAudioGameState(const Anki::AudioEngine::Multiplexer::PostAudioGameState& msg)
{
  _engAudioInput->HandleMessage(msg);
}

void Process_postAudioSwitchState(const Anki::AudioEngine::Multiplexer::PostAudioSwitchState& msg)
{
  _engAudioInput->HandleMessage(msg);
}

void Process_postAudioParameter(const Anki::AudioEngine::Multiplexer::PostAudioParameter& msg)
{
  _engAudioInput->HandleMessage(msg);
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
  if (consoleVar && consoleVar->ParseText(tryValue))
  {
    //SendVerifyDebugConsoleVarMessage(_externalInterface, varName, consoleVar->ToString().c_str(), consoleVar, true);
    LOG_INFO("AnimProcessMessages.Process_setDebugConsoleVarMessage.Success", "'%s' set to '%s'", varName, tryValue);
  }
  else
  {
    LOG_WARNING("AnimProcessMessages.Process_setDebugConsoleVarMessage.Fail", "Error setting '%s' to '%s'",
                varName, tryValue);
    //      SendVerifyDebugConsoleVarMessage(_externalInterface, msg.varName.c_str(),
    //                                       consoleVar ? "Error: Failed to Parse" : "Error: No such variable",
    //                                       consoleVar, false);
  }
}

void Process_startRecordingMics(const Anki::Cozmo::RobotInterface::StartRecordingMics& msg)
{
  auto* micDataSystem = _context->GetMicDataSystem();
  if (micDataSystem == nullptr)
  {
    return;
  }

  micDataSystem->RecordRawAudio(msg.duration_ms,
                                std::string(msg.path,
                                            msg.path_length),
                                msg.runFFT);
}

void Process_drawTextOnScreen(const Anki::Cozmo::RobotInterface::DrawTextOnScreen& msg)
{
  FaceInfoScreenManager::getInstance()->SetCustomText(msg);
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
  if (consoleFunc) {
    enum { kBufferSize = 512 };
    char buffer[kBufferSize];
    const uint32_t res = NativeAnkiUtilConsoleCallFunction(funcName, funcArgs, kBufferSize, buffer);
    LOG_INFO("AnimProcessMessages.Process_runDebugConsoleFuncMessage", "%s '%s' set to '%s'",
                     (res != 0) ? "Success" : "Failure", funcName, funcArgs);
  }
  else
  {
    LOG_WARNING("AnimProcessMessages.Process_runDebugConsoleFuncMessage.NoConsoleFunc", "No Func named '%s'",funcName);
  }
}

void Process_textToSpeechPrepare(const RobotInterface::TextToSpeechPrepare& msg)
{
  _animEngine->HandleMessage(msg);
}

void Process_textToSpeechDeliver(const RobotInterface::TextToSpeechDeliver& msg)
{
  _animEngine->HandleMessage(msg);
}

void Process_textToSpeechCancel(const RobotInterface::TextToSpeechCancel& msg)
{
  _animEngine->HandleMessage(msg);
}

void Process_setConnectionStatus(const Anki::Cozmo::SwitchboardInterface::SetConnectionStatus& msg)
{
  UpdateConnectionFlow(std::move(msg), _animStreamer, _context);
}

void Process_setBLEPin(const Anki::Cozmo::SwitchboardInterface::SetBLEPin& msg)
{
  SetBLEPin(msg.pin);
}

void AnimProcessMessages::ProcessMessageFromEngine(const RobotInterface::EngineToRobot& msg)
{
  //LOG_WARNING("AnimProcessMessages.ProcessMessageFromEngine", "%d", msg.tag);
  bool forwardToRobot = false;
  switch (msg.tag)
  {
    case RobotInterface::EngineToRobot::Tag_absLocalizationUpdate:
    {
      forwardToRobot = true;
      _context->GetMicDataSystem()->ResetMicListenDirection();
      break;
    }

#include "clad/robotInterface/messageEngineToRobot_switch_from_0x50_to_0xAF.def"

    default:
      forwardToRobot = true;
      break;
  }

  if (forwardToRobot) {
    // Send message along to robot if it wasn't handled here
    AnimComms::SendPacketToRobot((char*)msg.GetBuffer(), msg.Size());
  }

} // ProcessMessageFromEngine()


// ========== END OF PROCESSING MESSAGES FROM ENGINE ==========


// ========== START OF PROCESSING MESSAGES FROM ROBOT ==========
// #pragma mark "RobotToEngine handlers"

static void ProcessMicDataMessage(const RobotInterface::MicData& payload)
{
  FaceInfoScreenManager::getInstance()->DrawMicInfo(payload);

  auto * micDataSystem = _context->GetMicDataSystem();
  if (micDataSystem != nullptr)
  {
    micDataSystem->ProcessMicDataPayload(payload);
  }
}

static void HandleRobotStateUpdate(const Anki::Cozmo::RobotState& robotState)
{
  FaceInfoScreenManager::getInstance()->Update(robotState);

#if ANKI_DEV_CHEATS
  auto * micDataSystem = _context->GetMicDataSystem();
  if (micDataSystem != nullptr)
  {
    const auto liftHeight_mm = ConvertLiftAngleToLiftHeightMM(robotState.liftAngle);
    const bool isMicFace = FaceInfoScreenManager::getInstance()->GetCurrScreenName() == ScreenName::MicDirectionClock;
    if (isMicFace && LIFT_HEIGHT_CARRY-1.f <= liftHeight_mm)
    {
      micDataSystem->SetForceRecordClip(true);
    }
  }
#endif
}

void AnimProcessMessages::ProcessMessageFromRobot(const RobotInterface::RobotToEngine& msg)
{
  const auto tag = msg.tag;
  switch (tag)
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

  // Forward to engine
  SendAnimToEngine(msg);

} // ProcessMessageFromRobot()

// ========== END OF PROCESSING MESSAGES FROM ROBOT ==========

// ========== START OF CLASS METHODS ==========
// #pragma mark "Class methods"

Result AnimProcessMessages::Init(AnimEngine* animEngine,
                                 AnimationStreamer* animStreamer,
                                 Audio::EngineRobotAudioInput* audioInput,
                                 const AnimContext* context)
{
  // Preconditions
  DEV_ASSERT(nullptr != animEngine, "AnimProcessMessages.Init.InvalidAnimEngine");
  DEV_ASSERT(nullptr != animStreamer, "AnimProcessMessages.Init.InvalidAnimStreamer");
  DEV_ASSERT(nullptr != audioInput, "AnimProcessMessages.Init.InvalidAudioInput");
  DEV_ASSERT(nullptr != context, "AnimProcessMessages.Init.InvalidAnimContext");

  // Setup robot and engine sockets
  AnimComms::InitComms();

  _animEngine             = animEngine;
  _animStreamer           = animStreamer;
  _proceduralAudioClient  = _animStreamer->GetProceduralAudioClient();
  _engAudioInput          = audioInput;
  _context                = context;

  InitConnectionFlow(_animStreamer);

  return RESULT_OK;
}

Result AnimProcessMessages::MonitorConnectionState(void)
{
  // Send block connection state when engine connects
  static bool wasConnected = false;
  if (!wasConnected && AnimComms::IsConnectedToEngine()) {
    LOG_INFO("AnimProcessMessages.MonitorConnectionState", "Robot now available");
    RobotInterface::RobotAvailable idMsg;
    idMsg.hwRevision = 0;
    idMsg.serialNumber = OSState::getInstance()->GetSerialNumber();
    RobotInterface::SendAnimToEngine(idMsg);

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
      RobotInterface::SendAnimToEngine(msg);
    }

    wasConnected = true;
  }
  else if (wasConnected && !AnimComms::IsConnectedToEngine()) {
    wasConnected = false;
  }

  return RESULT_OK;

}

Result AnimProcessMessages::Update(BaseStationTime_t currTime_nanosec)
{
  if (!AnimComms::IsConnectedToRobot()) {
    LOG_WARNING("AnimProcessMessages.Update", "No connection to robot");
    return RESULT_FAIL_IO_CONNECTION_CLOSED;
  }

  MonitorConnectionState();

  _context->GetMicDataSystem()->Update(currTime_nanosec);

  // Process incoming messages from engine
  u32 dataLen;

  // Process messages from engine
  while((dataLen = AnimComms::GetNextPacketFromEngine(pktBuffer_, MAX_PACKET_BUFFER_SIZE)) > 0)
  {
    Anki::Cozmo::RobotInterface::EngineToRobot msg;
    memcpy(msg.GetBuffer(), pktBuffer_, dataLen);
    if (msg.Size() != dataLen) {
      LOG_WARNING("AnimProcessMessages.Update.EngineToRobot.InvalidSize",
                  "Invalid message size from engine (%d != %d)",
                  msg.Size(), dataLen);
      continue;
    }
    if (!msg.IsValid()) {
      LOG_WARNING("AnimProcessMessages.Update.EngineToRobot.InvalidData", "Invalid message from engine");
      continue;
    }
    ProcessMessageFromEngine(msg);
  }

  // Process messages from robot
  while ((dataLen = AnimComms::GetNextPacketFromRobot(pktBuffer_, MAX_PACKET_BUFFER_SIZE)) > 0)
  {
    Anki::Cozmo::RobotInterface::RobotToEngine msg;
    memcpy(msg.GetBuffer(), pktBuffer_, dataLen);
    if (msg.Size() != dataLen) {
      LOG_WARNING("AnimProcessMessages.Update.RobotToEngine.InvalidSize",
                  "Invalid message size from robot (%d != %d)",
                  msg.Size(), dataLen);
      continue;
    }
    if (!msg.IsValid()) {
      LOG_WARNING("AnimProcessMessages.Update.RobotToEngine.InvalidData", "Invalid message from robot");
      continue;
    }
    ProcessMessageFromRobot(msg);
    _proceduralAudioClient->ProcessMessage(msg);
  }

#if FACTORY_TEST
#if defined(SIMULATOR)
  // Simulator never has EMR
  FaceInfoScreenManager::getInstance()->SetShouldDrawFAC(false);
#else
  FaceInfoScreenManager::getInstance()->SetShouldDrawFAC(!Factory::GetEMR()->fields.PACKED_OUT_FLAG);
#endif
#endif

  return RESULT_OK;
}

bool AnimProcessMessages::SendAnimToRobot(const RobotInterface::EngineToRobot& msg)
{
  LOG_TRACE("AnimProcessMessages.SendAnimToRobot", "Send tag %d size %u", msg.tag, msg.Size());
  return AnimComms::SendPacketToRobot(msg.GetBuffer(), msg.Size());
}

bool AnimProcessMessages::SendAnimToEngine(const RobotInterface::RobotToEngine & msg)
{
  LOG_TRACE("AnimProcessMessages.SendAnimToEngine", "Send tag %d size %u", msg.tag, msg.Size());
  return AnimComms::SendPacketToEngine(msg.GetBuffer(), msg.Size());
}

} // namespace Cozmo
} // namespace Anki
