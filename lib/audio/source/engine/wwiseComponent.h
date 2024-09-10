/***********************************************************************************************************************
 *
 *  WwiseComponent
 *  Engine
 *
 *  Author: Jordan Rivas
 *  Created: 02/06/17
 *
 *  Description:
 *  - Interface that encapsulates the underlying Wwise sound engine
 *
 *  Copyright: Anki, Inc. 2017
 *
 ***********************************************************************************************************************/

#ifndef __AnkiAudio_WwiseComponent_H__
#define __AnkiAudio_WwiseComponent_H__

#include "audioEngine/audioTypes.h"
#include "audioEngine/audioEngineConfigDefines.h"
#include "audioEngine/audioCallback.h"
#include "audioEngine/audioScene.h"
#include "util/helpers/noncopyable.h"
#include "util/math/numericCast.h"
#include <map>
#include <memory>
#include <set>
#include <string>


// Wwise forward declarations
struct AkStreamMgrSettings;
struct AkMemSettings;
struct AkDeviceSettings;
struct AkInitSettings;
struct AkPlatformInitSettings;
struct AkMusicSettings;
struct AkCommSettings;
struct AkCallbackInfo;

class AudioFilePackageLowLevelIOBlocking;


namespace Anki {
namespace AudioEngine {
  
class WwiseComponent : private Anki::Util::noncopyable {
  
public:
  
  WwiseComponent();
  
  ~WwiseComponent();
  
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Setup/Teardown functions ... must be called before any other functions.
  // Note:  + Most of the time these functions are ok to call back to back at program start
  //        + If either function fails, you should tear down the audio engine if you want to quit safely
  
  // Call Initialize at startup.
  // Returns true if successful, false otherwise.
  bool Initialize( const SetupConfig& config );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Add Zip file paths to Wwise file search paths
  // Note: this will search and remove any zip file paths that no longer exist
  void AddZipFiles( const std::vector<std::string>& pathsToZipFiles );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Set Audio Log Ouput
  bool SetLogOutput( ErrorLevel errorLevel, LogCallbackFunc logCallback );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Set Session Profiler Capture
  // Write profiler session log to .prof file
  bool WriteProfilerCapture( bool write, const std::string& fileName );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Set Session Output Capture
  // Write audio output to .wav file
  bool WriteAudioOutputCapture( bool write, const std::string& fileName );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Needs to be called once per frame in order to update the underlying audio engine
  void Update( float deltaTime );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Perform all queued callbacks
  void FlushCallbackQueue();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Load Banks with name "BankName.bnk"
  bool LoadBank( const std::string bankName );
  
  // Unload Banks with name "BankName.bnk"
  bool UnloadBank( const std::string bankName );
  
  // TODO: Need to update and get back into project
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Prepare List of Events
  // Event Bank must be loaded before calling this
  bool PrepareEvents( const std::vector<std::string>& eventNameList, bool load = true );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Prepare Game Syncs
  // Event Bank must be loaded before calling this
  // Load media linked to prepared events through game sync group and state
  bool PrepareGameSyncs( const AudioGroupType groupType, const AudioSceneStateGroup stateGroup, bool load = true );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Every posted event requires a game object to be associated with it.
  // Game objects need to be registered before they can be used.
  // + optionalName is only used for debugging purposes
  // Retrun true if successful
  bool RegisterAudioGameObject( const AudioGameObject gameObjectId, const std::string& optionalName = "" );
  bool UnregisterAudioGameObject( const AudioGameObject gameObjectId );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Need listener to hear content played by Game Object
  // Must set a default listner
  void SetDefaultListeners( const std::vector<AudioGameObject>& listeners);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Posts the specified event to the audio engine (usually plays a specific audio event)
  // Audio engine accepts events as either a string, or an id
  // + This saves the user from having to pre-convert strings to id's as the underlying audio engine will do that for us
  // + Strings are case insensitive
  AudioPlayingId PostEvent( AudioEventId eventId,
                            AudioGameObject gameObject,
                            AudioCallbackContext* callbackContext = nullptr );
  
  // Stops playing all sounds on the specified game object.
  // + If AudioGameObject::Invalid is specified, then ALL audio will be stopped
  void StopAllAudioEvents( AudioGameObject gameObject = kInvalidAudioGameObject );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Set RTPC Value
  bool SetRTPCValue( AudioParameterId parameterId,
                     AudioRTPCValue rtpcValue,
                     AudioGameObject gameObject,
                     AudioTimeMs valueChangeDuration = 0,
                     AudioCurveType curve = AudioCurveType::Linear );
  
