/***********************************************************************************************************************
 *
 *  AudioEngineController
 *  Audio
 *
 *  Created by Jordan Rivas on 02/10/17.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Interface that encapsulates the underlying sound engine
 *  - Hides all engine specific details behind this interface
 *
 **********************************************************************************************************************/

#ifndef __AnkiAudio_AudioEngineController_H__
#define __AnkiAudio_AudioEngineController_H__


#include "audioEngine/audioExport.h"
#include "audioEngine/audioTypes.h"
#include "audioEngine/audioEngineConfigDefines.h"
#include "audioEngine/audioCallback.h"
#include "util/helpers/noncopyable.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>


namespace Anki {
namespace AudioEngine {

class MusicConductor;
class WwiseComponent;
struct MusicConductorConfig;
struct AudioScene;
namespace PlugIns {
  class AnkiPluginInterface;
}

class AUDIOENGINE_EXPORT AudioEngineController : private Anki::Util::noncopyable
{
public:

  static const char* kLogChannelName;

  using AudioNameList = std::vector< std::string >;


  AudioEngineController();
  virtual ~AudioEngineController();

  bool IsInitialized() const { return _isInitialized; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Setup/Teardown functions ... must be called before any other functions.
  // Note:  + Most of the time these functions are ok to call back to back at program start
  //        + If either function fails, you should tear down the audio engine if you want to quit safely


  // Call Initialize at startup.
  // Returns true if successful, false otherwise.
  bool InitializeAudioEngine( const SetupConfig& config );

  // App can optionally Initialize Music Conductor and Plugin Interface components
  void InitializeMusicConductor( const MusicConductorConfig& config );

  void InitializePluginInterface();

  // Set Audio Log Output
  bool SetLogOutput( ErrorLevel errorLevel, LogCallbackFunc logCallback );

  // Save session profiler capture to a file
  bool WriteProfilerCapture( bool write, const std::string& fileName = "" );

  // Save session audio output to a file
  bool WriteAudioOutputCapture( bool write, const std::string& fileName = ""  );

  // Add zip files to file locations
  void AddZipFiles( const std::vector<std::string>& pathsToZipFiles );

  // Check if a file exist in all file locations
  bool CheckFileExists( const std::string& fileName );

  // Register/Unregister for Audio Engine Global callbacks
  // Note: Callbacks are called on the Audio Engine's thread, any work done will block that thread
  // Note: Callback order is not guaranteed
  // Return kInvalidAudioEngineCallbackId if registration failed
  AudioEngineCallbackId RegisterGlobalCallback( AudioEngineCallbackFlag callbackFlag,
                                                AudioEngineCallbackFunc&& callbackFunc );
  bool UnregisterGlobalCallback( AudioEngineCallbackId callbackId );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //  Basic Audio engine functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Note: Transfers ownership of callback context to Audio Controller
  // TODO: Deprecate - Users can use GetAudioIdFromString() to get EventId
  virtual AudioPlayingId PostAudioEvent( const std::string& eventName,
                                         AudioGameObject gameObjectId = kInvalidAudioGameObject,
                                         AudioCallbackContext* callbackContext = nullptr );

  virtual AudioPlayingId PostAudioEvent( AudioEventId eventId,
                                         AudioGameObject gameObjectId = kInvalidAudioGameObject,
                                         AudioCallbackContext* callbackContext = nullptr );

  // Stops playing all sounds on the specified game object
  // + If AudioGameObject::Invalid is specified, then ALL audio will be stopped
  virtual void StopAllAudioEvents( AudioGameObject gameObjectId = kInvalidAudioGameObject );

  virtual bool SetState( AudioStateGroupId stateGroupId,
                         AudioStateId stateId ) const;

  virtual bool SetSwitchState( AudioSwitchGroupId switchGroupId,
                               AudioSwitchStateId switchStateId,
                               AudioGameObject gameObjectId ) const;

  virtual bool SetParameter( AudioParameterId parameterId,
                             AudioRTPCValue rtpcValue,
                             AudioGameObject gameObjectId,
                             AudioTimeMs valueChangeDuration = 0,
                             AudioCurveType curve = AudioCurveType::Linear ) const;

  virtual bool SetParameterWithPlayingId( AudioParameterId parameterId,
                                          AudioRTPCValue rtpcValue,
                                          AudioPlayingId playingId,
                                          AudioTimeMs valueChangeDuration = 0,
                                          AudioCurveType curve = AudioCurveType::Linear ) const;

  // Note: inOut_rtpcValueType will be set to the RTPC current source
  bool GetParameterValue( AudioParameterId parameterId,
                          AudioGameObject gameObject,
                          AudioPlayingId playingId,
                          AudioRTPCValue& out_rtpcValue,
                          AudioRTPCValueType& inOut_rtpcValueType );

  // Register Game Objects
  // Note: Game Object Ids must be unique
  bool RegisterGameObject( AudioGameObject gameObjectId, const std::string& optionalName = "" );

  bool UnregisterGameObject( AudioGameObject gameObjectId );

  // Set Game Object Listeners
  void SetDefaultListeners( const std::vector<AudioGameObject>& listeners);


  // Game-Defined Auxiliary Sends
  using AuxSendList = std::vector<AudioAuxBusValue>;
  bool SetGameObjectAuxSendValues( AudioGameObject gameObjectId, const AuxSendList& auxSendValues );

  bool SetGameObjectOutputBusVolume( AudioGameObject emitterGameObj,
                                     AudioGameObject listenerGameObj,
                                     AudioReal32 controlVolume );
  // Tick Audio Engine
  // This will Tick the Music Conductor, ProcessAudioQueue() & FlushAudioQueue()
  // Note: This is thread safe
  void Update();

  // Process all Audio Engine events
  // This flushes all audio events that have been posted
  void ProcessAudioQueue() const;

  // Process all Callbacks
  // Note: Call this on the same thread as Update() to assure the callback objects are not deleted while callback is
  // being performed.
  void FlushCallbackQueue();

  // Get hash value for string
  // Return 0 if invalid
  static uint32_t GetAudioIdFromString( const std::string& name );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Access components
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Music
  // Can be nullptr if not initialized
  MusicConductor* const GetMusicConductor() const { return _musicConductor.get(); }


  // Plugin Access
  // Can be nullptr if not initialized
  PlugIns::AnkiPluginInterface* const GetPluginInterface() const { return _pluginInterface.get(); }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Manage audio scenes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Add/Remove AudioScenes - Store Game Scenes
  // Regester Audio Scene with a json file
  // Return true for success
  bool RegisterAudioSceneWithJsonFile( const std::string& resourcePath, std::string& out_sceneName );
  // Transfer Ownership Exp. RegisterAudioScene( std::move( theScene ) )
  void RegisterAudioScene( const AudioScene&& scene );
  void UnregisterAudioScene( const std::string& sceneName );
  // Load/Unload AudioScene - Use Game Scenes
  bool LoadAudioScene( const std::string& sceneName );
  bool UnloadAudioScene( const std::string& sceneName );
  // ActiveScenes - Query current loaded scenes
  const AudioNameList& GetActiveScenes() const { return _activeSceneNames; }
  // Get Access to Scene
  // Return nullptr if SceneName is not registered
  const AudioScene* GetAudioScene(const std::string& sceneName ) const;

  // TODO: Change Scenes - Add/Remove Scenes with diff

  // Load/Unload Soundbank
  bool LoadSoundbank( const std::string& bankName );
  bool UnloadSoundbank( const std::string& bankName );

  const AudioNameList& GetLoadedBankNames() const { return _loadedBankNames; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private functions
private:

  // Clean up callback messages
  void MoveCallbackContextToGarbageCollector( const AudioCallbackContext* callbackContext );
  void ClearGarbageCollector();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private members
private:

  bool _isInitialized = false;

  // Order matters! Members will be destroyed in reverse order of declaration.
  // Audio engine may make callbacks into our plugins, so the audio engine
  // component must be shut down before plugins are destroyed.
  std::unique_ptr< PlugIns::AnkiPluginInterface > _pluginInterface;
  std::unique_ptr< WwiseComponent > _engineComponent;
  std::unique_ptr< MusicConductor > _musicConductor;

  // Track Registered Game Objects
  std::unordered_map< AudioGameObject, std::string > _registeredGameObjects;

  using CallbackContextMap = std::unordered_map< AudioPlayingId, AudioCallbackContext* >;
  CallbackContextMap _callbackContextMap;

  // Guard access to callbackContextMap
  std::mutex _callbackContextMutex;

  std::vector< AudioCallbackContext* > _callbackGarbageCollector;

  // Loaded Audio Scenes
  std::unordered_map< std::string, AudioScene > _audioScenes;

  AudioNameList _loadedBankNames;
  AudioNameList _activeSceneNames;

};

} // AudioEngine
} // Anki


#endif // __AnkiAudio_AudioEngineController_H__
