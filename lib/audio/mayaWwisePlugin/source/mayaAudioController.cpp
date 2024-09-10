//
//  MayaAudioController.cpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 1/09/18.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//


#include "mayaAudioController.h"
#include "mayaIncludes.h"
#include <sstream>
#include <string>


namespace AE = Anki::AudioEngine;
// Setup Ak Logging callback
static void AudioEngineLogCallback( uint32_t, const char*, AE::ErrorLevel, AE::AudioPlayingId, AE::AudioGameObject );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MayaAudioController::MayaAudioController(char* soundbanksPath)
: _soundbankLoader( new AE::SoundbankLoader( *this, std::string(soundbanksPath) ) )
{
  // Config Engine
  AE::SetupConfig config{};
  // Read/Write Asset path
  config.assetFilePath = std::string(soundbanksPath);
  
  // Cozmo uses default audio locale regardless of current context.
  // Locale-specific adjustments are made by setting GameState::External_Language
  // below.
  config.audioLocale = AE::AudioLocaleType::EnglishUS;

  // Engine Memory
  // NOTE: This is huge! Okay for desktop development not shipping Apps
  config.defaultMemoryPoolSize      = ( 16 * 1024 * 1024 );   // 16 MB
  config.defaultLEMemoryPoolSize    = ( 16 * 1024 * 1024 );   // 16 MB
  config.ioMemorySize               = (  8 * 1024 * 1024 );   //  8 MB
  config.defaultMaxNumPools         = 30;
  config.enableGameSyncPreparation  = true;
  config.enableStreamCache          = true;


  // Start your Engines!!!
  InitializeAudioEngine( config );

  if (IsInitialized()) {
    // Setup Engine Logging callback
    SetLogOutput( AE::ErrorLevel::All, &AudioEngineLogCallback );

    // Load soundbanks
    _soundbankLoader->LoadDefaultSoundbanks();

    // Register Game Object
    _gameObj = 1;
    bool success = RegisterGameObject(_gameObj, "MayaGameObj");
    if (!success) {
      MGlobal::displayError("Failed to Register Maya Game Object");
    }
    else {
      MGlobal::displayInfo("Successfully Registered Maya Game Object!");
      SetDefaultListeners( { _gameObj } );
    }
  }
  else {
    MGlobal::displayError("Failed to Initialize WWise SDK");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MayaAudioController::PostAudioKeyframe( const AudioKeyframe* keyframe )
{
  bool success = false;
  if ( IsInitialized() && (keyframe != nullptr) ) {
    // Set States
    success = true;
    bool actionSuccess;
    for ( const auto& aState : keyframe->states ) {
      actionSuccess = SetState( GetAudioIdFromString(aState.stateGroup),
                                GetAudioIdFromString(aState.state) );
      if ( !actionSuccess ) {
        MGlobal::displayError("MayaAudioController.PostAudioKeyframe - Set State '" +
                              MString(aState.Description().c_str()) + "'");
        success &= false;
      }
    }
    // Set Switches
    for ( const auto& aSwitch : keyframe->switches ) {
      actionSuccess = SetSwitchState( GetAudioIdFromString(aSwitch.switchGroup),
                                      GetAudioIdFromString(aSwitch.state),
                                      _gameObj);
      if ( !actionSuccess ) {
        MGlobal::displayError("MayaAudioController.PostAudioKeyframe - Set Switch '" +
                              MString(aSwitch.Description().c_str()) + "'");
        success &= false;
      }
    }
    // Set Parameters
    for ( const auto& aParameter : keyframe->parameters ) {
      actionSuccess = SetParameter( GetAudioIdFromString(aParameter.name),
                                    aParameter.value,
                                    _gameObj,
                                    aParameter.duration_ms,
                                    static_cast<Anki::AudioEngine::AudioCurveType>(aParameter.curveType) );
      if ( !actionSuccess ) {
        MGlobal::displayError("MayaAudioController.PostAudioKeyframe - Set Parameter /'" +
                              MString(aParameter.Description().c_str()) + "'");
        success &= false;
      }
    }
    
    // Post Events, last!
    const auto* probEvent = keyframe->GetAudioEventToPlay();
    if ( probEvent != nullptr ) {
      // Post Event
      AE::AudioPlayingId playingId = PostAudioEvent( probEvent->name, _gameObj );
      
      if ( playingId != AE::kInvalidAudioPlayingId ) {
        // Set Volume RTPC on play event, this is project specific parameter.
        static const AE::AudioParameterId parameterId = GetAudioIdFromString("Event_Volume");
        actionSuccess = SetParameterWithPlayingId( parameterId, probEvent->volume, playingId );
      }
      else {
        actionSuccess = false;
      }
      if ( !actionSuccess ) {
        MGlobal::displayError("MayaAudioController.PostAudioKeyframe - Post Event /'" +
                              MString(probEvent->Description().c_str()) + "'");
        success &= false;
      }
    }
    
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MayaAudioController::PlayAudioEvent(const std::string& eventName)
{
  AE::AudioPlayingId playingId = PostAudioEvent(eventName, _gameObj);
  if ( playingId == AE::kInvalidAudioPlayingId ) {
    MGlobal::displayError("MayaAudioController.PlayAudioEvent - Failed to post audio event: " + MString(eventName.c_str()));
    return false;
  } else {
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MayaAudioController::SetParameterValue(const std::string& paramName, const float paramValue)
{
  bool success = SetParameter(GetAudioIdFromString(paramName), paramValue, _gameObj);
  if ( !success ) {
    MGlobal::displayError("MayaAudioController.SetParameterValue - Failed to set value for audio parameter: " + MString(paramName.c_str()));
    return false;
  } else {
    return true;
  }
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
  
  if ( ((uint32_t)errorLevel & (uint32_t)AE::ErrorLevel::Message) == (uint32_t)AE::ErrorLevel::Message ) {
    MString debugOut("AudioEngineLog ");
    debugOut += logStream.str().c_str();
    MGlobal::displayInfo(debugOut);
  }
  
  if ( ((uint32_t)errorLevel & (uint32_t)AE::ErrorLevel::Error) == (uint32_t)AE::ErrorLevel::Error ) {
    MString debugOut("AudioEngineError ");
    debugOut += logStream.str().c_str();
    MGlobal::displayError(debugOut);
  }
}