  bool SetRTPCValueWithPlayingId( AudioParameterId parameterId,
                                  AudioRTPCValue rtpcValue,
                                  AudioPlayingId playingId,
                                  AudioTimeMs valueChangeDuration = 0,
                                  AudioCurveType curve = AudioCurveType::Linear );
  
  
  // Get Audio Engine's current RTPC Value
  // Note: inOut_rtpcValueType will be set to the RTPC current source
  bool GetRTPCValue( AudioParameterId parameterId,
                     AudioGameObject gameObject,
                     AudioPlayingId playingId,
                     AudioRTPCValue& out_rtpcValue,
                     AudioRTPCValueType& inOut_rtpcValueType );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool SetSwitch( AudioSwitchGroupId switchGroupId,
                  AudioSwitchStateId switchStateId,
                  AudioGameObject gameObject );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Set State
  bool SetState( AudioStateGroupId stateGroupId, AudioStateId stateId );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Game-Defined Auxiliary Sends
  // Wwise only allows upto 4 aux buses per GameObject
  // Note: To clear a GameObject's Aux buses sends pass in an empty vector
  bool SetGameObjectAuxSendValues( AudioGameObject gameObject,
                                   const std::vector<AudioAuxBusValue>& auxSendValues );
  
  // Set the output bus volume (direct) to be used for the specified GameObject.
  // The control value is a number ranging from 0.0f to 1.0f.
  bool SetGameObjectOutputBusVolume( AudioGameObject emitterGameObj,
                                     AudioGameObject listenerGameObj,
                                     AudioReal32 controlVolume );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Convert strings into audio ids
  
  // All of the audio engine names are converted to an unsigned int via a hash
  static uint32_t GetAudioIdFromString( const std::string& name );
  
  
  // Get Default GameObject Id for Global objects: Mixer, Bus, FX, etc.
  AudioGameObject GetDefaultGameObjectId();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // System methods
  void SetRandomSeed( uint16_t seed );
  
  // Check if file exist in all file locations
  bool CheckFileExists( const std::string& fileName );
  
  // Register/Unregister for Audio Engine Global callbacks
  // Note: Callbacks are called on the Audio Engine's thread, any work done will block that thread
  // Note: Callback order is not guaranteed
  // Return kInvalidAudioEngineCallbackId if registration failed
  AudioEngineCallbackId RegisterGlobalCallback( AudioEngineCallbackFlag callbackFlag,
                                                AudioEngineCallbackFunc&& callbackFunc );
  bool UnregisterGlobalCallback( AudioEngineCallbackId callbackId );
  
  // This is for internal use only, do not call!
  // Perform work to call appropriate registered callbacks
  static void PerformEngineGlobalCallback( AudioEngineCallbackFlag callbackFlag, WwiseComponent* component );
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private functions
private:
  // Fill out our settings structs with all of our default settings.
  void GetDefaultSettings( const SetupConfig&       config,
                           AkStreamMgrSettings&     streamManagerSettings,
                           AkMemSettings&           memoryManagerSettings,
                           AkDeviceSettings&        deviceSettings,
                           AkInitSettings&          initSettings,
                           AkPlatformInitSettings&  platformSettings,
                           AkMusicSettings&         musicSettings ) const;
  
  // Initializes all of our required sub-modules
  bool InitSubModules( const SetupConfig&       config,
                       AkStreamMgrSettings&     streamManagerSettings,
                       AkMemSettings&           memoryManagerSettings,
                       AkDeviceSettings&        deviceSettings,
                       AkInitSettings&          initSettings,
                       AkPlatformInitSettings&  platformSettings,
                       AkMusicSettings&         musicSettings );
  
  // Initialize file location resolver and set paths
  bool InitFileResolver( const SetupConfig& config,
                         AkDeviceSettings&  deviceSettings );

  // Initializes our required sound banks
  bool InitSoundBanks( const SetupConfig& config );
  
  // Terminates and un-initializes the underlying audio engine.
  void Terminate();
  
  // Returns English US if local type is not found
  const std::string GetLanguageLocaleString( AudioLocaleType locale );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Private members
private:
  
  bool _isInitialized = false;
  
  // File Resolver
  AudioFilePackageLowLevelIOBlocking* _fileLocationResolver = nullptr;
  
  // Audio Engine Global callback variables
  // Store Audio Engine Callback info
  struct AudioEngineCallbackContext {
    AudioEngineCallbackId CallbackId     = kInvalidAudioEngineCallbackId;
    AudioEngineCallbackFlag CallbackFlag = AudioEngineCallbackFlag::NoAudioEngineCallbacks;
    AudioEngineCallbackFunc CallbackFunc = nullptr;

    AudioEngineCallbackContext( AudioEngineCallbackId callbackId,
                                AudioEngineCallbackFlag callbackFlag,
                                AudioEngineCallbackFunc&& callbackFunc )
    : CallbackId( callbackId )
    , CallbackFlag( callbackFlag )
    , CallbackFunc( std::move(callbackFunc) ) {}
  };

  std::mutex _audioEngineCallbackMutex;
  AudioEngineCallbackId _lastEngineCallbackId = kInvalidAudioEngineCallbackId;
  using AudioEngineCallbackContextPtr = std::shared_ptr<AudioEngineCallbackContext>;
  using AudioEngineCallbackContextSet = std::set<AudioEngineCallbackContextPtr>;
  std::map<AudioEngineCallbackFlag, AudioEngineCallbackContextSet> _engineCallbackTypeMap;
  std::map<AudioEngineCallbackId, AudioEngineCallbackContextPtr> _engineCallbackIdMap;

}; // class WwiseComponent

} // AudioEngine
} // Anki

#endif /* __AnkiAudio_WwiseComponent_H__ */
