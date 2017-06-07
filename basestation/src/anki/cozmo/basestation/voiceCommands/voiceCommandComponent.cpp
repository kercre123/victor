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

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/languagePhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
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
  Anki::Cozmo::VoiceCommand::VoiceCommandComponent* sThis = nullptr;
}

using namespace Anki::AudioUtil;
using namespace Anki::Cozmo::ExternalInterface;

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
#if REMOTE_CONSOLE_ENABLED
namespace {
  
void HearVoiceCommand(ConsoleFunctionContextRef context)
{
  VoiceCommandType command = static_cast<VoiceCommandType>(ConsoleArg_Get_Int(context, "command"));
  
  if (sThis != nullptr)
  {
    sThis->DoForceHeardPhrase(command);
  }
}
  
CONSOLE_FUNC(HearVoiceCommand, "VoiceCommand", int command);

}
#endif
  
  
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
  sThis = this;
}
  
VoiceCommandComponent::~VoiceCommandComponent()
{
  sThis = nullptr;
}

bool VoiceCommandComponent::Init()
{
  // Set up the AudioCaptureSystem
  {
    _captureSystem->Init();
    if (!ANKI_VERIFY(_captureSystem->IsValid(), "VoiceCommandComponent.Init", "CaptureSystem Init failed"))
    {
      return false;
    }
    _captureSystem->StartRecording();
  }
  
  // Set up the SpeechRecognizer
  {
    const auto* dataLoader = _context.GetDataLoader();
    if (!ANKI_VERIFY(dataLoader != nullptr, "VoiceCommandComponent.Init", "DataLoader missing"))
    {
      return false;
    }

    ANKI_VERIFY(_phraseData->Init(dataLoader->GetVoiceCommandConfig()), "VoiceCommandComponent.Init", "CommandPhraseData Init failed.");
    
    const Util::Data::DataPlatform* dataPlatform = _context.GetDataPlatform();
    if (!ANKI_VERIFY(dataPlatform != nullptr, "VoiceCommandComponent.Init", "DataPlatform missing"))
    {
      return false;
    }
    
    SetLocaleInfo();
    
    const auto& currentLanguageType = _locale->GetLanguage();
    const auto& currentCountryType = _locale->GetCountry();
    const auto& languageFilenames = _phraseData->GetLanguageFilenames(currentLanguageType, currentCountryType);
    static const std::string& voiceCommandAssetDirStr = "assets/voiceCommand/base";
    
    // We expect a valid language to be set in SetLocaleInfo() above; If we don't have valid files now then something's wrong
    if (languageFilenames._languageDataDir.empty() ||
        languageFilenames._ltsFilename.empty() ||
        languageFilenames._netfileFilename.empty())
    {
      PRINT_NAMED_ERROR("VoiceCommandComponent.Init.LanguageData",
                        "Missing language data for locale %s",
                        _locale->ToString().c_str());
      return false;
    }
    
    const std::string& languageDirStr = languageFilenames._languageDataDir;
    
    const std::string& pathBase = dataPlatform->pathToResource(Util::Data::Scope::Resources, voiceCommandAssetDirStr);
    const std::string& generalNNPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._netfileFilename} );
    const std::string& generalPronunPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._ltsFilename} );
    
    _recognizer.reset();
    SpeechRecognizerTHF* newRecognizer = new SpeechRecognizerTHF();
    newRecognizer->Init(generalPronunPath);
    
    const auto& contextDataMap = _phraseData->GetContextData();
    for (const auto& contextDataEntry : contextDataMap)
    {
      const auto typeIndex = Util::EnumToUnderlying(contextDataEntry.first);
      const auto& contextData = contextDataEntry.second;
      
      newRecognizer->AddRecognitionDataAutoGen(typeIndex, generalNNPath, _phraseData->GetPhraseDataList(currentLanguageType, contextDataEntry.first),
                                               contextData._isPhraseSpotted, contextData._allowsFollowup);
    }
    
    _recognizer.reset(newRecognizer);
    ResetContext();
    _recognizer->Start();
  }
  
  // Set up the RecognizerProcessor
  {
    if (_recogProcessor == nullptr)
    {
      PRINT_NAMED_ERROR("VoiceCommandComponent.Init", "Can't Init voice command component with an invalid audio processor.");
      return false;
    }
    
    _recogProcessor->SetSpeechRecognizer(_recognizer.get());
    _recogProcessor->SetAudioCaptureSystem(_captureSystem.get());
    _recogProcessor->Start();
  }
  
  _initialized = true;
  return true;
}

void VoiceCommandComponent::SetLocaleInfo()
{
  const auto& contextLocale = *_context.GetLocale();
  const auto& currentLanguageType = contextLocale.GetLanguage();
  const auto& currentCountryType = contextLocale.GetCountry();
  const auto& languageFilenames = _phraseData->GetLanguageFilenames(currentLanguageType, currentCountryType);
  
  // Check whether we have valid language data. If not fall back to English
  if (languageFilenames._languageDataDir.empty() ||
      languageFilenames._ltsFilename.empty() ||
      languageFilenames._netfileFilename.empty())
  {
    PRINT_NAMED_WARNING("VoiceCommandComponent.SetLocaleInfo",
                        "No language data found for locale %s. Falling back to US English.",
                        contextLocale.ToString().c_str());
    _locale.reset(new Util::Locale(Util::Locale::Language::en, Util::Locale::CountryISO2::US));
  }
  else
  {
    _locale.reset(new Util::Locale(contextLocale));
  }
}

