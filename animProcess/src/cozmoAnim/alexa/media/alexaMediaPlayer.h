/**
 * File: alexaMediaPlayer.h
 *
 * Author: ross
 * Created: Oct 20 2018
 *
 * Description: A streaming mp3 decoder for Alexa's voice and other audio.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

/***************************************************************************************************
NOTE:  This is shitty prototype code. I'm putting it in master so that Alexa will start speaking sooner.
TODO (VIC-9853): re-implement this properly. I think it should more closely resemble MediaPlayer.h/cpp,
                 and there should not be a thread::sleep
*************************************************************************************************/

// Since this file uses the sdk, here's the SDK license
/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAMEDIAPLAYER_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAMEDIAPLAYER_H

#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTools/streamingWaveDataInstance.h"
#include "audioEngine/audioTypes.h"

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

// TODO: some of these could be fwd declared
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <PlaylistParser/UrlContentToAttachmentConverter.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <unordered_set>


struct mpg123_handle_struct;

namespace Anki {
  
namespace AudioMetaData {
  namespace GameEvent {
    enum class GenericEvent : uint32_t;
  }
  namespace GameParameter {
    enum class ParameterType : uint32_t;
  }
}

namespace AudioMetaData {
  namespace GameEvent {
    enum class GenericEvent : uint32_t;
  }
  namespace GameParameter {
    enum class ParameterType : uint32_t;
  }
}

namespace Util {
  namespace Dispatch {
    class Queue;
  }
  template <typename T> class RingBuffContiguousRead;
}

namespace Vector {

class AlexaReader;
namespace Anim {
  class AnimContext;
}
struct AudioInfo;
namespace Audio {
  class CozmoAudioController;
}
class SpeechRecognizerSystem;
class SpeechRecognizerTHF;


class AlexaMediaPlayer : public alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface
                       , public alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface
                       , public alexaClientSDK::playlistParser::UrlContentToAttachmentConverter::ErrorObserverInterface
                       , public alexaClientSDK::avsCommon::utils::RequiresShutdown
                       , public std::enable_shared_from_this<AlexaMediaPlayer>
{
  using SourceId = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface::SourceId;
public:

  enum class Type : uint8_t {
    TTS,
    Alerts,
    Audio,
    Notifications
  };

  AlexaMediaPlayer( Type type,
                    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory );
  ~AlexaMediaPlayer();

  void Init( const Anim::AnimContext* context );

  void Update();

  virtual SourceId setSource( std::shared_ptr< alexaClientSDK::avsCommon::avs::attachment::AttachmentReader > attachmentReader,
                              const alexaClientSDK::avsCommon::utils::AudioFormat* format=nullptr ) override;
  virtual SourceId setSource( const std::string &url, std::chrono::milliseconds offset = std::chrono::milliseconds::zero() ) override;
  virtual SourceId setSource( std::shared_ptr<std::istream> stream, bool repeat) override;

  virtual bool play( SourceId id ) override;
  virtual bool stop( SourceId id ) override;
  virtual bool pause( SourceId id ) override;
  virtual bool resume( SourceId id ) override;
  virtual std::chrono::milliseconds getOffset( SourceId id ) override;
  virtual uint64_t getNumBytesBuffered() override;
  virtual void setObserver( std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface> playerObserver ) override;
  virtual bool setVolume( int8_t volume ) override;
  virtual bool adjustVolume( int8_t delta ) override;
  virtual bool setMute( bool mute ) override;
  virtual bool getSpeakerSettings( SpeakerSettings* settings ) override;
  virtual alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type getSpeakerType() override;
  virtual void onError() override; // a UrlContentToAttachmentConverter experienced an error
  virtual void doShutdown() override;

  // true if this player is or is about to become active (playing, decode, etc.)
  bool IsActive();

private:

  Type _type;
  alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings _settings;

  using StreamingWaveDataPtr = std::shared_ptr<AudioEngine::StreamingWaveDataInstance>;
  using AudioController = Audio::CozmoAudioController;
  using DispatchQueue = Util::Dispatch::Queue;
  
  // Set audio variables for Media Player based off it's _name
  void SetMediaPlayerAudioEvents();
  
  // Set the volume in Audio Controller for Media Player
  void SetPlayerVolume() const;
  void OnSettingsChanged() const;

  // decodes from _mp3Buffer into data, returns millisec decoded
  int Decode( const StreamingWaveDataPtr& data, bool flush );
  
  // copied size shorts in _decodedPcm into waveData, returning the number of milliseconds
  float CopyToWaveData( size_t size, const StreamingWaveDataPtr& waveData, bool flush );

  void CallOnPlaybackFinished( SourceId id, bool runOnCaller = false );
  void CallOnPlaybackError( SourceId id );
  
  bool StopInternal( SourceId id, bool runOnCaller = false );

  void SavePCM( short* buff, size_t size=0 ) const;
  void SaveMP3( const unsigned char* buff, size_t size=0 ) const;

  std::string GetSettingsFilename() const;
  void SaveSettings() const;
  void LoadSettings();

  void OnNewSourceSet();

  void LogPlayingSourceMismatchEvent(const std::string& func, SourceId id, SourceId playingId);
  
  mpg123_handle_struct* _decoderHandle = nullptr;
  
  enum class State {
    Idle=0,
    Preparing,
    Playable,
    Playing,
    Paused,
    ClipPlayable,
    ClipPlaying,
  };
  std::atomic<State> _state;
  std::atomic<State> _stateBeforePause;

  void SetState( State state );

  const char* const StateToString() const;
  static const char* const StateToString(const State& state);

  // return true if set, only set state if it is currently desired (atomically), otherwise returns false and
  // doesn't impact _state
  bool ExchangeState( State expectedCurrentState, State desiredState );

  // static to keep all instances of this object returning unique source IDs
  static std::atomic<SourceId> _nextAvailableSourceID;

  SourceId GetNewSourceID();

  volatile SourceId _playingSource = 0;
  volatile SourceId _nextSourceToPlay = 0;

  std::map<SourceId, std::unique_ptr< AlexaReader >> _readers;
  static constexpr size_t kMaxReadSize = 16384; // 16k
  std::array<short,kMaxReadSize> _decodedPcm;
  
  struct MediaInfo {
    int channels;
    long sampleRate;
  };
  MediaInfo _mediaInfo;
  
  
  // A baked in wwise event that should play once state is ClipPlayable
  AudioEngine::AudioEventId _playableClip;
  // Time the baked-in wwise event started
  float _clipStartTime_ms = -1.0f;

  // An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
  alexaClientSDK::avsCommon::utils::threading::Executor _executor;

  /// The set of MediaPlayerObserverInterface instances to notify of state changes
  std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface>> _observers;

  std::unique_ptr<Util::RingBuffContiguousRead<uint8_t>> _mp3Buffer;

  uint64_t _offset_ms = 0;
  bool _firstPass = true;
  size_t _attemptedDecodeBytes = 0;
  
  // whether we're downloading a format that we can process
  enum class DataValidity : uint8_t {
    Unknown=0,
    Valid,
    Invalid,
  };
  DataValidity _dataValidity;

  StreamingWaveDataPtr _waveData;

  std::string _cacheSaveFolder;
  std::string _persistentSaveFolder;

  // worker thread
  DispatchQueue* _dispatchQueue = nullptr;

  // audio controller provided by context
  AudioController* _audioController = nullptr;
  
  // The ideal amount of audio samples in buffer
  size_t _idealBufferSampleSize = 0;
  // Size of buffer before starting playback
  size_t _minPlaybackBufferSize = 0;

  // Used to create objects that can fetch remote HTTP content.
  std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> _contentFetcherFactory;

  // Used to stream urls into attachments
  std::shared_ptr<alexaClientSDK::playlistParser::UrlContentToAttachmentConverter> _urlConverter;
  // _urlConverter generated an error
  std::atomic_bool _errorDownloading{ false };

  const AudioInfo& _audioInfo;
  
  // for ensuring callbacks don't fire after this class was deleted
  using AudioCallbackType = std::function<void(SourceId)>;
  std::shared_ptr<AudioCallbackType> _audioPlaybackFinishedPtr;
  std::shared_ptr<AudioCallbackType> _audioPlaybackErrorPtr;
  
  std::atomic<bool> _shuttingDown;
  // guards access to _observers. not sure of order of sdk calls, so recursive just in case
  std::recursive_mutex _observerMutex;

  // todo: these three wouldn't be necessary if we had a thread instead of a dispatch queue. We basically need to
  // join() the play/decoding thread during doShutdown(). In the meantime, we wait for a condition variable

  volatile bool _playLoopRunning;
  std::condition_variable _playLoopRunningCondition;

  // held the entire time the play loop is active
  std::mutex _playLoopMutex;
  
  // frames our mp3 decoder tried to decode, but are actually not valid mp3
  unsigned int _invalidFrames = 0;

};

} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXAMEDIAPLAYER_H
