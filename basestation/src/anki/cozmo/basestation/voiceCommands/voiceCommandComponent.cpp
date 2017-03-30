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

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"

#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.h"
#include "anki/common/basestation/utils/timer.h"
#include "audioUtil/audioRecognizerProcessor.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"

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
, _recogProcessor(new AudioUtil::AudioRecognizerProcessor{})
, _phraseData(new CommandPhraseData())
{
  if (!_recogProcessor->IsValid())
  {
    _recogProcessor.reset();
    LOG_INFO("VoiceCommandComponent.RecogProcessorSetupFail", "Failure often means permission to use mic was denied.");
  }
}
  
VoiceCommandComponent::~VoiceCommandComponent() = default;

void VoiceCommandComponent::Init()
{
  if (_recogProcessor == nullptr)
  {
    PRINT_NAMED_WARNING("VoiceCommandComponent.Init", "Can't Init voice command component with an invalid audio processor.");
    return;
  }
  
  const auto* dataLoader = _context.GetDataLoader();
  if (ANKI_VERIFY(dataLoader != nullptr, "VoiceCommandComponent.Init", "DataLoader missing"))
  {
    ANKI_VERIFY(_phraseData->Init(dataLoader->GetVoiceCommandConfig()), "VoiceCommandComponent.Init", "CommandPhraseData Init failed.");
    std::vector<const char*> phraseList = _phraseData->GetPhraseListRaw();
    SpeechRecognizerTHF* newRecognizer = new SpeechRecognizerTHF();
    
    newRecognizer->Init(phraseList.data(), Util::numeric_cast<unsigned int>(phraseList.size()));
    
    _recognizer.reset(newRecognizer);
    _recogProcessor->SetSpeechRecognizer(_recognizer.get());
    _recogProcessor->Start();
  }
}

template<typename T>
void VoiceCommandComponent::BroadcastVoiceEvent(T&& event)
{
  auto* externalInterface = _context.GetExternalInterface();
  if (externalInterface)
  {
    externalInterface->BroadcastToGame<VoiceCommandEvent>(VoiceCommandEventUnion(std::forward<T>(event)));
  }
}
  
void VoiceCommandComponent::Update()
{
  bool heardCommandUpdated = false;
  
  while (_recogProcessor->HasResults())
  {
    const auto& nextResult = _recogProcessor->PopNextResult();
    
    const auto& commandsFound = _phraseData->GetCommandsInPhrase(nextResult);
    // For now we're assuming every recognized phrase equals one command
    if (ANKI_VERIFY(commandsFound.size() == 1,
                    "VoiceCommandComponent.Update",
                    "Phrase %s produced %d commands, expected only 1.",
                    nextResult.c_str(), commandsFound.size()))
    {
      heardCommandUpdated = HandleCommand(commandsFound[0]);
    }
    
    // If we updated the pending, heard command, leave the rest of the results for later
    if (heardCommandUpdated)
    {
      LOG_INFO("VoiceCommandComponent.HeardCommand", "%s", VoiceCommandTypeToString(commandsFound[0]));
      break;
    }
  }

  // TODO this needs updating now that I'm processing different kinds of commands
//  if (ANKI_CONSOLE_SYSTEM_ENABLED)
//  {
//    static bool forceTriggerAlreadySet = false;
//    if (kShouldForceTrigger &&
//        (static_cast<uint32_t>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) % kForceTriggerFreq_s) == 0)
//    {
//      if (!forceTriggerAlreadySet)
//      {
//        LOG_INFO("VoiceCommandComponent.ForceTrigger","");
//        heardCommandUpdated = true;
//        forceTriggerAlreadySet = true;
//      }
//    }
//    else
//    {
//      forceTriggerAlreadySet = false;
//    }
//  }
  
  UpdateCommandLight(heardCommandUpdated);
  
  if (heardCommandUpdated)
  {
    BroadcastVoiceEvent(CommandHeardEvent(_pendingHeardCommand));
  }
}

bool VoiceCommandComponent::HandleCommand(const VoiceCommandType& command)
{
  // Bail out early if we still have a command we're waiting on being handled
  // This assumes that a pending, heard command that we stored intentionally (below) SHOULD be consumed and cleared
  // by an appropriate behavior immediately, and we should never end up in a situation where commands that we
  // want to react to are getting queued up.
  if (AnyCommandPending())
  {
    PRINT_NAMED_ERROR("VoiceCommandComponent.HandleCommand.OldCommandStillPending",
                      "A command has been heard(%s) while another was pending(%s). New command ignored.",
                      VoiceCommandTypeToString(command),
                      VoiceCommandTypeToString(_pendingHeardCommand));
    return false;
  }
  
  bool updatedHeardCommand = false;
  switch(_listenContext)
  {
    case VoiceCommandListenContext::Count:
    {
      // Intentionally ignore commands when there is no context to handle them
      break;
    }
    case VoiceCommandListenContext::Keyphrase:
    {
      // TODO this if check shouldn't be necessary once recognition contexts are set up and trigger keyphrase recognition
      // will be the ONLY thing that can occur in that context
      if (command == VoiceCommandType::HeyCozmo)
      {
        _pendingHeardCommand = command;
        updatedHeardCommand = true;
        _listenContext = VoiceCommandListenContext::Freeplay;
      }
      break;
    }
    case VoiceCommandListenContext::Freeplay:
    {
      // TODO this shouldn't be necessary once recognition contexts are set up and trigger keyphrase recognition
      // will be the ONLY thing that can occur in that context
      if (command != VoiceCommandType::HeyCozmo)
      {
        _pendingHeardCommand = command;
        updatedHeardCommand = true;
        _listenContext = VoiceCommandListenContext::Keyphrase;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("VoiceCommandComponent.HandleCommand.InvalidContext",
                        "Tried to handle command %s with invalid context %s",
                        VoiceCommandTypeToString(command),
                        VoiceCommandListenContextToString(_listenContext));
    }
  }
  
  return updatedHeardCommand;
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
  
  if (heardTriggerPhrase && robot && _commandLightTimeRemaining_s < 0.f)
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
    robot->GetBodyLightComponent().StartLoopingBackpackLights(kHeardCommandLights, BackpackLightSource::Voice, _bodyLightDataLocator);
  }
}

} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

