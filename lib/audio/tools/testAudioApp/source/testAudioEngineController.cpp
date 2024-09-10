/**
 * File: testAudioEngineController.cpp
 *
 * Author: Jordan Rivas
 * Created: 1/16/18
 *
 * Description: Test Audio App interface to Audio Engine
 *              - Subclass of AudioEngine::AudioEngineController
 *              - Implement helper functions to load sound banks and perform audio actions
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "testAudioEngineController.h"
#include "testAudioLogger.h"
#include "audioEngine/audioScene.h"
#include "util/fileUtils/fileUtils.h"
#include <cassert>
#include <sstream>

namespace AE = Anki::AudioEngine;
// Setup Ak Logging callback
static void AudioEngineLogCallback( uint32_t, const char*, AE::ErrorLevel, AE::AudioPlayingId, AE::AudioGameObject );


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TestAudioEngineController::TestAudioEngineController(const std::string& basePath,
                                                     const std::string& writePath,
                                                     const std::string& localeTitle)
{
  // Config Engine
  // Read/Write Asset path
  _config.assetFilePath = basePath;
  _config.writeFilePath = writePath;
  
  // Locale
  if (!localeTitle.empty()) {
    _config.audioLocale = AE::AudioLocaleTypeFromString(localeTitle);
  }
  
  // Engine Memory
  _config.defaultMemoryPoolSize      = ( 4 * 1024 * 1024 );
  _config.defaultLEMemoryPoolSize    = ( 8 * 1024 * 1024 );
  _config.ioMemorySize               = ( 4 * 1024 * 1024 );
  _config.defaultMaxNumPools         = 30;
  _config.enableGameSyncPreparation  = true;
  _config.enableStreamCache          = true;
  
  // Start your Engines!!!
  InitializeAudioEngine( _config );
  
  if (IsInitialized()) {
    LOG_INFO("Initialized Wwsise");
    // Setup Engine Logging callback
    SetLogOutput( AE::ErrorLevel::All, &AudioEngineLogCallback );
    
    // Register Game Object & set default Listener
    _gameObj = 1;
    RegisterGameObject(_gameObj, "TestAudioGameObj");

    SetDefaultListeners( { _gameObj } );

  }
  else {
    LOG_ERROR("Failed to Initialize WWise SDK");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestAudioEngineController::LoadAudioScene(const std::string& sceneFilePath)
{
  std::string sceneName;
  RegisterAudioSceneWithJsonFile(sceneFilePath, sceneName);
  
  // We are leveraging the AudioScene struct however we don't need to load the events only the sound banks
  _activeScene = GetAudioScene(sceneName);
  if (_activeScene == nullptr) {
    LOG_ERROR("Can't retrieve scene '%s'", sceneName.c_str());
    return false;
  }
  
  // Add full path to Zip file names
  std::vector<std::string> zipFilePaths;
  if (!_activeScene->ZipFiles.empty()) {
    zipFilePaths.reserve(_activeScene->ZipFiles.size());
    for (auto& zipFile : _activeScene->ZipFiles) {
      zipFilePaths.push_back(Anki::Util::FileUtils::FullFilePath({ _config.assetFilePath, zipFile }));
    }
    AddZipFiles(zipFilePaths);
  }
  
  // Load scene's sound banks
  for (auto& bankName : _activeScene->Banks) {
    if (!LoadSoundbank(bankName)) {
      LOG_ERROR("Failed to Load Sound Bank '%s'", bankName.c_str());
      return false;
    }
  }
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestAudioEngineController::PostNextAction(std::function<void()> completedCallbackFunc)
{
  assert(_activeScene != nullptr);
  const auto& event = _activeScene->Events[_actionIdx++];
  
  // Perform Action Details
  // Set Game States
  if (!event.GameStates.empty()) {
    for (auto& stateGroup : event.GameStates) {
      const auto groupId = GetAudioIdFromString(stateGroup.StateGroupName);
      for (auto& state : stateGroup.GroupStates) {
        const auto stateId = GetAudioIdFromString(state);
        LOG_INFO("Set State Group '%s' State '%s'", stateGroup.StateGroupName.c_str(), state.c_str());
        SetState(groupId, stateId);
      }
    }
  }
  
  // Set Switch States
  if (!event.SwitchStates.empty()) {
    for (auto& switchGroup : event.SwitchStates) {
      const auto groupId = GetAudioIdFromString(switchGroup.StateGroupName);
      for (auto& state : switchGroup.GroupStates) {
        const auto stateId = GetAudioIdFromString(state);
        LOG_INFO("Set Switch State Group '%s' State '%s'", switchGroup.StateGroupName.c_str(), state.c_str());
        SetSwitchState(groupId, stateId, _gameObj);
      }
    }
  }
  
  // Set Parameter Values
  if (!event.Parameters.empty()) {
    for (auto& parameter : event.Parameters) {
      const auto parameterId = GetAudioIdFromString(parameter.ParameterName);
      LOG_INFO("Set Parameter '%s' Value '%f'", parameter.ParameterName.c_str(), parameter.ParameterValue);
      // Set parameter globally
      SetParameter(parameterId, parameter.ParameterValue, AE::kInvalidAudioGameObject);
    }
  }
  
  // Post Event
  if (!event.EventName.empty()) {
    const auto& eventName = event.EventName;
    AE::AudioCallbackContext* callbackContext = new AE::AudioCallbackContext();
    // Set callback flags
    callbackContext->SetCallbackFlags(AE::AudioCallbackFlag::Complete);
    // Execute callbacks synchronously (on main thread)
    callbackContext->SetExecuteAsync( false );
    // Register callbacks for event
    callbackContext->SetEventCallbackFunc([completedCallbackFunc, &eventName]
                                          (const AE::AudioCallbackContext* thisContext,
                                           const AE::AudioCallbackInfo& callbackInfo)
                                          {
                                            LOG_INFO("Event Completed: '%s'", eventName.c_str());
                                            completedCallbackFunc();
                                          });
    LOG_INFO("Post Event: '%s'", eventName.c_str());
    PostAudioEvent(eventName, _gameObj, callbackContext);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestAudioEngineController::AllActionsCompleted() const
{
  if (_activeScene == nullptr) {
    return true;
  }
  return (_actionIdx >= _activeScene->Events.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Setup Ak Logging callback
void AudioEngineLogCallback( uint32_t akErrorCode,
                            const char* errorMessage,
                            AE::ErrorLevel errorLevel,
                            AE::AudioPlayingId playingId,
                            AE::AudioGameObject gameObjectId )
{
  std::ostringstream logStream;
  logStream << "ErrorCode: " << akErrorCode << " Message: '" << ((nullptr != errorMessage) ? errorMessage : "")
  << "' LevelBitFlag: " << (uint32_t)errorLevel << " PlayingId: " << playingId << " GameObjId: " << gameObjectId;
  
  if (((uint32_t)errorLevel & (uint32_t)AE::ErrorLevel::Message) == (uint32_t)AE::ErrorLevel::Message) {
    LOG_INFO("WwiseLog: '%s'", logStream.str().c_str());
  }
  
  if (((uint32_t)errorLevel & (uint32_t)AE::ErrorLevel::Error) == (uint32_t)AE::ErrorLevel::Error) {
    LOG_ERROR("WwiseLog: '%s'", logStream.str().c_str());
  }
}
