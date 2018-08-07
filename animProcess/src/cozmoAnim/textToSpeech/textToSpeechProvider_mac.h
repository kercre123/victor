/**
 * File: textToSpeechProvider_mac.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string and style.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#ifndef __cozmo_textToSpeech_textToSpeechProvider_mac_H__
#define __cozmo_textToSpeech_textToSpeechProvider_mac_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_OSX)

#include "textToSpeechProvider.h"
#include "textToSpeechProviderConfig.h"
#include "json/json.h"

#include <string>

// Forward declarations
namespace Anki {
  namespace Util {
    class RandomGenerator;
  }
  namespace Vector {
    class AnimContext;
  }
}

namespace Anki {
namespace Vector {
namespace TextToSpeech {

//
// TextToSpeechProviderImpl: Platform-specific implementation of text-to-speech interface
//
// This class is used to isolate engine and audio code from details of a specific text-to-speech implementation.
//
class TextToSpeechProviderImpl
{
public:
  TextToSpeechProviderImpl(const Vector::AnimContext* ctx, const Json::Value& tts_platform_config);
  ~TextToSpeechProviderImpl();

  Result SetLocale(const std::string & locale);

  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  // Pointer to RNG provided by context
  Anki::Util::RandomGenerator * _rng = nullptr;

  // Path to TTS resources
  std::string _tts_resource_path;

  // Platform configuration options
  Json::Value _tts_platform_config;

  // Current locale, current language
  std::string _locale;
  std::string _language;

  // Current configuration
  std::unique_ptr<TextToSpeechProviderConfig> _tts_config;

  // License state
  bool _tts_licensed = false;

  // Opaque handle to Acapela TTS SDK
  void* _lpBabTTS = nullptr;


  // Private methods
  Result Initialize(const std::string & locale);
  void Cleanup();

}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Vector
} // end namespace Anki

#endif // ANKI_PLATFORM_OSX

#endif //__victor_textToSpeech_textToSpeechProvider_H__
