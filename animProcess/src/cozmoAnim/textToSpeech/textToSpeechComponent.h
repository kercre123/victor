/**
* File: textToSpeechComponent.h
*
* Author: Molly Jameson
* Created: 03/21/16
*
* Overhaul: Andrew Stein / Jordan Rivas, 08/18/16
*
* Description: Component wrapper to generate, cache and use wave data from a given string and style.
*
* Copyright: Anki, Inc. 2016
*
*/

#ifndef __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechComponent_H__
#define __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechComponent_H__

#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "audioEngine/audioTypes.h"
#include "coretech/common/shared/types.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/types/sayTextStyles.h"
#include "clad/types/textToSpeechTypes.h"
#include "util/helpers/templateHelpers.h"
#include <deque>
#include <mutex>
#include <unordered_map>

// Forward declarations
namespace Anki {
  namespace Cozmo {
    class AnimContext;
    namespace Audio {
      class CozmoAudioController;
    }
    namespace RobotInterface {
      struct TextToSpeechPrepare;
      struct TextToSpeechDeliver;
      struct TextToSpeechPlay;
      struct TextToSpeechCancel;
    }
    namespace TextToSpeech {
      class TextToSpeechProvider;
    }
  }
  namespace Util {
    namespace Dispatch {
      class Queue;
    }
  }
}

namespace Anki {
namespace Cozmo {

class TextToSpeechComponent
{
public:

  TextToSpeechComponent(const AnimContext* context);
  ~TextToSpeechComponent();

  //
  // CLAD message handlers are called on the main thread to handle incoming requests.
  //
  void HandleMessage(const RobotInterface::TextToSpeechPrepare& msg);
  void HandleMessage(const RobotInterface::TextToSpeechDeliver& msg);
  void HandleMessage(const RobotInterface::TextToSpeechPlay& msg);
  void HandleMessage(const RobotInterface::TextToSpeechCancel& msg);

  //
  // Update method is called once per tick on main thread. This method responds to events
  // posted by worker thread and performs tasks in synchronization with the rest of the animation engine.
  //
  void Update();

private:
  // -------------------------------------------------------------------------------------------------------------------
  // Private types
  // -------------------------------------------------------------------------------------------------------------------
  using AudioController = Anki::Cozmo::Audio::CozmoAudioController;
  using TextToSpeechProvider = Anki::Cozmo::TextToSpeech::TextToSpeechProvider;
  using DispatchQueue = Anki::Util::Dispatch::Queue;
  using TTSID_t = uint8_t;
  using EventPair = std::pair<TTSID_t, TextToSpeechState>;
  using EventQueue = std::deque<EventPair>;

  // TTS creation state
  enum class AudioCreationState {
    None,       // Does NOT exist
    Preparing,  // In process of creating data
    Ready       // Data is ready to use
  };

  // TTS data bundle
  struct TtsBundle
  {
    // TTS request context
    AudioCreationState state = AudioCreationState::None;
    SayTextVoiceStyle style = SayTextVoiceStyle::Count;
    float pitchScalar = 0.f;
    AudioEngine::StandardWaveDataContainer* waveData = nullptr;

    ~TtsBundle() { Util::SafeDelete(waveData); }
  };

  // -------------------------------------------------------------------------------------------------------------------
  // Private members
  // -------------------------------------------------------------------------------------------------------------------

  static constexpr TTSID_t kInvalidTTSID = 0;

  // Internal mutex
  mutable std::mutex _lock;

  // Map of data bundles
  std::unordered_map<TTSID_t, TtsBundle> _ttsWaveDataMap;

  // Map of TTSID's to corresponding AudioEventId's for delayed playback
  std::unordered_map<TTSID_t, AudioEngine::AudioEventId> _ttsIDtoAudioEventIdMap;

  // Audio controller provided by context
  AudioController * _audioController = nullptr;

  // Worker thread
  DispatchQueue * _dispatchQueue = nullptr;

  // Platform-specific provider
  std::unique_ptr<TextToSpeechProvider> _pvdr;

  // Thread-safe event queue
  EventQueue _evtq;
  std::mutex _evtq_mutex;

  // -------------------------------------------------------------------------------------------------------------------
  // Private methods
  // -------------------------------------------------------------------------------------------------------------------

  // Thread-safe event notifications
  void PushEvent(const EventPair& evt);
  bool PopEvent(EventPair& evt);

  // Use Text to Speech lib to create audio data & reformat into StandardWaveData format
  // Return nullptr if Text to Speech lib fails to create audio data
  AudioEngine::StandardWaveDataContainer* CreateAudioData(const std::string& text,
                                                          SayTextVoiceStyle style,
                                                          float durationScalar);

  // Find TtsBundle for operation
  const TtsBundle* GetTtsBundle(const TTSID_t ttsID) const;

  TtsBundle* GetTtsBundle(const TTSID_t ttsID);

  // Asynchronous create the wave data for the given text and style, to be played later
  // Use GetOperationState() to check if wave data is Ready
  // Return RESULT_OK on success
  Result CreateSpeech(const TTSID_t ttsID,
                      const std::string& text,
                      const SayTextVoiceStyle style,
                      const float durationScalar,
                      const float pitchScalar);

  // Get the current state of the create speech operation
  AudioCreationState GetOperationState(const TTSID_t ttsID) const;

  // Set up Audio Engine to play text's audio data
  // out_duration_ms provides approximate duration of event before processing in audio engine
  // out_eventId provides audio event that can be used to trigger playback
  // Return false if the audio has NOT been created or is not yet ready. Output parameters will NOT be valid.
  bool PrepareAudioEngine(const TTSID_t ttsID, float& out_duration_ms, AudioEngine::AudioEventId& out_eventId);

  // Clear speech audio data from audio engine and clear operation data
  void CleanupAudioEngine(const TTSID_t ttsID);

  // Clear speech operation audio data from memory
  void ClearOperationData(const TTSID_t ttsID);

  // Clear ALL loaded text audio data from memory
  void ClearAllLoadedAudioData();

  //
  // State transition helpers
  // These methods are called by Update() on the main thread, in response to events posted by the worker thread.
  //
  void OnStateInvalid(const TTSID_t ttsID);
  void OnStatePreparing(const TTSID_t ttsID);
  void OnStatePrepared(const TTSID_t ttsID);

  // Audio helpers
  void SetAudioProcessingStyle(SayTextVoiceStyle style);
  void SetAudioProcessingPitch(float pitchScalar);
  void PostAudioEvent(AudioEngine::AudioEventId eventId, uint8_t ttsID);

  // AudioEngine Callbacks
  void OnUtteranceCompleted(uint8_t ttsID) const;

}; // class TextToSpeechComponent


} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechComponent_H__
