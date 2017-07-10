/**
 * File: textToSpeechProvider.cpp
 *
 * Description: Platform-agnostic interface to platform-specific implementations
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider.h"

#include "util/helpers/ankiDefines.h"

// Which provider implementation do we use for this platform?
#if defined(ANKI_PLATFORM_OSX)
#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_osx.h"
#elif defined(ANKI_PLATFORM_IOS)
#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_ios.h"
#elif defined(ANKI_PLATFORM_ANDROID)
#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_android.h"
#else
#error "No text-to-speech provider implemented for this platform"
#endif

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
      
TextToSpeechProvider::TextToSpeechProvider(const CozmoContext * ctx) :
  _impl(new TextToSpeechProviderImpl(ctx))
{
  // Nothing to do here
}
      
TextToSpeechProvider::~TextToSpeechProvider()
{
  // Nothing to do here
}
      
Result TextToSpeechProvider::CreateAudioData(const std::string& text,
                                             float durationScalar,
                                             TextToSpeechProviderData& data)
{
  // Forward call to implementation
  return _impl->CreateAudioData(text, durationScalar, data);
}
  
} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

