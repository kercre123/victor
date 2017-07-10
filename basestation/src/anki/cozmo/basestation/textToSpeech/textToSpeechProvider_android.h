/**
 * File: textToSpeechProvider_android.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */


#ifndef __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_android_H__
#define __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_android_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_ANDROID)

#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider.h"

// Forward declarations (flite)
struct cst_voice_struct;

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

//
// TextToSpeechProviderImpl: Platform-specific implementation of text-to-speech provider
//

class TextToSpeechProviderImpl
{
public:
  TextToSpeechProviderImpl(const CozmoContext* context);
  ~TextToSpeechProviderImpl();
  
  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  struct cst_voice_struct* _voice = nullptr;
  
}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_ANDROID

#endif //__Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_android_H__
