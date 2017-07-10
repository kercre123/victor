/**
 * File: textToSpeechProvider_ios.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string and style.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */


#ifndef __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_ios_H__
#define __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_ios_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_IOS)

#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider.h"
#include <string>

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

//
// TextToSpeechProvider: Abstract interface for text-to-speech processing
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
  // Path to temporary audio file
  std::string _path;
  
}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_IOS

#endif //__Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_ios_H__
