/**
 * File: textToSpeechProvider.cpp
 *
 * Description: Platform-agnostic interface to platform-specific implementations
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "engine/textToSpeech/textToSpeechProvider.h"

#include "util/helpers/ankiDefines.h"

// Which provider implementation do we use for this platform?
#if defined(ANKI_PLATFORM_OSX)
#include "engine/textToSpeech/textToSpeechProvider_osx.h"
#elif defined(ANKI_PLATFORM_IOS)
#include "engine/textToSpeech/textToSpeechProvider_ios.h"
#elif defined(ANKI_PLATFORM_ANDROID)
#include "engine/textToSpeech/textToSpeechProvider_android.h"
#else
#error "No text-to-speech provider implemented for this platform"
#endif

#include "engine/cozmoContext.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
      
TextToSpeechProvider::TextToSpeechProvider(const CozmoContext * ctx, const Json::Value& tts_config)
{
  // Get configuration struct for this platform
#if defined(ANKI_PLATFORM_OSX)
  Json::Value tts_platform_config = tts_config["osx"];
#elif defined(ANKI_PLATFORM_IOS)
  Json::Value tts_platform_config = tts_config["ios"];
#elif defined(ANKI_PLATFORM_ANDROID)
  Json::Value tts_platform_config = tts_config["android"];
#endif
  
  // Instantiate provider for this platform
  _impl.reset(new TextToSpeechProviderImpl(ctx, tts_platform_config));
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

