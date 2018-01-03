/**
 * File: textToSpeechProvider_android.cpp
 *
 * Description: Implementation-specific details of text-to-speech conversion
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_ANDROID)

#include "textToSpeechProvider_android.h"
#include "textToSpeechProvider_acapela.h"

#include "cozmoAnim/cozmoAnimContext.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/logging/logging.h"

// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

// Debug sliders
#if REMOTE_CONSOLE_ENABLED

#define CONSOLE_GROUP "TextToSpeech.VoiceParameters"

namespace {
  CONSOLE_VAR_RANGED(s32, kVoiceSpeed, CONSOLE_GROUP, 100, 30, 300);
  CONSOLE_VAR_RANGED(s32, kVoiceShaping, CONSOLE_GROUP, 100, 70, 140);
  CONSOLE_VAR_RANGED(s32, kVoicePitch, CONSOLE_GROUP, 100, 70, 160);
}

#endif

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
  
TextToSpeechProviderImpl::TextToSpeechProviderImpl(const CozmoAnimContext* context, const Json::Value& tts_platform_config)
{
  // Check for valid data platform before we do any work
  using DataPlatform = Anki::Util::Data::DataPlatform;
  using Locale = Anki::Util::Locale;

  const DataPlatform * dataPlatform = context->GetDataPlatform();
  if (nullptr == dataPlatform) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProvider.Initialize.NoDataPlatform", "Missing data platform");
    return;
  }

  // Check for valid locale before we do any work
  const Locale * locale = context->GetLocale();
  if (nullptr == locale) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProvider.Initialize.NoLocale", "Missing locale");
    return;
  }

  // Set up default parameters
  _tts_path = dataPlatform->pathToResource(Anki::Util::Data::Scope::Resources, "tts");
  _tts_voice = "Ryan";
  _tts_speed = 100;
  _tts_shaping = 100;
  _tts_pitch = 100;

  //
  // Allow language configuration to override defaults.  Note pitch is not
  // supported on all platforms, so it is not supported as a config parameter
  // at this time.
  //
  const std::string& language = locale->GetLanguageString();
  Json::Value tts_language_config = tts_platform_config[language.c_str()];

  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kVoiceKey, _tts_voice);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kSpeedKey, _tts_speed);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kShapingKey, _tts_shaping);
  //JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kPitchKey, _tts_pitch);

  LOG_DEBUG("TextToSpeechProvider.Initialize", "language=%s voice=%s speed=%d shaping=%d pitch=%d",
            language.c_str(), _tts_voice.c_str(), _tts_speed, _tts_shaping, _tts_pitch);

  #if REMOTE_CONSOLE_ENABLED
  {
    kVoiceSpeed = _tts_speed;
    kVoiceShaping = _tts_shaping;
    kVoicePitch = _tts_pitch;
  }
  #endif

  LOG_WARNING("TextToSpeechProvider.Initialize.Disabled", "Text-to-speech disabled on this platform");

}

TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
  // Nothing to do here
}

Result TextToSpeechProviderImpl::CreateAudioData(const std::string& text,
                                                 float durationScalar,
                                                 TextToSpeechProviderData& data)
{
  LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "text=%s duration=%f",
            Anki::Util::HidePersonallyIdentifiableInfo(text.c_str()),
            durationScalar);

  LOG_WARNING("TextToSpeechProvider.CreateAudioData.Disabled", "Text-to-speech disabled on this platform");
  return RESULT_FAIL_INVALID_OBJECT;
} // CreateAudioData()

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_ANDROID
