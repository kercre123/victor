/**
* File: voiceCommandComponent.cpp
*
* Author: Lee Crippen
* Created: 2/8/2017
*
* Description: Component handling listening for voice commands, controlling recognizer
* listen context, and broadcasting command events.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"

#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.H"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "audioUtil/audioRecognizerProcessor.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
#include "clad/types/voiceCommandTypes.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

namespace VoiceCommandConsoleVars{
  CONSOLE_VAR(bool, kShouldForceTrigger, "VoiceCommands", false);
  CONSOLE_VAR(u32, kForceTriggerFreq_s, "VoiceCommands", 30);
}
using namespace VoiceCommandConsoleVars;


namespace Anki {
namespace Cozmo {
  
VoiceCommandComponent::VoiceCommandComponent(const CozmoContext& context)
: _context(context)
, _platform(*(_context.GetDataPlatform()))
, _recogProcessor(new AudioUtil::AudioRecognizerProcessor{})
{
  if (!_recogProcessor->IsValid())
  {
    _recogProcessor.reset();
    LOG_INFO("VoiceCommandComponent.RecogProcessorSetupFail", "Failure often means permission to use mic was denied.");
  }
  else
  {
    SpeechRecognizerTHF* newRecognizer = new SpeechRecognizerTHF();
    
    // TODO: Need to be loading in phrases from data, probably json
    constexpr unsigned int numTestPhrases = 1;
    const char* testPhraseList[numTestPhrases] = { "HeyCuzmo" };
    newRecognizer->Init(testPhraseList, numTestPhrases);
    
    _recognizer.reset(newRecognizer);
    _recogProcessor->SetSpeechRecognizer(_recognizer.get());
    _recogProcessor->Start();
  }
}
  
VoiceCommandComponent::~VoiceCommandComponent() = default;
  
template<typename T>
void VoiceCommandComponent::BroadcastVoiceEvent(T event)
{
  auto* robot = _context.GetRobotManager()->GetFirstRobot();
  if (robot && robot->HasExternalInterface())
  {
    robot->GetExternalInterface()->BroadcastToGame<VoiceCommandEvent>(VoiceCommandEventUnion(std::move(event)));
  }
}
  
void VoiceCommandComponent::Update()
{
  bool heardPrimaryTrigger = false;
  while (_recogProcessor->HasResults())
  {
    const auto& nextResult = _recogProcessor->PopNextResult();
    LOG_INFO("VoiceCommandComponent.HeardCommand", "%s", nextResult.c_str());
    
    // TODO: Here we will do some screening of the next result, including checking whether it's the primary trigger.
    // For now we're only listening for the primary trigger so we know that's what we heard.
    heardPrimaryTrigger = true;
    BroadcastVoiceEvent(CommandHeardEvent(VoiceCommandType::HEY_COZMO));
  }
  
  if (ANKI_CONSOLE_SYSTEM_ENABLED)
  {
    if (kShouldForceTrigger &&
        (static_cast<uint32_t>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) % kForceTriggerFreq_s) == 0)
    {
      LOG_INFO("VoiceCommandComponent.ForceTrigger","");
      heardPrimaryTrigger = true;
    }
  }
  
  UpdateCommandLight(heardPrimaryTrigger);
}
  
void VoiceCommandComponent::UpdateCommandLight(bool heardTriggerPhrase)
{
  auto* robot = _context.GetRobotManager()->GetFirstRobot();
  
  if (_commandLightTimeRemaining_s >= 0.f)
  {
    auto timePassedSinceLastUpdate_s = BaseStationTimer::getInstance()->GetTimeSinceLastTickInSeconds();
    _commandLightTimeRemaining_s -= timePassedSinceLastUpdate_s;
    if (_commandLightTimeRemaining_s < 0.f && robot)
    {
      robot->GetBodyLightComponent().StopLoopingBackpackLights(_bodyLightDataLocator);
    }
  }
  
  if (heardTriggerPhrase && robot)
  {
    static const BackpackLights kHeardCommandLights = {
      .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLUE, NamedColors::BLACK}},
      .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
      .onPeriod_ms            = {{0,0,0,1450,0}},
      .offPeriod_ms           = {{0,0,0,1000,0}},
      .transitionOnPeriod_ms  = {{0,0,0,50,0}},
      .transitionOffPeriod_ms = {{0,0,0,500,0}},
      .offset                 = {{0,0,0,0,0}}
    };
    
    static constexpr auto kIndexOfVoiceLight = Util::EnumToUnderlying(LEDId::LED_BACKPACK_BACK);
    
    // Add up the time we transition on, stay on, and transition off to figure out how long we should display.
    static const float kTimeForCommandLight_s = Util::numeric_cast<float>(0.001f * (100 + // Add in a little time for the light to be off
                                                                                    kHeardCommandLights.onPeriod_ms[kIndexOfVoiceLight] +
                                                                                    kHeardCommandLights.transitionOnPeriod_ms[kIndexOfVoiceLight] +
                                                                                    kHeardCommandLights.transitionOffPeriod_ms[kIndexOfVoiceLight]));
    
    _commandLightTimeRemaining_s = kTimeForCommandLight_s;
    _bodyLightDataLocator = robot->GetBodyLightComponent().StartLoopingBackpackLights(kHeardCommandLights, BackpackLightSource::Voice);
  }
}

} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

