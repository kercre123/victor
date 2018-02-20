/**
 * File: textToSpeechProvider.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string.
 * This class declares an interface common to all platform-specific implementations.
 * This is done to insulate engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */


#ifndef __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_H__
#define __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_H__

#include "coretech/common/shared/types.h"

#include "audioUtil/audioDataTypes.h"

// Forward declarations
namespace Anki {
  namespace Cozmo {
    class AnimContext;
    namespace TextToSpeech {
      class TextToSpeechProviderImpl;
    }
  }
}

namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

//
// TextToSpeechProviderData is used to hold audio returned from TTS provider to engine.
// Audio data is automatically released when the object is destroyed.
//
class TextToSpeechProviderData
{
public:
  using AudioChunk = AudioUtil::AudioChunk;
  using AudioSample = AudioUtil::AudioSample;

  TextToSpeechProviderData() {}
  ~TextToSpeechProviderData() {}

  int GetSampleRate() const { return _sampleRate; }
  int GetNumChannels() const { return _numChannels; }
  size_t GetNumSamples() const { return _chunk.size(); }
  const short * GetSamples() const { return _chunk.data(); }

  const AudioChunk& GetChunk() const { return _chunk; }
  AudioChunk& GetChunk() { return _chunk; }

  void Init(int sampleRate, int numChannels)
  {
    _sampleRate = sampleRate;
    _numChannels = numChannels;
    _chunk.clear();
  }

  void AppendSample(AudioSample sample, size_t numSamples = 1)
  {
    _chunk.insert(_chunk.end(), numSamples, sample);
  }

  void AppendSamples(const AudioSample * samples, size_t numSamples)
  {
    // There's got to be a better way to do this
    const size_t oldSize = _chunk.size();
    _chunk.resize(oldSize + numSamples);
    memcpy(&_chunk[oldSize], samples, numSamples * sizeof(AudioSample));
  }

private:
  int _sampleRate;
  int _numChannels;
  AudioChunk _chunk;
};

//
// TextToSpeechProvider defines a common interface for various platform-specific implementations.
//
class TextToSpeechProvider
{
public:
  // Configuration keywords shared by all providers
  static constexpr const char * kVoiceKey = "voice";
  static constexpr const char * kSpeedKey = "speed";
  static constexpr const char * kShapingKey = "shaping";

  TextToSpeechProvider(const AnimContext* ctx, const Json::Value& tts_config);
  ~TextToSpeechProvider();

  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  // Pointer to platform-specific implementation
  std::unique_ptr<TextToSpeechProviderImpl> _impl;

};


} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif //__Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_H__
