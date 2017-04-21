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

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "audioUtil/audioRecognizerProcessor.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"

#include <functional>

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

namespace {
  CONSOLE_VAR(bool, kShouldForceTrigger, "VoiceCommands", false);
  CONSOLE_VAR(u32, kForceTriggerFreq_s, "VoiceCommands", 30);
}

using namespace Anki::AudioUtil;
using namespace Anki::Cozmo::ExternalInterface;

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
VoiceCommandComponent::VoiceCommandComponent(const CozmoContext& context)
: _context(context)
, _recogProcessor(new AudioUtil::AudioRecognizerProcessor{})
, _captureSystem(new AudioUtil::AudioCaptureSystem{})
, _phraseData(new CommandPhraseData())
{
  if (!_recogProcessor->IsValid())
  {
    _recogProcessor.reset();
    PRINT_NAMED_ERROR("VoiceCommandComponent.RecogProcessorSetupFail", "Likely error setting up AudioCaptureSystem on this platform.");
  }
  
  auto* extInterface = _context.GetExternalInterface();
  if (nullptr != extInterface)
  {
    auto helper = MakeAnkiEventUtil(*extInterface, *this, GetSignalHandles());
    
    helper.SubscribeGameToEngine<MessageGameToEngineTag::VoiceCommandEvent>();
  }
}
  
VoiceCommandComponent::~VoiceCommandComponent() = default;

void VoiceCommandComponent::Init()
{
  // Set up the AudioCaptureSystem
  {
    _captureSystem->Init();
    if (!ANKI_VERIFY(_captureSystem->IsValid(), "VoiceCommandComponent.Init", "CaptureSystem Init failed"))
    {
      return;
    }
    _captureSystem->StartRecording();
  }
  
  // Set up the SpeechRecognizer
  {
    const auto* dataLoader = _context.GetDataLoader();
    if (!ANKI_VERIFY(dataLoader != nullptr, "VoiceCommandComponent.Init", "DataLoader missing"))
    {
      return;
    }

    ANKI_VERIFY(_phraseData->Init(dataLoader->GetVoiceCommandConfig()), "VoiceCommandComponent.Init", "CommandPhraseData Init failed.");
    std::vector<const char*> phraseList = _phraseData->GetPhraseListRaw();
    SpeechRecognizerTHF* newRecognizer = new SpeechRecognizerTHF();
    newRecognizer->Init();
    
    const Util::Data::DataPlatform* dataPlatform = _context.GetDataPlatform();
    if (!ANKI_VERIFY(dataPlatform != nullptr, "VoiceCommandComponent.Init", "DataPlatform missing"))
    {
      return;
    }
    
    // TODO: Generalize this for different languages COZMO-10089
    static const char* const voiceCommandAssetDirStr = "assets/voiceCommand";
    static const char* const universalDataDirStr = "base";
    static const char* const contextualDataDirStr = "contextual";
    static const char* const languageDirStr = "en_us_16kHz_v10";
    
    const std::string& pathBase = dataPlatform->pathToResource(Util::Data::Scope::Resources, voiceCommandAssetDirStr);
    const std::string& generalNNPath = Util::FileUtils::FullFilePath( {pathBase, universalDataDirStr, languageDirStr, "nn_en_us_mfcc_16k_15_250_v5.1.1.raw"} );
    
    const auto& contextDataMap = _phraseData->GetContextData();
    for (const auto& contextDataEntry : contextDataMap)
    {
      const auto typeIndex = Util::EnumToUnderlying(contextDataEntry.first);
      const auto& contextData = contextDataEntry.second;
      
      const std::string& contextTypeString = EnumToString(contextDataEntry.first);
      const std::string& nnFilename = Util::FileUtils::FullFilePath( {pathBase, contextualDataDirStr, languageDirStr, (contextTypeString + "_NN.raw")} );
      const std::string& nnFileToUse = contextData._isPhraseSpotted ? nnFilename : generalNNPath;
      const std::string& srchFilename = Util::FileUtils::FullFilePath( {pathBase, contextualDataDirStr, languageDirStr, (contextTypeString + "_SRCH.raw")} );
      
      newRecognizer->AddRecognitionDataFromFile(typeIndex, nnFileToUse, srchFilename, contextData._isPhraseSpotted, contextData._allowsFollowup);
    }
    
    constexpr auto keyphraseIndex = Util::EnumToUnderlying(VoiceCommandListenContext::Keyphrase);
    constexpr auto freeplayIndex = Util::EnumToUnderlying(VoiceCommandListenContext::Freeplay);
    newRecognizer->SetRecognizerIndex(keyphraseIndex);
    newRecognizer->SetRecognizerFollowupIndex(freeplayIndex);
    
    _recognizer.reset(newRecognizer);
    _recognizer->Start();
  }
  
  // Set up the RecognizerProcessor
  {
    if (_recogProcessor == nullptr)
    {
      PRINT_NAMED_ERROR("VoiceCommandComponent.Init", "Can't Init voice command component with an invalid audio processor.");
      return;
    }
    
    _recogProcessor->SetSpeechRecognizer(_recognizer.get());
    _recogProcessor->SetAudioCaptureSystem(_captureSystem.get());
    _recogProcessor->Start();
  }
  
  _initialized = true;
}

