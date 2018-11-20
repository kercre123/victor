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


#include <memory>
#include <sstream>
#include <unordered_set>
#include <atomic>
#include <map>
#include <mutex>
#include <queue>


#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>

// TODO: some of these could be fwd declared
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <PlaylistParser/UrlContentToAttachmentConverter.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTools/streamingWaveDataInstance.h"

// TEMP
struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;


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
class AnimContext;
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

  void Init( const AnimContext* context );

  void Update();

  virtual SourceId setSource( std::shared_ptr< alexaClientSDK::avsCommon::avs::attachment::AttachmentReader > attachmentReader,
                              const alexaClientSDK::avsCommon::utils::AudioFormat *format=nullptr ) override;
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
  virtual void onError() override;
  virtual void doShutdown() override;

private:

  Type _type;
  alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings _settings;

  using StreamingWaveDataPtr = std::shared_ptr<AudioEngine::StreamingWaveDataInstance>;
  using AudioController = Audio::CozmoAudioController;
  using DispatchQueue = Util::Dispatch::Queue;
  
  // Set audio variables for Media Player based off it's _name
  void SetMediaPlayerAudioEvents();
  
  // Set the volume in Audio Controller for Media Player
  // NOTE: Acceptable volume value [0.0, 1.0]
  void SetPlayerVolume( float volume );

  // decodes from _mp3Buffer into data, returns millisec decoded
  int Decode( const StreamingWaveDataPtr& data, bool flush = false );

  void CallOnPlaybackFinished( SourceId id );

  const char* const StateToString() const;

  void SavePCM( short* buff, size_t size=0 ) const;
  void SaveMP3( const unsigned char* buff, size_t size=0 ) const;

  enum class State {
    Idle=0,
    Preparing,
    Playable,
    Playing,
    Paused
  };
  std::atomic<State> _state;

  void SetState( State state );

  static SourceId _sourceID;
  SourceId _playingSource = 0;

  std::map<SourceId, std::unique_ptr< AlexaReader >> _readers;
  short _decodedPcm[1152*2]; // Got value from minimp3.h MINIMP3_MAX_SAMPLES_PER_FRAME

  // An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
  alexaClientSDK::avsCommon::utils::threading::Executor _executor;

  /// The set of MediaPlayerObserverInterface instances to notify of state changes
  std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface>> _observers;

  std::unique_ptr<Util::RingBuffContiguousRead<uint8_t>> _mp3Buffer;

  uint64_t _offset_ms = 0;
  bool _firstPass = true;

  StreamingWaveDataPtr _waveData;

  std::string _saveFolder;

  // worker thread
  DispatchQueue* _dispatchQueue = nullptr;

  // audio controller provided by context
  AudioController* _audioController = nullptr;
  
  // Audio play control events
  AudioMetaData::GameEvent::GenericEvent      _playEvent;
  AudioMetaData::GameEvent::GenericEvent      _pauseEvent;
  AudioMetaData::GameEvent::GenericEvent      _resumeEvent;
  AudioMetaData::GameParameter::ParameterType _volumeParameter;
  uint8_t                                     _pluginId;

  /// Used to create objects that can fetch remote HTTP content.
  std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> _contentFetcherFactory;

  /// Used to stream urls into attachments
  std::shared_ptr<alexaClientSDK::playlistParser::UrlContentToAttachmentConverter> _urlConverter;

  const AudioInfo& _audioInfo;

  // TEMP
  SpeechRecognizerTHF*            _recognizer = nullptr;
  SpeexResamplerState*            _speexState = nullptr;
  SpeechRecognizerSystem*         _speechRegSys = nullptr;
  static constexpr size_t         _kResampleMaxSize = 2000;
  short                           _resampledPcm[_kResampleMaxSize];
  std::queue<std::pair<int, int>> _detectedTriggers_ms;
  
  void UpdateDetectorState(float& inout_lastPlayedMs);
  
};

} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXAMEDIAPLAYER_H
