/**
 * File: textToSpeechProvider_osx.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string and style.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#ifndef __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_osx_H__
#define __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_osx_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_OSX)

#include "textToSpeechProvider.h"

// Forward declarations (Cozmo)
namespace Anki {
  namespace Cozmo {
    class CozmoContext;
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
  TextToSpeechProviderImpl(const CozmoContext* ctx);
  ~TextToSpeechProviderImpl();
  
  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  // Private members
  void* _lpBabTTS = nullptr;

}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_OSX

#endif //__Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_H__