// NOTE: This will be called by this or another thread via the callback that is referenced within.
bool VoiceCommandComponent::RequestEnableVoiceCommand(AudioCaptureSystem::PermissionState permissionState)
{
  std::lock_guard<std::recursive_mutex> lock(_permissionCallbackLock);
  
  switch (permissionState)
  {
    case AudioCaptureSystem::PermissionState::Granted:
    {
      if (!_initialized)
      {
        LOG_INFO("VoiceCommandComponent.HandlePermissionState.Granted", "");
        Init();
      }
      return true;
    }
    case AudioCaptureSystem::PermissionState::DeniedNoRetry:
    {
      LOG_INFO("VoiceCommandComponent.HandlePermissionState.DeniedNoRetry", "");
      _permRequestAlreadyDenied = true;
      return false;
    }
    case AudioCaptureSystem::PermissionState::DeniedAllowRetry:
    {
      _permRequestAlreadyDenied = true;
      // NOTE INTENTIONAL FALL THROUGH HERE
    }
    case AudioCaptureSystem::PermissionState::Unknown:
    {
      if (!_recordPermissionBeingRequested)
      {
        _recordPermissionBeingRequested = true;
        LOG_INFO("VoiceCommandComponent.HandlePermissionState.UnknownOrDeniedAllowRetry", "Now requesting permission.");
        
        // Make a callback that lets us know when the permissions request is done
        auto callback = [this] ()
        {
          const auto& permState = _captureSystem->GetPermissionState(_permRequestAlreadyDenied);
          _commandRecogEnabled = RequestEnableVoiceCommand(permState);
          
          const auto& convertedPermState = ConvertAudioCapturePermission(permState);
          LOG_INFO("VoiceCommandComponent.HandlePermissionState.PermRequestCallbackResult", "Result State: %s", EnumToString(convertedPermState));
          
          constexpr bool useDeferred = true;
          BroadcastVoiceEvent(VoiceCommand::StateData(_commandRecogEnabled, convertedPermState), useDeferred);
          
          _recordPermissionBeingRequested = false;
        };
        _captureSystem->RequestCapturePermission(callback);
      }
      return false;
    }
  }
}

bool VoiceCommandComponent::StateRequiresCallback(AudioCaptureSystem::PermissionState permissionState) const
{
  switch (permissionState)
  {
    case AudioCaptureSystem::PermissionState::Granted: // NOTE INTENTIONAL FALL THROUGH HERE
    case AudioCaptureSystem::PermissionState::DeniedNoRetry:
    {
      return false;
    }
    case AudioCaptureSystem::PermissionState::DeniedAllowRetry: // NOTE INTENTIONAL FALL THROUGH HERE
    case AudioCaptureSystem::PermissionState::Unknown:
    {
      return true;
    }
  }
}

template<typename T>
void VoiceCommandComponent::BroadcastVoiceEvent(T&& event, bool useDeferred)
{
  auto* externalInterface = _context.GetExternalInterface();
  if (externalInterface)
  {
    if (useDeferred)
    {
      externalInterface->BroadcastDeferred(ExternalInterface::MessageEngineToGame(VoiceCommandEvent(VoiceCommandEventUnion(std::forward<T>(event)))));
    }
    else
    {
      externalInterface->BroadcastToGame<VoiceCommandEvent>(VoiceCommandEventUnion(std::forward<T>(event)));
    }
  }
}
  
