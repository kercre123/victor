/***********************************************************************************************************************
 *
 *  AudioPlaybackJob
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 4/10/2018
 *
 *  Description
 *  + System to load and playback recordings/audio files/etc
 *
 **********************************************************************************************************************/

// #include "cozmoAnim/audio/audioPlaybackJob.h"
#include "audioPlaybackJob.h"
#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTools/audioWaveFileReader.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "audioUtil/waveFile.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "micDataTypes.h"

#include "util/logging/logging.h"

#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"

#include <string>
#include <thread>


namespace Anki {
namespace Cozmo {
namespace Audio {

using namespace AudioUtil;
using namespace AudioEngine;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlaybackJob::AudioPlaybackJob( CozmoAudioController* audioController, const std::string& filename ) :
  _filename( filename ),
  _audioController( audioController ),
  _data( nullptr ),
  _isComplete( false )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlaybackJob::~AudioPlaybackJob()
{
  Anki::Util::SafeDelete( _data );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackJob::Update()
{
  if ( !IsComplete() )
  {
    // first we need to load the data
    if ( !IsDataLoaded() )
    {
      LoadAudioData();

      if ( !IsDataLoaded() )
      {
        // failed to load the .wav file, just bail
        SetComplete();
        return;
      }
    }

    // now wait until we can play our data back
    if ( CanPlaybackAudioData() )
    {
      PlaybackAudioData();

      // whether the playback worked or not, we've completed our job
      SetComplete();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackJob::SetComplete()
{
  _isComplete = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioPlaybackJob::IsComplete() const
{
  return _isComplete;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackJob::LoadAudioData()
{
  // make sure we delete any old data
  Anki::Util::SafeDelete( _data );

  // load in our new wav file
  _data = AudioWaveFileReader::LoadWaveFile( _filename );

  if ( nullptr != _data )
  {
    PRINT_CH_INFO( "MicData", "AudioPlaybackJob",
                   "Successful loaded .wav file [rate:%u] [channels:%u] [samples:%u]",
                   _data->sampleRate,
                   (uint32_t)_data->numberOfChannels,
                   (uint32_t)_data->bufferSize );
  }
  else
  {
    PRINT_CH_INFO( "MicData", "AudioPlaybackJob", "Failed to load .wav file (%s)", _filename.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioPlaybackJob::CanPlaybackAudioData() const
{
  // need data in order to play it back
  bool playbackAllowed = IsDataLoaded();

  // note: turns out the audio data from the plugin is never cleared, so we can't wait on it to be free, we just
  //       need to crush it.  We probably want to add a callback when the playback is done so we can let others know,
  //       as well as clear the audio data in the plugin
//  AudioEngine::PlugIns::AnkiPluginInterface* const plugin = _audioController->GetPluginInterface();
//  DEV_ASSERT( nullptr != plugin, "AudioPlaybackJob.InvalidPluginInterface" );
//
//  // wait until other audio is finished playing before we allow our audio to proceed (else we'll stomp it)
//  playbackAllowed &= !plugin->WavePortalHasAudioDataInfo();

  return playbackAllowed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackJob::PlaybackAudioData()
{
  DEV_ASSERT_MSG( CanPlaybackAudioData(), "AudioPlaybackJob", "Cannot playback audio data as requested" );

  AudioEngine::PlugIns::AnkiPluginInterface* const plugin = _audioController->GetPluginInterface();
  DEV_ASSERT( nullptr != plugin, "AudioPlaybackJob.InvalidPluginInterface" );

  // clear out any old audio data
  if ( plugin->WavePortalHasAudioDataInfo() )
  {
    plugin->ClearWavePortalAudioData();
  }

  // give our audio data over to the plugin and release our memory to it
  plugin->GiveWavePortalAudioDataOwnership( _data );
  _data->ReleaseAudioDataOwnership();

  if ( plugin->WavePortalHasAudioDataInfo() )
  {
    PRINT_CH_INFO( "MicData", "AudioPlaybackJob", "Data transferred to audio plugin" );
  }

  using namespace Anki::AudioMetaData;
  {
    // now post this message to the audio engine which tells it to play the chunk of memeory we just passed to the plugin
    const AudioEngine::AudioEventId audioId = AudioEngine::ToAudioEventId( GameEvent::GenericEvent::Play__Robot_Vic__External_Voice_Message );
    const AudioEngine::AudioGameObject audioGameObject = AudioEngine::ToAudioGameObject( GameObjectType::VoiceRecording );

    _audioController->PostAudioEvent( audioId, audioGameObject );
  }

  Anki::Util::SafeDelete( _data );
}

} // Audio
} // namespace Cozmo
} // namespace Anki
