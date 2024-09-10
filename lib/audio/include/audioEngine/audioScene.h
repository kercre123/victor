//
//  audioScene.h
//  Audio Engine
//
//  Created by Jordan Rivas on 7/6/15.
//  AudioScene is a set of structs which define a group of Audio Banks and Events that are used during a specific period
//  (a scene). These structs are intended to be constructed during apps load phase and given to the AudioEngineController.
//  The app will load and unload the AudioScene as needed.
//

#ifndef __AnkiAudio_AudioScene_H__
#define __AnkiAudio_AudioScene_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include <string>
#include <vector>

namespace Json {
class Value;
}

namespace Anki {
namespace AudioEngine {

// Forward declare sub-structs
struct AudioSceneStateGroup;
struct AudioSceneEvent;
struct AudioSceneParameter;

// Define Types
using AudioEventList = std::vector<AudioSceneEvent>;
using AudioBankList = std::vector<std::string>;
using AudioZipFileList = std::vector<std::string>;
using AudioGameSyncGroupList = std::vector<AudioSceneStateGroup>;
using AudioGroupStateList = std::vector<std::string>;
using AudioParameterList = std::vector<AudioSceneParameter>;

//----------------------------------------------------------------------------------------------------------------------
// Main Scene Meta Data container. The SeneName is how the AudioEngineController identifies the Scene.
struct AUDIOENGINE_EXPORT AudioScene
{
  // Name of the Audio Scene
  std::string SceneName;
  // Events that will be prepared when the AudioScene is loaded.
  // Event meta data Bank must be already loaded or added to the bank list for this scene.
  std::vector<AudioSceneEvent> Events;
  // Banks that will be loaded when the Audio Scene is load.
  // The Banks are defined by their name "BankName"
  AudioBankList Banks;
  // Zip files will be added to project asset search paths
  AudioZipFileList ZipFiles;
  

  AudioScene() {};

  AudioScene( const std::string& sceneName,
              const AudioEventList& events,
              const AudioBankList& banks,
              const AudioZipFileList& zipFiles = {} );
  
  AudioScene( const Json::Value& sceneJson );

  // TODO: Not sure if we really need this.

  // Get a list Switch Groups used in this scene.
  // Pass in the Event Name to only get Switch Groups contained in that Event, otherwise don't pass in any arguments to
  // get all the events used in this AudioScene.
  const std::vector<std::string> GetSwitchGroups( const std::string& eventName = "" );
};


//----------------------------------------------------------------------------------------------------------------------
// AudioSceneEvent describes a single event in a AudioScene. It contains the Switch and Game Groups State for the Event.
// TODO: Consider renaming to AudioSceneAction
struct AUDIOENGINE_EXPORT AudioSceneEvent
{
  // Event Name - Must match event title in Wwise
  std::string EventName;
  // List of Switch Group States
  AudioGameSyncGroupList SwitchStates;
  // List of Game Group States
  AudioGameSyncGroupList GameStates;
  // List of Game Parameters
  AudioParameterList Parameters;

  AudioSceneEvent() {};
  
  AudioSceneEvent( const std::string& eventName,
                   const AudioGameSyncGroupList& switchStates = {},
                   const AudioGameSyncGroupList& gameStates = {},
                   const AudioParameterList& gameParameters = {} );
  
  AudioSceneEvent( const Json::Value& eventJson );

  // TODO: Not sure if we really need this.
  // Get all Switch Group States in this Event.
  const std::vector<std::string> GetSwitchStates();
};


//----------------------------------------------------------------------------------------------------------------------
// AudioSceneStateGroup describes a Switch or Game Group. This defines the group and any default cases that should be
// loaded when the AudioScene is loaded.
struct AUDIOENGINE_EXPORT AudioSceneStateGroup
{
  // State Group Name - Must match group title in Wwise
  std::string StateGroupName;
  // List of States to be loaded with the AudioScene Load - Must match state title in Wwise
  AudioGroupStateList GroupStates;

  AudioSceneStateGroup() {};
  
  AudioSceneStateGroup( const std::string& stateGroupName, const AudioGroupStateList& defaultStates );
  
  AudioSceneStateGroup( const Json::Value& groupJson );
};


//----------------------------------------------------------------------------------------------------------------------
// AudioSceneParameter describes a RTPC (Real Time Parameter Control).
struct AUDIOENGINE_EXPORT AudioSceneParameter
{
  // Parameter Name - Must match title in Wwise
  std::string ParameterName;
  // Parameter Value - Must be valid value in Wwise
  float ParameterValue = 0.0f;
  // TODO: Add GameObject, Rate of Change & Curve type
  
  AudioSceneParameter() {};
  
  AudioSceneParameter( const std::string& parameterName, float parameterValue);
  
  AudioSceneParameter( const Json::Value& parameterJson );
};

} // AudioEngine
} // Anki

#endif /* __AnkiAudio_AudioScene_H__ */