void VoiceCommandComponent::Update()
{
  bool heardCommandUpdated = false;
  
  while (_recogProcessor->HasResults())
  {
    const auto& nextResult = _recogProcessor->PopNextResult();
    
    if (!_commandRecogEnabled)
    {
      continue;
    }
    
    const auto& nextPhrase = nextResult.first;
    const auto& commandsFound = _phraseData->GetCommandsInPhrase(nextPhrase);
    
    const auto& numCommands = commandsFound.size();
    if (numCommands == 0)
    {
      LOG_INFO("VoiceCommandComponent.HeardCommand", "Heard phrase with no command: %s", nextPhrase.c_str());
      continue;
    }
    
    // For now we're assuming every recognized phrase equals one command
    if (ANKI_VERIFY(numCommands == 1,
                    "VoiceCommandComponent.Update",
                    "Phrase %s produced %d commands, expected only 1.",
                    nextPhrase.c_str(), commandsFound.size()))
    {
      // TODO: Update the config phrase data with an optional score minimum to use for a phrase.
      const auto& commandFound = commandsFound[0];
      heardCommandUpdated = HandleCommand(commandFound);
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

void VoiceCommandComponent::SetListenContext(VoiceCommandListenContext listenContext)
{
  if (_listenContext == listenContext)
  {
    return;
  }
  
  _listenContext = listenContext;
  
  const auto newIndex = Util::EnumToUnderlying(_listenContext);
  _recognizer->SetRecognizerIndex(newIndex);
  
  // When we switch back to the keyphrase context, also update the follow index
  if (_listenContext == VoiceCommandListenContext::Keyphrase)
  {
    const auto freeplayIndex = Util::EnumToUnderlying(VoiceCommandListenContext::Freeplay);
    _recognizer->SetRecognizerFollowupIndex(freeplayIndex);
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
    PRINT_NAMED_WARNING("VoiceCommandComponent.HandleCommand.OldCommandStillPending",
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
        SetListenContext(VoiceCommandListenContext::Freeplay);
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
        SetListenContext(VoiceCommandListenContext::Keyphrase);
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

// NOTE: The duplication (and thus conversion) of permission state types is due to not wanting to require a CLAD definition type
// for AudioUtil (so it can stand alone and be shared), but still wanting to use CLAD types when communicating in Cozmo code.
AudioCapturePermissionState VoiceCommandComponent::ConvertAudioCapturePermission(AudioUtil::AudioCaptureSystem::PermissionState state)
{
  switch (state)
  {
    case AudioCaptureSystem::PermissionState::Unknown:          return AudioCapturePermissionState::Unknown;
    case AudioCaptureSystem::PermissionState::Granted:          return AudioCapturePermissionState::Granted;
    case AudioCaptureSystem::PermissionState::DeniedAllowRetry: return AudioCapturePermissionState::DeniedAllowRetry;
    case AudioCaptureSystem::PermissionState::DeniedNoRetry:    return AudioCapturePermissionState::DeniedNoRetry;
  }
}

template<>
void VoiceCommandComponent::HandleMessage(const VoiceCommandEvent& event)
{
  const auto& vcEventUnion = event.voiceCommandEvent;
  switch(vcEventUnion.GetTag())
  {
    case VoiceCommandEventUnionTag::requestStatusUpdate:
    {
      BroadcastVoiceEvent(VoiceCommand::StateData(_commandRecogEnabled,
                                                  ConvertAudioCapturePermission(_captureSystem->GetPermissionState(_permRequestAlreadyDenied))));
      break;
    }
    case VoiceCommandEventUnionTag::changeEnabledStatus:
    {
      if (vcEventUnion.Get_changeEnabledStatus().isVCEnabled)
      {
        // First request enabling based on current permission state
        _commandRecogEnabled = RequestEnableVoiceCommand(_captureSystem->GetPermissionState(_permRequestAlreadyDenied));
        
        auto resultingPermState = _captureSystem->GetPermissionState(_permRequestAlreadyDenied);
        
        // If no callback is going to be broadcasting the request result, we should do it now
        if (!StateRequiresCallback(resultingPermState))
        {
          BroadcastVoiceEvent(VoiceCommand::StateData(_commandRecogEnabled, ConvertAudioCapturePermission(resultingPermState)));
        }
      }
      else
      {
        _commandRecogEnabled = false;
        BroadcastVoiceEvent(VoiceCommand::StateData(_commandRecogEnabled,
                                                    ConvertAudioCapturePermission(_captureSystem->GetPermissionState(_permRequestAlreadyDenied))));
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
