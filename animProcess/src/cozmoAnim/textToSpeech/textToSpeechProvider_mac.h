/**
 * File: textToSpeechProvider_mac.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string and style.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#ifndef __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_mac_H__
#define __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_mac_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_OSX)

#include "textToSpeechProvider.h"
#include <string>

// Forward declarations (Cozmo)
namespace Anki {
  namespace Cozmo {
    class AnimContext;
  }
}

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

//
// TextToSpeechProviderImpl: Platform-specific implementation of text-to-speech interface
//
// This class is used to isolate engine and audio code from details of a specific text-to-speech implementation.
//
class TextToSpeechProviderImpl
{
public:
  TextToSpeechProviderImpl(const AnimContext* ctx, const Json::Value& tts_platform_config);
  ~TextToSpeechProviderImpl();
  
  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  // Configurable parameters
  std::string _tts_language;
  std::string _tts_voice;
  int _tts_speed;
  int _tts_shaping;
  bool _tts_licensed;
  
  // Opaque handle to Acapela TTS SDK
  void* _lpBabTTS = nullptr;

}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_OSX

#endif //__Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_H__