void VoiceCommandComponent::ResetContext()
{
  if (_recognizer)
  {
    constexpr auto keyphraseIndex = Util::EnumToUnderlying(VoiceCommandListenContext::Keyphrase);
    constexpr auto freeplayIndex = Util::EnumToUnderlying(VoiceCommandListenContext::Freeplay);
    _recognizer->SetRecognizerIndex(keyphraseIndex);
    _recognizer->SetRecognizerFollowupIndex(freeplayIndex);
  }
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
        return Init();
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
    const auto& commandDataFound = _phraseData->GetDataForPhrase(_locale->GetLanguage(), nextPhrase);
    const auto& commandType = commandDataFound->GetVoiceCommandType();
    
    if (commandType == VoiceCommandType::Count)
    {
      LOG_INFO("VoiceCommandComponent.HeardPhraseNoCommand", "Heard phrase with no command: %s", nextPhrase.c_str());
      continue;
    }
    
    if (commandDataFound->HasDataValueSet(PhraseData::DataValueType::MinRecogScore))
    {
      const auto& minScore = commandDataFound->GetMinRecogScore();
      const auto& phraseScore = nextResult.second;
      // Negative scores are from forced commands, so let those pass through
      if (phraseScore > 0.0f && phraseScore < minScore)
      {
        LOG_INFO("VoiceCommandComponent.HeardCommandBelowThreshold",
                 "Heard phrase %s but score %.2f was below minimum %.2f",
                 nextPhrase.c_str(), phraseScore, minScore);
        continue;
      }
    }
    heardCommandUpdated = HandleCommand(commandType);
    
    // If we updated the pending, heard command, leave the rest of the results for later
    if (heardCommandUpdated)
    {
      LOG_INFO("VoiceCommandComponent.HeardCommand", "%s", EnumToString(commandType));
      break;
    }
  }
  
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
  const auto& listenContextDataIter = _phraseData->GetContextData().find(_listenContext);
  if (!ANKI_VERIFY(listenContextDataIter->second._commandsSet.count(command) > 0,
                  "VoiceCommandComponent.HandleCommand.InvalidCommandForContext",
                  "Tried to handle command %s with context %s",
                   EnumToString(command), EnumToString(_listenContext)))
  {
    return false;
  }
    
  switch(_listenContext)
  {
    case VoiceCommandListenContext::Count:
    {
      // Intentionally ignore commands when there is no context to handle them
      break;
    }
    case VoiceCommandListenContext::Keyphrase:
    {
      _pendingHeardCommand = command;
      updatedHeardCommand = true;
      SetListenContext(VoiceCommandListenContext::Freeplay);
      
      break;
    }
    case VoiceCommandListenContext::Freeplay:
    {
      _pendingHeardCommand = command;
      updatedHeardCommand = true;
      SetListenContext(VoiceCommandListenContext::Keyphrase);
      
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
        ResetContext();
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

void VoiceCommandComponent::DoForceHeardPhrase(VoiceCommandType commandType)
{
  // Verify we have the data structures we expect from Init()
  if (!_recognizer || !_phraseData || !_commandRecogEnabled)
  {
    return;
  }
  
  if (commandType == VoiceCommandType::Count)
  {
    return;
  }
  
  const char* phrase = _phraseData->GetFirstPhraseForCommand(_locale->GetLanguage(), commandType);
  if (nullptr == phrase)
  {
    DEV_ASSERT_MSG(false,
                   "VoiceCommandComponent.DoForceHeardPhrase.PhraseNotFound",
                   "No phrase found for voice command %s.  Be sure the type has a phrase associated with it",
                   VoiceCommandTypeToString(commandType));
    return;
  }
  
  // Check that we have a valid current context index on the recognizer
  auto rawIndex = _recognizer->GetRecognizerIndex();
  if (rawIndex == SpeechRecognizer::InvalidIndex ||
      rawIndex < 0 ||
      rawIndex >= Util::EnumToUnderlying(VoiceCommandListenContext::Count))
  {
    return;
  }
  
  // Verify we have context data for the current context
  const auto& contextDataMap = _phraseData->GetContextData();
  const auto& dataIter = contextDataMap.find(static_cast<VoiceCommandListenContext>(rawIndex));
  if (dataIter == contextDataMap.end())
  {
    return;
  }
  
  // Grab the commands for this context
  const auto& commandSet = dataIter->second._commandsSet;
  
  // We're not going to force a command that isn't in the context
  if (commandSet.find(commandType) == commandSet.end())
  {
    PRINT_NAMED_ERROR("VoiceCommandComponent.DoForceHeardPhrase.PhraseNotInContext",
                      "The phrase %s is not valid within the current context",
                      VoiceCommandTypeToString(commandType));
    return;
  }
  
  // We only support forcing commands on our special kind of recognizer
  auto* recogTHF = dynamic_cast<SpeechRecognizerTHF*>(_recognizer.get());
  if (recogTHF == nullptr)
  {
    return;
  }
    
  recogTHF->SetForceHeardPhrase(phrase);
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

