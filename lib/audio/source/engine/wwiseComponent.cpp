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




#include "audioEngine/audioDefines.h"
#include "engine/audioFilePackageLowLevelIOBlocking.h"
#include "engine/wwiseComponent.h"
#include "util/helpers/templateHelpers.h"
// Wise Libs
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>


// Plug-ins
#include "audioEnginePluginIncludes.h"

// Communication between Wwise and the game (excluded in release build)
#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include <assert.h>
#include <vector>
#include <algorithm>


//----------------------------------------------------------------------------------------------------------------------
// Alloc/Free hooks required to be define by the game
namespace AK
{
  // Simply use malloc for our allocation
  void * AllocHook( size_t in_size )
  {
    return malloc( in_size );
  }
  
  // Simply use free to deallocate
  void FreeHook( void * in_ptr )
  {
    free( in_ptr );
  }
}


namespace Anki {
namespace AudioEngine {
  
static std::mutex sCallbackQueueMutex;
static std::vector<std::pair<AudioCallbackContext*, std::unique_ptr<const AudioCallbackInfo>>> sCallbackQueue;
  
// Convert between Anki's AudioRTPCValueType & Wwise's RTPCValue_type
AudioRTPCValueType ToRTCPValueType( AK::SoundEngine::Query::RTPCValue_type type );
AK::SoundEngine::Query::RTPCValue_type ToRTCPValueType( AudioRTPCValueType type );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WwiseComponent::WwiseComponent() :
_fileLocationResolver( new AudioFilePackageLowLevelIOBlocking() )
{
  // Setup engine global callback type map
  static const std::vector<AudioEngineCallbackFlag> kEngineFlagList
  { BegingFrameRender, EndFrameRender, EndAudioProcessing };
  for ( const auto& engType : kEngineFlagList ) {
    _engineCallbackTypeMap.emplace( engType, AudioEngineCallbackContextSet() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WwiseComponent::~WwiseComponent()
{
  Terminate();
  Anki::Util::SafeDelete( _fileLocationResolver );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::Terminate()
{
#ifndef AK_OPTIMIZED

  AK::Comm::Term();

#endif // AK_OPTIMIZED
  
  AK::MusicEngine::Term();
  
  AK::SoundEngine::Term();
  
  _fileLocationResolver->Term();
  
  if ( AK::IAkStreamMgr::Get() )
  {
    AK::IAkStreamMgr::Get()->Destroy();
  }
  
  AK::MemoryMgr::Term();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::Initialize( const SetupConfig& config )
{
  AkStreamMgrSettings     streamManagerSettings;
  AkMemSettings           memoryManagerSettings;
  AkDeviceSettings        deviceSettings;
  AkInitSettings          initSettings;
  AkPlatformInitSettings  platformSettings;
  AkMusicSettings         musicSettings;
  
  // Make sure we call AudioEngineController::Terminate() between each initialize (eg. Unity plugin manually tears down the audio engine)
  assert( !AK::SoundEngine::IsInitialized() );
  
  bool initialized = false;
  
  // Fill out our initialization settings
  GetDefaultSettings( config, streamManagerSettings, memoryManagerSettings, deviceSettings, initSettings, platformSettings, musicSettings );
  
  // Initialize all of the various modules and load our sound banks.
  initialized = InitSubModules( config, streamManagerSettings, memoryManagerSettings, deviceSettings, initSettings, platformSettings, musicSettings );
  if ( initialized )
  {
    initialized = InitSoundBanks( config );
  }
  else
  {
    Terminate();
  }
  
  // Confirm Initial phase and Engine successfully initialized
  _isInitialized = ( initialized && AK::SoundEngine::IsInitialized() );
  
  if ( config.randomSeed != SetupConfig::kNoRandomSeed ) {
    SetRandomSeed( config.randomSeed );
  }

  return _isInitialized;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::AddZipFiles( const std::vector<std::string>& pathsToZipFiles )
{
  _fileLocationResolver->RemoveMissingZipFiles();
  for (auto const& path : pathsToZipFiles)
  {
    AKRESULT result =
    _fileLocationResolver->AddZipFile(path,
                                      AudioDefaultIOHookBlocking::SearchPathLocation::Head);
    if ( result != AK_Success && result != AK_PathNodeAlreadyInList && !path.empty())
    {
      AUDIO_LOG( "WwiseComponent.AddZipFiles: Failed to add zip file (%s) [error %d]",
                path.c_str(), result);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::GetDefaultSettings( const SetupConfig&       config,
                                         AkStreamMgrSettings&     streamManagerSettings,
                                         AkMemSettings&           memoryManagerSettings,
                                         AkDeviceSettings&        deviceSettings,
                                         AkInitSettings&          initSettings,
                                         AkPlatformInitSettings&  platformSettings,
                                         AkMusicSettings&         musicSettings ) const
{
  // Memory Manager settings
  {
    memoryManagerSettings.uMaxNumPools = config.defaultMaxNumPools;
  }
  
  // Stream Manager settings
  {
    AK::StreamMgr::GetDefaultSettings( streamManagerSettings );
    streamManagerSettings.uMemorySize = config.streamingMangMemorySize;
  }
  
  // Device settings
  {
    AK::StreamMgr::GetDefaultDeviceSettings( deviceSettings );
    deviceSettings.bUseStreamCache  = config.enableStreamCache;
    deviceSettings.uIOMemorySize    = config.ioMemorySize;
    deviceSettings.uGranularity     = config.ioMemoryGranularitySize;
    
    // Thread Scheduler
    if ( config.threadScheduler.priority != SetupConfig::ThreadProperties::kInvalidPriority ) {
      deviceSettings.threadProperties.nPriority = config.threadScheduler.priority;
    }
    if ( config.threadScheduler.affinityMask != SetupConfig::ThreadProperties::kInvalidAffinityMask ) {
      deviceSettings.threadProperties.dwAffinityMask = config.threadScheduler.affinityMask;
    }
  }
  
  // Sound Engine initialization settings
  {
    AK::SoundEngine::GetDefaultInitSettings( initSettings );
    initSettings.uContinuousPlaybackLookAhead = config.defaultPlaybackLookAhead;
    initSettings.uDefaultPoolSize             = config.defaultMemoryPoolSize;
    initSettings.bEnableGameSyncPreparation   = config.enableGameSyncPreparation;
    // Check if there is Main Output Shareset
    if (!config.mainOutputSharesetName.empty()) {
      const auto sharesetId = AK::SoundEngine::GetIDFromString( config.mainOutputSharesetName.c_str() );
      initSettings.settingsMainOutput.audioDeviceShareset = sharesetId;
    }

    if ( config.bufferSize != SetupConfig::kInvalidBufferSize ) {
      initSettings.uNumSamplesPerFrame = config.bufferSize;
    }
  }
  
  // Platform specific settings
  {
    AK::SoundEngine::GetDefaultPlatformInitSettings( platformSettings );
    platformSettings.uLEngineDefaultPoolSize = config.defaultLEMemoryPoolSize;
    
    if ( config.sampleRate != SetupConfig::kInvalidSampleRate ) {
      platformSettings.uSampleRate = config.sampleRate;
    }
    
    if ( config.bufferRefillsInVoice != SetupConfig::kInvalidBufferRefillsInVoice ) {
      platformSettings.uNumRefillsInVoice = config.bufferRefillsInVoice;
    }
    
    // Thread Bank Manager
    {
      if ( config.threadBankManager.priority != SetupConfig::ThreadProperties::kInvalidPriority ) {
        platformSettings.threadBankManager.nPriority = config.threadBankManager.priority;
      }
      if ( config.threadBankManager.affinityMask != SetupConfig::ThreadProperties::kInvalidAffinityMask ) {
        platformSettings.threadBankManager.dwAffinityMask = config.threadBankManager.affinityMask;
      }
    }

    // Thread Low Engine
    {
      if ( config.threadLowEngine.priority != SetupConfig::ThreadProperties::kInvalidPriority ) {
        platformSettings.threadLEngine.nPriority = config.threadLowEngine.priority;
      }
      if ( config.threadLowEngine.affinityMask != SetupConfig::ThreadProperties::kInvalidAffinityMask ) {
        platformSettings.threadLEngine.dwAffinityMask = config.threadLowEngine.affinityMask;
      }
    }

    // Thread Monitor
    {
      if ( config.threadMonitor.priority != SetupConfig::ThreadProperties::kInvalidPriority ) {
        platformSettings.threadMonitor.nPriority = config.threadMonitor.priority;
      }
      if ( config.threadMonitor.affinityMask != SetupConfig::ThreadProperties::kInvalidAffinityMask ) {
        platformSettings.threadMonitor.dwAffinityMask = config.threadMonitor.affinityMask;
      }
    }
    
#ifdef ANDROID
    // Set Android Java environment
    platformSettings.pJavaVM = static_cast<JavaVM*>( config.javaVm );
    platformSettings.jNativeActivity = static_cast<jobject>( config.javaActivity );
#endif // ANDROID
  }
  
  // Music Manager settings
  if ( config.enableMusicEngine ) {
    AK::MusicEngine::GetDefaultInitSettings( musicSettings );
  }
  
  AUDIO_DEBUG( "WwiseComponent.GetDefaultSettings: Audio Engine settings defaulted" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::InitSubModules( const SetupConfig&       config,
                                     AkStreamMgrSettings&     streamManagerSettings,
                                     AkMemSettings&           memoryManagerSettings,
                                     AkDeviceSettings&        deviceSettings,
                                     AkInitSettings&          initSettings,
                                     AkPlatformInitSettings&  platformSettings,
                                     AkMusicSettings&         musicSettings )
{
  // Initialize the Memory Manager
  {
    const AKRESULT result = AK::MemoryMgr::Init( &memoryManagerSettings );
    if ( AK_Success != result )
    {
      AUDIO_LOG( "WwiseComponent.InitSubModules: Memory manager failed to intialize (%d)", result );
      return false;
    }
    
    // Create Memory Pool for PrepareEvents
    const AkMemPoolId poolId = AK::MemoryMgr::CreatePool( NULL, config.defaultMemoryPoolSize, config.defaultPoolBlockSize, AkMalloc );
    if ( AK_INVALID_POOL_ID == poolId ) {
      AUDIO_LOG( "WwiseComponent.InitSubModules: CreatePool failed - AK::MemoryMgr::CreatePool == AK_INVALID_POOL_ID" );
      return false;
    }
    initSettings.uPrepareEventMemoryPoolID = poolId;
    AK::MemoryMgr::SetPoolName( poolId, "EventPool" );
  }
  
  // Initialize the Stream Manager
  {
    const AK::IAkStreamMgr* streamManager = AK::StreamMgr::Create( streamManagerSettings );
    if ( nullptr == streamManager )
    {
      AUDIO_LOG( "WwiseComponent.InitSubModules: Stream manager failed to initialize" );
      return false;
    }
  }

  // Initialize our File Location Resolver
  // + AudioDefaultIOHookBlocking::Init() sets this as our one-and-only File Location Resolver for the Stream Manager
  {
    if ( !InitFileResolver( config, deviceSettings ) ) {
      AUDIO_LOG( "WwiseComponent.InitSubModules: File resolver failed to initialize (%d)", result );
      return false;
    }
  }

  // Initialize the Sound Engine
  {
    const AKRESULT result = AK::SoundEngine::Init( &initSettings, &platformSettings );
    if ( AK_Success != result )
    {
      AUDIO_LOG( "WwiseComponent.InitSubModules: Sound engine failed to initialize (%d)", result );
      return false;
    }
  }
  
  // Initialize Music Engine
  if ( config.enableMusicEngine ) {
    const AKRESULT result = AK::MusicEngine::Init( &musicSettings );
    if ( AK_Success != result )
    {
      AUDIO_LOG( "WwiseComponent.InitSubModules: Music engine failed to initialize (%d)", result );
      return false;
    }
  }
  
  // Initialize Comms for debugging/profiling
#ifndef AK_OPTIMIZED
  {
    AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings( commSettings );

    // Reset Ports to honor setting in AkCommunication.h, which are influenced by
    // AK_COMM_NO_DYNAMIC_PORTS in our build settings
    commSettings.ports = AkCommSettings::Ports();
    
    const AKRESULT result = AK::Comm::Init( commSettings );
    if ( AK_Success != result )
    {
      AUDIO_LOG( "WwiseComponent.InitSubModules: Communications failed to initialize (%d)", result );
    }
  }
#endif // AK_OPTIMIZED

  AUDIO_LOG( "WwiseComponent.InitSubModules: Success!" );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::InitFileResolver( const SetupConfig& config,
                                       AkDeviceSettings&  deviceSettings )
{
  assert( nullptr != _fileLocationResolver );
    
  if ( AK_Success != _fileLocationResolver->Init( deviceSettings, true ) )
  {
    AUDIO_LOG( "WwiseComponent.InitFileResolver: File Location Resolver failed to intialize (%d)", result );
    return false;
  }

#ifdef AK_MAC_OS_X
  // Due to a bug in WWise for OS X (but not for Android or iOS), we have to call SetBasePath
  // even if we are just passing an empty string or we will fail to load the sound banks.
  (void) _fileLocationResolver->SetBasePath("");
#endif // AK_MAC_OS_X
  
  // Let the file resolver know where our asset files are located.
  // Set normal file resolver path
  if ( AK_Success != _fileLocationResolver->SetBasePath( config.assetFilePath.c_str() ) )
  {
    AUDIO_LOG( "WwiseComponent.InitFileResolver: SetBasePath( %s ) failed [error %d]", config.assetFilePath.c_str(), result );
  }

  // Set path resolver function
  _fileLocationResolver->SetPathResolver(config.pathResolver);
  
  // Add Zipped assets paths
  AddZipFiles(config.pathToZipFiles);
  
  // Set Android Asset Manager
#ifdef ANDROID
  if (config.assetManager) {
    _fileLocationResolver->SetAssetManager(config.assetManager);
    _fileLocationResolver->SetAssetBasePath(config.assetManagerBasePath);
  }
#endif // ANDROID

  // Set Write Path
  _fileLocationResolver->SetWriteBasePath(config.writeFilePath);

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::InitSoundBanks( const SetupConfig& config )
{
  // Set our local language.
  const std::string localeStr = GetLanguageLocaleString( config.audioLocale );
  if ( AK_Success != AK::StreamMgr::SetCurrentLanguage( localeStr.c_str() ) )
  {
    AUDIO_LOG( "WwiseComponent.InitSoundBanks: Failed to set the current language [error %d]", result );
  }
  
  AUDIO_LOG( "WwiseComponent.InitSoundBanks: Success!" );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Audio Engine Log static var and function
static LogCallbackFunc _logCallbackFunc = nullptr;
static void AkLocalOutputFunc( AK::Monitor::ErrorCode in_eErrorCode,    // Error code number value
                               const AkOSChar* in_pszError,             // Message or error string to be displayed
                               AK::Monitor::ErrorLevel in_eErrorLevel,  // Specifies whether it should be displayed as a message or an error
                               AkPlayingID in_playingID,                // Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
                               AkGameObjectID in_gameObjID )            // Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
{
  if ( _logCallbackFunc != nullptr ) {
    const auto errorCode = static_cast<const uint32_t>( in_eErrorCode );
    const auto errorMsg = static_cast<const char*>( in_pszError );
    const auto errorLevel = static_cast<const AudioEngine::ErrorLevel>( in_eErrorLevel );
    const auto playingId = static_cast<const AudioPlayingId>( in_playingID );
    const auto gameObjId = static_cast<const AudioGameObject>( in_gameObjID );
    _logCallbackFunc( errorCode, errorMsg, errorLevel, playingId, gameObjId );
  }
}

bool WwiseComponent::SetLogOutput( ErrorLevel errorLevel, LogCallbackFunc logCallback )
{
  bool success = false;
  if ( _isInitialized ) {
    const auto akErrorLevel = static_cast<AK::Monitor::ErrorLevel>( errorLevel );
    AKRESULT result = AK::Monitor::SetLocalOutput( akErrorLevel, &AkLocalOutputFunc );
    success = (AK_Success == result);
    if ( success ) {
      _logCallbackFunc = logCallback;
    }
    else {
      AUDIO_LOG( "WwiseComponent.SetLogOutput: Failed to set Local Output [error %d]", result );
      _logCallbackFunc = nullptr;
    }
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::WriteProfilerCapture( bool write, const std::string& fileName )
{
  if (write) {
    if (fileName.empty()) {
      AUDIO_LOG( "WwiseComponent.WriteProfilerCapture: Invalid File Name" );
      return false;
    }
    const AKRESULT result = AK::SoundEngine::StartProfilerCapture(fileName.c_str());
    if (result != AK_Success) {
      AUDIO_LOG( "WwiseComponent.WriteProfilerCapture: Failed to start profile capture to file '%s'",
                 fileName.c_str() );
      return false;
    }
    return true;
  }
  else {
    const AKRESULT result = AK::SoundEngine::StopProfilerCapture();
    if (result != AK_Success) {
      AUDIO_LOG( "WwiseComponent.WriteProfilerCapture: Failed to stop profile capture" );
      return false;
    }
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::WriteAudioOutputCapture( bool write, const std::string& fileName )
{
  if (write) {
    if (fileName.empty()) {
      AUDIO_LOG( "WwiseComponent.WriteAudioOutputCapture: Invalid File Name" );
      return false;
    }
    const AKRESULT result = AK::SoundEngine::StartOutputCapture(fileName.c_str());
    if (result != AK_Success) {
      AUDIO_LOG( "WwiseComponent.WriteAudioOutputCapture: Failed to start output capture to file '%s'",
                 fileName.c_str() );
      return false;
    }
    return true;
  }
  else {
    const AKRESULT result = AK::SoundEngine::StopOutputCapture();
    if (result != AK_Success) {
      AUDIO_LOG( "WwiseComponent.WriteAudioOutputCapture: Failed to stop output capture" );
      return false;
    }
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::Update( float deltaTime )
{
  if ( _isInitialized )
  {
    AK::SoundEngine::RenderAudio();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::FlushCallbackQueue()
{
  if ( _isInitialized ) {
    // Only use lock to grab callback queue, then unlock
    std::unique_lock<std::mutex> lock{ sCallbackQueueMutex };
    auto callbackQueue = std::move( sCallbackQueue );
    sCallbackQueue.clear();
    lock.unlock();
    
    for ( const auto& kvp : callbackQueue ) {
      AudioCallbackContext* context = kvp.first;
      const AudioCallbackInfo& callbackInfo = *kvp.second;
      context->HandleCallback( callbackInfo );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::LoadBank( const std::string bankName )
{
  AkBankID bankId;
  AKRESULT result;
  result = AK::SoundEngine::LoadBank( bankName.c_str(), AK_DEFAULT_POOL_ID, bankId );
  if ( result != AK_Success ) {
    AUDIO_LOG( "WwiseComponent.LoadBank: Failed to load sound bank (%s) [error %d]", bankName.c_str(), result );
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::UnloadBank( const std::string bankName )
{
  AKRESULT result;
  result = AK::SoundEngine::UnloadBank( bankName.c_str(), NULL );
  if ( result != AK_Success ) {
    AUDIO_LOG( "WwiseComponent.UnloadBank: Failed to unload sound bank (%s) [error %d]", bankName.c_str(), result );
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::PrepareEvents( const std::vector<std::string>& eventNameList, bool load )
{
  // Convert Vector of strings into array of char *
  std::vector< const char * > convertNameList = std::vector< const char * >();
  convertNameList.reserve( eventNameList.size() );
  for (const auto& item : eventNameList ) {
    convertNameList.push_back( item.c_str() );
  }
  const char** events = const_cast<const char**>( convertNameList.data() );
  using namespace AK::SoundEngine;
  PreparationType preparationType = load ? PreparationType::Preparation_Load : PreparationType::Preparation_Unload;
  AKRESULT result;
  result = AK::SoundEngine::PrepareEvent(preparationType, events, static_cast<AkUInt32>( eventNameList.size() ) );
  if ( AK_Success != result )
  {
    std::string eventListStr;
    for ( auto& event : eventNameList ) {
      eventListStr += event;
      eventListStr += ", ";
    }
    AUDIO_LOG( "WwiseComponent.PrepareEvents: Can NOT %s Prepare Events %s", ( load ? "load" : "unLoad" ), eventListStr.c_str() );
    return false;
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::PrepareGameSyncs( const AudioGroupType groupType, const AudioSceneStateGroup stateGroup, bool load )
{
  // Convert Vector of strings into array of char *
  std::vector< const char * > convertNameList = std::vector< const char * >();
  convertNameList.reserve( stateGroup.GroupStates.size() );
  for (const auto& item : stateGroup.GroupStates ) {
    convertNameList.push_back( item.c_str() );
  }
  const char** groupStates = const_cast<const char**>( convertNameList.data() );
  using namespace AK::SoundEngine;
  PreparationType preparationType = load ? PreparationType::Preparation_Load : PreparationType::Preparation_Unload;
  AkGroupType type = ( AudioGroupType::AudioGroupTypeSwitch == groupType ) ? AkGroupType_Switch : AkGroupType_State;
  AKRESULT result;
  result = AK::SoundEngine::PrepareGameSyncs(preparationType, type, stateGroup.StateGroupName.c_str(), groupStates, static_cast<AkUInt32>( stateGroup.GroupStates.size() ) );
  
  if ( AK_Success != result )
  {
    std::string gameSyncListStr;
    for ( auto& aState : stateGroup.GroupStates ) {
      gameSyncListStr += aState;
      gameSyncListStr += ", ";
    }
    AUDIO_LOG( "WwiseComponent.PrepareGameSyncs: Can NOT %s Prepare Game Sync %s", ( load ? "load" : "unLoad" ), gameSyncListStr.c_str() );
    return false;
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::RegisterAudioGameObject( const AudioGameObject gameObjectId, const std::string& optionalName )
{
  bool success = false;
  if ( _isInitialized )
  {
    // Register Game Object
    const AKRESULT result = AK::SoundEngine::RegisterGameObj( gameObjectId, optionalName.c_str() );
    if ( AK_Success == result ) {
      success =  true;
    }
    else {
      AUDIO_LOG( "WwiseComponent.RegisterAudioGameObject: WWise failed to register GameObjectId %lu - %s with AKResult %d",
                (unsigned long)gameObjectId, optionalName.c_str(), result );
    }
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::UnregisterAudioGameObject( const AudioGameObject gameObjectId )
{
  bool success = false;
  if ( _isInitialized )
  {
    const AKRESULT result = AK::SoundEngine::UnregisterGameObj( gameObjectId );
    if ( AK_Success == result ) {
      success = true;
    }
    else {
      AUDIO_LOG( "WwiseComponent.UnregisterAudioGameObject: Failed to unregister GameObjectId %lu with AKResult %d",
                (unsigned long)gameObjectId, result );
    }
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::SetDefaultListeners( const std::vector<AudioGameObject>& listeners)
{
  if ( !_isInitialized || listeners.empty() ) {
    return;
  }
  AK::SoundEngine::SetDefaultListeners( listeners.data(), static_cast<AkUInt32>(listeners.size()) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class InfoType>
static void ExecuteOrQueueCallback( AudioCallbackContext* context, const InfoType& callbackInfo )
{
  if ( context->GetExecuteAsync() )
  {
    context->HandleCallback( callbackInfo );
  }
  else
  {
    // queue callback for later execution
    std::lock_guard<std::mutex> lock{ sCallbackQueueMutex };
    sCallbackQueue.emplace_back( context, std::unique_ptr<const AudioCallbackInfo>( new InfoType( callbackInfo ) ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void EventCallbackFunc( AkCallbackType callbackType, AkCallbackInfo* callbackInfo )
{
  assert( nullptr != callbackInfo );
  
  //FIXME: !!! Callback is called on Engine's thread Need to put it back on our thread !!!!!!!!!!!!!
  
  if ( nullptr != callbackInfo->pCookie )
  {
    AudioCallbackContext* callbackContext = static_cast<AudioCallbackContext*>( callbackInfo->pCookie );
    
    switch ( callbackType ) {
        
      case AK_Duration:
      {
        AkDurationCallbackInfo* akCallback = static_cast<AkDurationCallbackInfo*>( callbackInfo );
        const AudioDurationCallbackInfo callback( static_cast<AudioGameObject>( akCallback->gameObjID ),
                                                  static_cast<AudioReal32>( akCallback->fDuration ),
                                                  static_cast<AudioReal32>( akCallback->fEstimatedDuration ),
                                                  static_cast<AudioUInt32>( akCallback->audioNodeID ),
                                                  akCallback->bStreaming );
        
        ExecuteOrQueueCallback( callbackContext, callback );
      }
        break;
        
      case AK_Marker:
      {
        AkMarkerCallbackInfo* akCallback = static_cast<AkMarkerCallbackInfo*>( callbackInfo );
        const AudioMarkerCallbackInfo callback( static_cast<AudioGameObject>( akCallback->gameObjID ),
                                                static_cast<AudioUInt32>( akCallback->uIdentifier ),
                                                static_cast<AudioUInt32>( akCallback->uPosition ),
                                                akCallback->strLabel );
        
        ExecuteOrQueueCallback( callbackContext, callback );
      }
        break;
        
      case AK_EndOfEvent:
      {
        AkEventCallbackInfo* akCallback = static_cast<AkEventCallbackInfo*>( callbackInfo );
        const AudioCompletionCallbackInfo callback( static_cast<AudioGameObject>( akCallback->gameObjID ),
                                                    static_cast<AudioPlayingId>( akCallback->playingID ),
                                                    static_cast<AudioEventId>( akCallback->eventID ) );
        
        ExecuteOrQueueCallback( callbackContext, callback );
        // Note: This is ideal if we are only using AK_EndOfEvent callback, not sure how this will effect
        // AK_EndOfDynamicSequenceItem
        AK::SoundEngine::CancelEventCallback( callbackContext->GetPlayId() );
      }
        break;
        
      case AK_Starvation:
      {
        AkEventCallbackInfo* akCallback = static_cast<AkEventCallbackInfo*>( callbackInfo );
        const AudioErrorCallbackInfo callback( static_cast<AudioGameObject>( akCallback->gameObjID ),
                                               static_cast<AudioPlayingId>( akCallback->playingID ),
                                               static_cast<AudioEventId>( akCallback->eventID ),
                                               AudioCallbackErrorType::Starvation );
      }
        break;
        
      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Only use this method if the CallbackFlag is Not equal to AudioCallbackFlag:NoCallback
AkCallbackType ConvertCallbackType( AudioCallbackFlag flags )
{
  AkCallbackType akType = (AkCallbackType)0;
  
  if ( (AudioCallbackFlag::Duration & flags) == AudioCallbackFlag::Duration ) {
    akType = (AkCallbackType)( akType | AK_Duration );
  }
  
  if ( (AudioCallbackFlag::Marker & flags) == AudioCallbackFlag::Marker ) {
    akType = (AkCallbackType)( akType | AK_Marker );
  }
  
  if ( (AudioCallbackFlag::Complete & flags) == AudioCallbackFlag::Complete ) {
    akType = (AkCallbackType)( akType | AK_EndOfEvent );
  }
  
  return akType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlayingId WwiseComponent::PostEvent( AudioEventId eventId, AudioGameObject gameObject, AudioCallbackContext* callbackContext )
{
  AudioPlayingId audioId = kInvalidAudioPlayingId;
  
  if ( _isInitialized )
  {
    if ( nullptr != callbackContext ) {
      AkCallbackType akFlags = ConvertCallbackType( callbackContext->GetCallbackFlags() );
      // Post Event with callback, always use AK_EndOfEvent to manage callback context object
      audioId = AK::SoundEngine::PostEvent( eventId, gameObject, akFlags | AK_EndOfEvent, &EventCallbackFunc, callbackContext );
    }
    else {
      audioId = AK::SoundEngine::PostEvent( eventId, gameObject);
    }
    AUDIO_LOG( "WwiseComponent.PostEvent: Posted audio event %u, resulted in play id [%u]", eventId, audioId );
  }
  
  return audioId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::StopAllAudioEvents( AudioGameObject gameObject )
{
  if ( _isInitialized )
  {
    AK::SoundEngine::StopAll( gameObject );
    AUDIO_LOG( "WwiseComponent.StopAllAudioEvents: Stopping all audio events on game object [%u]", static_cast<uint32_t>(gameObject) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private function
AkCurveInterpolation AudioCurveTypeToAkCurveInterpolation( AudioCurveType curveType )
{
  AkCurveInterpolation curveInterpolation = AkCurveInterpolation_Linear;
  switch ( curveType ) {
      
    case AudioCurveType::Linear:
      curveInterpolation = AkCurveInterpolation_Linear;
      break;
      
    case AudioCurveType::SCurve:
      curveInterpolation = AkCurveInterpolation_SCurve;
      break;
      
    case AudioCurveType::InversedSCurve:
      curveInterpolation = AkCurveInterpolation_InvSCurve;
      break;
      
    case AudioCurveType::Sine:
      curveInterpolation = AkCurveInterpolation_Sine;
      break;
      
    case AudioCurveType::SineReciprocal:
      curveInterpolation = AkCurveInterpolation_SineRecip;
      break;
      
    case AudioCurveType::Exp1:
      curveInterpolation = AkCurveInterpolation_Exp1;
      break;
      
    case AudioCurveType::Exp3:
      curveInterpolation = AkCurveInterpolation_Exp3;
      break;
      
    case AudioCurveType::Log1:
      curveInterpolation = AkCurveInterpolation_Log1;
      break;
      
    case AudioCurveType::Log3:
      curveInterpolation = AkCurveInterpolation_Log3;
      break;
      
    default:
      break;
  }
  
  return curveInterpolation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetRTPCValue( AudioParameterId parameterId, AudioRTPCValue rtpcValue, AudioGameObject gameObject, AudioTimeMs valueChangeDuration, AudioCurveType curve )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    AkCurveInterpolation curveInterpolation = AudioCurveTypeToAkCurveInterpolation(curve);
    AKRESULT result = AK::SoundEngine::SetRTPCValue( parameterId, rtpcValue, gameObject, valueChangeDuration, curveInterpolation );
    AUDIO_DEBUG( "WwiseComponent.SetRTPCValue: Set RTPC value %f on audio parameterId %d, result %d", rtpcValue, parameterId, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetRTPCValueWithPlayingId( AudioParameterId parameterId, AudioRTPCValue rtpcValue, AudioPlayingId playingId, AudioTimeMs valueChangeDuration, AudioCurveType curve )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    AkCurveInterpolation curveInterpolation = AudioCurveTypeToAkCurveInterpolation(curve);
    AKRESULT result = AK::SoundEngine::SetRTPCValueByPlayingID( parameterId, rtpcValue, playingId, valueChangeDuration, curveInterpolation );
    AUDIO_DEBUG( "WwiseComponent.SetRTPCValue: Set RTPC value %f on audio parameterId %d, result %d", rtpcValue, parameterId, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::GetRTPCValue( AudioParameterId parameterId,
                                   AudioGameObject gameObject,
                                   AudioPlayingId playingId,
                                   AudioRTPCValue& out_rtpcValue,
                                   AudioRTPCValueType& inOut_rtpcValueType )
{
  bool success = false;
  if ( _isInitialized ) {
    auto inOut_type = ToRTCPValueType(inOut_rtpcValueType);
    AKRESULT result = AK::SoundEngine::Query::GetRTPCValue( parameterId, gameObject, playingId, out_rtpcValue, inOut_type);
    AUDIO_DEBUG( "WwiseComponent.GetRTPCValue: Get RTPC value %f on audio parameterId %d, result %d", out_rtpcValue, parameterId, result );
    inOut_rtpcValueType = ToRTCPValueType(inOut_type);
    success = ( AKRESULT::AK_Success == result );
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetSwitch( AudioSwitchGroupId switchGroupId, AudioSwitchStateId switchStateId, AudioGameObject gameObject )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    AKRESULT result = AK::SoundEngine::SetSwitch( switchGroupId, switchStateId, gameObject );
    AUDIO_DEBUG( "WwiseComponent.SetRTPCValue: Set Switch group %u to state %u, result %d", switchGroupId, switchStateId, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetState( AudioStateGroupId stateGroupId, AudioStateId stateId )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    AKRESULT result = AK::SoundEngine::SetState( stateGroupId, stateId );
    AUDIO_LOG( "WwiseComponent.SetState: Set State GroupId %u to StateId %u, result %d", stateGroupId, stateId, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetGameObjectAuxSendValues( AudioGameObject gameObject,
                                                 const std::vector<AudioAuxBusValue>& auxSendValues )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    // Create aux send array if there are values
    const AkUInt32 valueCount = static_cast<AkUInt32>( auxSendValues.size() );
    AkAuxSendValue* akSendValues = nullptr;
    if ( valueCount > 0 ) {
      akSendValues = new AkAuxSendValue[ valueCount ];
      for ( size_t idx = 0; idx < valueCount; ++idx ) {
        akSendValues[idx].auxBusID = auxSendValues[idx].auxBusId;
        akSendValues[idx].fControlValue = auxSendValues[idx].controlValue;
      }
    }
    // Set GameObject's aux buses
    // Pass null into sendValues and 0 for valueCount to remove aux buses
    AKRESULT result = AK::SoundEngine::SetGameObjectAuxSendValues( gameObject, akSendValues, valueCount );
    Anki::Util::SafeDeleteArray( akSendValues );
    AUDIO_LOG( "WwiseComponent.SetGameObjectAuxSendValues: Set GameObject %u bus count %u, result %d",
              static_cast<uint32_t>(gameObject), valueCount, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::SetGameObjectOutputBusVolume( AudioGameObject emitterGameObj,
                                                   AudioGameObject listenerGameObj,
                                                   AudioReal32 controlVolume )
{
  bool success = false;
  
  if ( _isInitialized )
  {
    AKRESULT result = AK::SoundEngine::SetGameObjectOutputBusVolume( emitterGameObj, listenerGameObj, controlVolume );
    AUDIO_LOG( "WwiseComponent.SetGameObjectOutputBusVolume: Set EmitterGameObj %u listenerGameObj %u output bus volume %f, result %d",
              emiterGameObj, listenerGameObj, controlVolume, result );
    success = ( AKRESULT::AK_Success == result);
  }
  
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t WwiseComponent::GetAudioIdFromString( const std::string& name )
{
  if ( !name.empty() ) {
    return AK::SoundEngine::GetIDFromString( name.c_str() );
  }
  return kInvalidAudioEventId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioGameObject WwiseComponent::GetDefaultGameObjectId()
{
  return AK_INVALID_GAME_OBJECT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::SetRandomSeed( uint16_t seed )
{
  if ( _isInitialized )
  {
    AK::SoundEngine::SetRandomSeed( (AkInt32)seed );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::CheckFileExists( const std::string& fileName )
{
  if ( _fileLocationResolver != nullptr ) {
    return ( _fileLocationResolver->CheckFileExists( fileName.c_str() ) == AK_Success );
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineCallbackFlag ConvertEngineCallbackFlag( AkGlobalCallbackLocation flag );
AkGlobalCallbackLocation ConvertEngineCallbackFlag( AudioEngineCallbackFlag flag );
  
void EngineGlobalCallbackFunc( AK::IAkGlobalPluginContext * in_pContext,
                               AkGlobalCallbackLocation in_eLocation,
                               void * in_pCookie )
{
  WwiseComponent::PerformEngineGlobalCallback( ConvertEngineCallbackFlag(in_eLocation),
                                               (WwiseComponent*)in_pCookie );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngineCallbackId WwiseComponent::RegisterGlobalCallback( AudioEngineCallbackFlag callbackFlag,
                                                              AudioEngineCallbackFunc&& callbackFunc )
{
  // Check for Invalid Id
  if ( ++_lastEngineCallbackId == kInvalidAudioEngineCallbackId ) {
    // Value rolled over
    ++_lastEngineCallbackId;
  }
  
  // Verify that the Id is not already registered
  while ( _engineCallbackIdMap.find( _lastEngineCallbackId ) != _engineCallbackIdMap.end() ) {
    // NOTE: This will loop forever if there are AudioEngineCallbackId (uint32) max registered callbacks
    ++_lastEngineCallbackId;
  }

  // Store callback context
  std::lock_guard<std::mutex> lock( _audioEngineCallbackMutex );
  // Note: context is given to shared_ptr don't need to worry about memory
  auto context = AudioEngineCallbackContextPtr( new AudioEngineCallbackContext( _lastEngineCallbackId,
                                                                                callbackFlag,
                                                                                std::move( callbackFunc ) ) );
  const auto idMapIt = _engineCallbackIdMap.emplace( _lastEngineCallbackId, context );
  // Add
  if ( idMapIt.second ) {
    // Emplace Success
    const auto& contextPtr = idMapIt.first->second;
    for ( auto& typeMapIt : _engineCallbackTypeMap ) {
      // If callback type flags is set add to TypeMap
      AudioEngineCallbackFlag mapType = typeMapIt.first;
      AudioEngineCallbackContextSet& mapContextSet = typeMapIt.second;
      if ( ( mapType & contextPtr->CallbackFlag ) == mapType ) {
        // Add to Type Map Set
        const auto setIt = mapContextSet.emplace( contextPtr );
        if ( setIt.second && ( mapContextSet.size() == 1 ) ) {
          // Context was successfully added to type set and is first callback in the set
          // Register for Wwise Global Callbacks of this type
          AK::SoundEngine::RegisterGlobalCallback( &EngineGlobalCallbackFunc,
                                                   ConvertEngineCallbackFlag( mapType ),
                                                   this );
        }
      }
    }
  }
  else {
    // Error
    PRINT_NAMED_ERROR("WwiseComponent.RegisterGlobalCallback", "_engineCallbackIdMap.emplace.Failed");
  }
  return _lastEngineCallbackId;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WwiseComponent::UnregisterGlobalCallback( AudioEngineCallbackId callbackId )
{
  // Find callback context for Id
  std::lock_guard<std::mutex> lock( _audioEngineCallbackMutex );
  const auto idMapIt = _engineCallbackIdMap.find( callbackId );
  if ( idMapIt == _engineCallbackIdMap.end() ) {
    // Not found
    return false;
  }
  
  // Find Context in Type Map & remove all copies
  bool didFind = false;
  const auto& contextPtr = idMapIt->second;
  for ( auto& entryTypeIt : _engineCallbackTypeMap ) {
    // Search for ContextPtr in each set
    AudioEngineCallbackFlag mapType = entryTypeIt.first;
    AudioEngineCallbackContextSet& mapContextSet = entryTypeIt.second;
    const auto contextIt = mapContextSet.find( contextPtr );
    if ( contextIt != mapContextSet.end() ) {
      // Found contextPtr, Remove it
      mapContextSet.erase( contextIt );
      if ( mapContextSet.empty() ) {
        //  Unregister with Wwise Global Callback if no more callbacks in that type
        AK::SoundEngine::UnregisterGlobalCallback( &EngineGlobalCallbackFunc,
                                                   ConvertEngineCallbackFlag( mapType ) );
      }
      didFind = true;
    }
  }
  // Remove from IdMap
  _engineCallbackIdMap.erase( idMapIt );
  
  if ( !didFind ) {
    PRINT_NAMED_ERROR("WwiseComponent.UnregisterGlobalCallback", "_engineCallbackTypeMap.Callback.NotFound");
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WwiseComponent::PerformEngineGlobalCallback( AudioEngineCallbackFlag flag, WwiseComponent* component )
{
  std::lock_guard<std::mutex> lock( component->_audioEngineCallbackMutex );
  const auto typeMapIt = component->_engineCallbackTypeMap.find( flag );
  if ( typeMapIt != component->_engineCallbackTypeMap.end() ) {
    ASSERT_NAMED(!typeMapIt->second.empty(), "Context Set is Empty!");
    // Perform all callbacks for flag type
    for ( const auto& contextPtr : typeMapIt->second ) {
      contextPtr->CallbackFunc( contextPtr->CallbackId, flag );
    }
  }
  else {
    PRINT_NAMED_ERROR("WwiseComponent.PerformEngineGlobalCallback",
                      "_engineCallbackTypeMap.AudioEngineCallbackFlag.NotFound");
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string WwiseComponent::GetLanguageLocaleString( AudioLocaleType locale )
{
  // See also https://www.audiokinetic.com/library/edge/?source=SDK&id=bankscommandline.html
  static const std::unordered_map<AudioLocaleType, std::string, Anki::Util::EnumHasher> localeMap {
    { AudioLocaleType::EnglishUS, "English(US)" },
    { AudioLocaleType::German, "German" },
    { AudioLocaleType::FrenchFrance, "French(France)" },
    { AudioLocaleType::Japanese, "Japanese" }
  };
  
  const auto& it = localeMap.find( locale );
  if ( it != localeMap.end() ) {
    return it->second;
  }
  
  // By default return English
  return "English(US)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioRTPCValueType ToRTCPValueType( AK::SoundEngine::Query::RTPCValue_type type )
{
  AudioRTPCValueType result = AudioRTPCValueType::Unavailable;
  switch ( type ) {
    case AK::SoundEngine::Query::RTPCValue_Default:
      result = AudioRTPCValueType::Default;
      break;
    case AK::SoundEngine::Query::RTPCValue_Global:
      result = AudioRTPCValueType::Global;
      break;
    case AK::SoundEngine::Query::RTPCValue_GameObject:
      result = AudioRTPCValueType::GameObject;
      break;
    case AK::SoundEngine::Query::RTPCValue_PlayingID:
      result = AudioRTPCValueType::PlayingId;
      break;
    case AK::SoundEngine::Query::RTPCValue_Unavailable:
      result = AudioRTPCValueType::Unavailable;
      break;
  }
  return result;
}

AK::SoundEngine::Query::RTPCValue_type ToRTCPValueType( AudioRTPCValueType type )
{
  AK::SoundEngine::Query::RTPCValue_type result = AK::SoundEngine::Query::RTPCValue_Default;
  switch ( type ) {
    case AudioRTPCValueType::Default:
      result = AK::SoundEngine::Query::RTPCValue_Default;
      break;
    case AudioRTPCValueType::Global:
      result = AK::SoundEngine::Query::RTPCValue_Global;
      break;
    case AudioRTPCValueType::GameObject:
      result = AK::SoundEngine::Query::RTPCValue_GameObject;
      break;
    case AudioRTPCValueType::PlayingId:
      result = AK::SoundEngine::Query::RTPCValue_PlayingID;
      break;
    case AudioRTPCValueType::Unavailable:
      result = AK::SoundEngine::Query::RTPCValue_Unavailable;
      break;
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline AkGlobalCallbackLocation operator| ( AkGlobalCallbackLocation a, AkGlobalCallbackLocation b )
{ return static_cast<AkGlobalCallbackLocation>( static_cast<uint>(a) | static_cast<uint>(b) ); }

AudioEngineCallbackFlag ConvertEngineCallbackFlag( AkGlobalCallbackLocation flag )
{
  AudioEngineCallbackFlag engineCallbackFlag = AudioEngineCallbackFlag::NoAudioEngineCallbacks;
  
  if ( (AkGlobalCallbackLocation_BeginRender & flag) == AkGlobalCallbackLocation_BeginRender ) {
    engineCallbackFlag = engineCallbackFlag | AudioEngineCallbackFlag::BegingFrameRender;
  }
  
  if ( (AkGlobalCallbackLocation_EndRender & flag) == AkGlobalCallbackLocation_EndRender ) {
    engineCallbackFlag = engineCallbackFlag | AudioEngineCallbackFlag::EndFrameRender;
  }
  
  if ( (AkGlobalCallbackLocation_End & flag) == AkGlobalCallbackLocation_End ) {
    engineCallbackFlag = engineCallbackFlag | AudioEngineCallbackFlag::EndAudioProcessing;
  }
  
  return engineCallbackFlag;
}

AkGlobalCallbackLocation ConvertEngineCallbackFlag( AudioEngineCallbackFlag flag )
{
  AkGlobalCallbackLocation callbackLoc = static_cast<AkGlobalCallbackLocation>(0);
  if ( (AudioEngineCallbackFlag::BegingFrameRender & flag) == AudioEngineCallbackFlag::BegingFrameRender ) {
    callbackLoc = callbackLoc | AkGlobalCallbackLocation_BeginRender;
  }
  
  if ( (AudioEngineCallbackFlag::EndFrameRender & flag) == AudioEngineCallbackFlag::EndFrameRender ) {
    callbackLoc = callbackLoc | AkGlobalCallbackLocation_EndRender;
  }
  
  if ( (AudioEngineCallbackFlag::EndAudioProcessing & flag) == AudioEngineCallbackFlag::EndAudioProcessing ) {
    callbackLoc = callbackLoc | AkGlobalCallbackLocation_End;
  }
  return callbackLoc;
}

} // AudioEngine
} // Anki
