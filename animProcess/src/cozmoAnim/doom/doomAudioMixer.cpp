
#include "cozmoAnim/doom/doomAudioMixer.h"
#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "audioEngine/audioTypeTranslator.h"
#include "clad/types/sayTextStyles.h"

namespace {
  constexpr Anki::AudioMetaData::GameObjectType kGameObject
    = Anki::AudioMetaData::GameObjectType::Cozmo_OnDevice;
  constexpr Anki::AudioMetaData::GameEvent::GenericEvent kEvent
    = Anki::AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Unprocessed;
}

void DoomAudioMixer::Play( const Anki::AudioEngine::StandardWaveDataContainer* container, bool looping)
{
  PRINT_NAMED_WARNING("DOOM","playing music in DoomAudioMixer");
  std::lock_guard<std::mutex> lock(_mutex);
  _playQueue.emplace( QueueEntry{ container, looping } );
}

void DoomAudioMixer::FlushPlayQueue()
{
  std::lock_guard<std::mutex> lock(_mutex);
  while( !_playQueue.empty() ) {
    PlayInternal( _playQueue.front().container, _playQueue.front().looping );
    _playQueue.pop();
  }
}

void DoomAudioMixer::PlayInternal( const Anki::AudioEngine::StandardWaveDataContainer* container, bool looping)
{
  if( _audioController == nullptr ) {
    return;
  }
  
  PRINT_NAMED_WARNING("DOOM","playing music in DoomAudioMixer::PlayInternal");
  
  auto* pluginInterface = _audioController->GetPluginInterface();
  DEV_ASSERT(nullptr != pluginInterface, "TextToSpeechComponent.PrepareAudioEngine.InvalidPluginInterface");
  
  // Clear previously loaded data
  if (pluginInterface->WavePortalHasAudioDataInfo()) {
    pluginInterface->ClearWavePortalAudioData();
  }
  
  using namespace Anki::AudioEngine;
  
  AudioCallbackContext* callbackContext = nullptr;
  if( looping ) {
    callbackContext = new AudioCallbackContext();
    callbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
    callbackContext->SetExecuteAsync( false ); // run on main thread. the first call of play should be on main thread too (it isnt yet)
    callbackContext->SetEventCallbackFunc( [this, container]( const AudioCallbackContext* thisContext,
                                                   const AudioCallbackInfo& callbackInfo )
    {
      // restart. don't restart if a different container has since been passed
      if( _lastContainer == container ) {
        // don't worry about stack overflow from recursion since this goes through the main
        // thread queue. obviously the sound buffer must still be around.
        Play( container, true);
      } // otherwise another tune has started already. ignore
    });
  }
  
  _lastContainer = container;
  
  
  StandardWaveDataContainer copy( container->sampleRate, container->numberOfChannels, container->bufferSize);
  for( size_t i=0; i<container->bufferSize; ++i ) {
    copy.audioBuffer[i] = container->audioBuffer[i];
  }
  // Pass ownership of audio data to plugin
  PRINT_NAMED_WARNING("DOOM","giving up wav data (length %zu) ownership in DoomAudioMixer::PlayInternal", container->bufferSize);
  pluginInterface->GiveWavePortalAudioDataOwnership(&copy);
  copy.ReleaseAudioDataOwnership();
  
  const auto eventId = static_cast<AudioEventId>(kEvent);
  const auto gameObject = static_cast<AudioGameObject>(kGameObject);
  
  PRINT_NAMED_WARNING("DOOM","setting switch in DoomAudioMixer::PlayInternal");
  const auto switchGroup = Anki::AudioMetaData::SwitchState::SwitchGroupType::Cozmo_Voice_Processing;
  const auto switchState = Anki::Cozmo::SayTextVoiceStyle::Unprocessed;
  _audioController->SetSwitchState(static_cast<AudioSwitchGroupId>(switchGroup),
                                   static_cast<AudioSwitchStateId>(switchState),
                                   gameObject);
  
  
  PRINT_NAMED_WARNING("DOOM","playing music in DoomAudioMixer::PlayInternal");
  _audioController->PostAudioEvent(eventId, gameObject, callbackContext);
}
