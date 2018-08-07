/**
 * File: textToSpeechProvider.cpp
 *
 * Description: Platform-agnostic interface to platform-specific implementations
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "textToSpeechProvider.h"

#include "util/helpers/ankiDefines.h"

// Which provider implementation do we use for this platform?
#if defined(ANKI_PLATFORM_OSX)
#include "textToSpeechProvider_mac.h"
#elif defined(ANKI_PLATFORM_VICOS)
#include "textToSpeechProvider_vicos.h"
#else
#error "No text-to-speech provider implemented for this platform"
#endif

#include "cozmoAnim/animContext.h"
#include "json/json.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "TextToSpeechProvider"

namespace Anki {
namespace Vector {
namespace TextToSpeech {

TextToSpeechProvider::TextToSpeechProvider(const AnimContext * ctx, const Json::Value& tts_config)
{
  // Get configuration struct for this platform
#if defined(ANKI_PLATFORM_OSX)
  Json::Value tts_platform_config = tts_config["osx"];
#elif defined(ANKI_PLATFORM_IOS)
  Json::Value tts_platform_config = tts_config["ios"];
#elif defined(ANKI_PLATFORM_VICOS)
  Json::Value tts_platform_config = tts_config["vicos"];
#endif

  // Instantiate provider for this platform
  _impl = std::make_unique<TextToSpeechProviderImpl>(ctx, tts_platform_config);
}

TextToSpeechProvider::~TextToSpeechProvider()
{
  // Nothing to do here
}

Result TextToSpeechProvider::SetLocale(const std::string & locale)
{
  DEV_ASSERT(_impl != nullptr, "TextToSpeechProvider.SetLocale.InvalidImplementation");
  return _impl->SetLocale(locale);
}

Result TextToSpeechProvider::CreateAudioData(const std::string& text,
                                             float durationScalar,
                                             TextToSpeechProviderData& data)
{
  // Forward call to implementation
  DEV_ASSERT(_impl != nullptr, "TextToSpeechProvider.CreateAudioData.InvalidImplementation");
  return _impl->CreateAudioData(text, durationScalar, data);
}

} // end namespace TextToSpeech
} // end namespace Vector
} // end namespace Anki
