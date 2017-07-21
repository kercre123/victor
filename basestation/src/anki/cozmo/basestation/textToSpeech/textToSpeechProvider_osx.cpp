/**
 * File: textToSpeechProvider_osx.cpp
 *
 * Description: Implementation-specific details of text-to-speech conversion
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_OSX)

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_acapela.h"
#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_osx.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataScope.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "util/environment/locale.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <cmath>

// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

// Acapela declarations
#include "ioBabTts.h"

// Acapela linkage magic
#define _BABTTSDYN_IMPL_
#include "ifBabTtsDyn.h"
#undef _BABTTSDYN_IMPL_

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {


TextToSpeechProviderImpl::TextToSpeechProviderImpl(const CozmoContext* ctx, const Json::Value& tts_platform_config)
{
  using Locale = Anki::Util::Locale;
  using Scope = Anki::Util::Data::Scope;
  using DataPlatform = Anki::Util::Data::DataPlatform;
  
  // Check for valid data platform before we do any work
  const DataPlatform * dataPlatform = ctx->GetDataPlatform();
  if (nullptr == dataPlatform) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProvider.Initialize.NoDataPlatform", "Unable to initialize TTS provider");
    return;
  }

  // Check for valid locale before we do any work
  const Locale * locale = ctx->GetLocale();
  if (nullptr == locale) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProvider.Initialize.NoLocale", "Unable to initialize TTS provider");
    return;
  }
  
  // Set up default parameters
  _tts_language = locale->GetLanguageString();
  _tts_voice = "Ryan22k_CO";
  _tts_speed = 100;
  _tts_shaping = 100;

  // Allow language configuration to override defaults
  Json::Value tts_language_config = tts_platform_config[_tts_language.c_str()];
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kVoiceKey, _tts_voice);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kSpeedKey, _tts_speed);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kShapingKey, _tts_shaping);
    
  LOG_INFO("TextToSpeechProviderImpl.Initialize",
           "tts_language=%s tts_voice=%s tts_speed=%d tts_shaping=%d",
           _tts_language.c_str(), _tts_voice.c_str(), _tts_speed, _tts_shaping);

  // Initialize Acapela DLL
  std::string resources = dataPlatform->pathToResource(Scope::Resources, "tts");
  HMODULE h = BabTtsInitDllEx(resources.c_str());
  if (nullptr == h) {
    LOG_ERROR("TextToSpeechProvider.Initialize.InitDll", "Unable to initialize TTS provider DLL");
    return;
  }
  
  bool ok = BabTTS_Init();
  if (!ok) {
    LOG_ERROR("TextToSpeechProvider.Initialize.Init", "Unable to initialize TTS provider");
    return;
  }
  
  const long numVoices = BabTTS_GetNumVoices();
  
  LOG_DEBUG("TextToSpeechProvider.Initialize.NumVoices", "TTS provider has %ld voices", numVoices);
  for (int i = 0; i < numVoices; ++i) {
    char voice[50];
    BabTtsError err = BabTTS_EnumVoices(i, voice);
    DEV_ASSERT(err == E_BABTTS_NOERROR, "TextToSpeechProvider.Initialize.EnumVoices");
    LOG_DEBUG("TextToSpeechProvider.Initialize.EnumVoices", "TTS provider has voice %s", voice);
  }
  
  _lpBabTTS = BabTTS_Create();
  if (nullptr == _lpBabTTS) {
    LOG_ERROR("TextToSpeechProvider.Initialize.Create", "Unable to create TTS provider handle");
    return;
  }
  
  BabTtsError err = BabTTS_Open(_lpBabTTS, _tts_voice.c_str(), BABTTS_USEDEFDICT);
  if (E_BABTTS_NOERROR != err) {
    LOG_ERROR("TextToSpeechProvider.Initialize.Open", "Unable to open TTS provider (%s)", BabTTS_GetErrorName(err));
    return;
  }
  
  err = BabTTS_SetSettings(_lpBabTTS, BABTTS_PARAM_SPEED, _tts_speed);
  if (E_BABTTS_NOERROR != err) {
    LOG_ERROR("TextToSpeechProvider.Initialize.SetSpeed", "Unable to set speed=%d (%s)",
              _tts_speed, BabTTS_GetErrorName(err));
  }
  
  err = BabTTS_SetSettings(_lpBabTTS, BABTTS_PARAM_VOCALTRACT, _tts_shaping);
  if (E_BABTTS_NOERROR != err) {
    LOG_ERROR("TextToSpeechProvider.Initialize.SetPitch", "Unable to set shaping=%d (%s)",
              _tts_shaping, BabTTS_GetErrorName(err));
  }

}
  
TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
  if (nullptr != _lpBabTTS) {
    BabTTS_Close(_lpBabTTS);
    BabTTS_Uninit();
    BabTtsUninitDll();
  }
}

Result TextToSpeechProviderImpl::CreateAudioData(const std::string& text,
                                                 float durationScalar,
                                                 TextToSpeechProviderData& data)
{
  if (nullptr == _lpBabTTS) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData", "No provider handle");
    return RESULT_FAIL_INVALID_OBJECT;
  }
  
  // Adjust base speed by duration scalar
  const float speechRate = AcapelaTTS::GetSpeechRate(_tts_speed, durationScalar);
  const int speed = Anki::Util::numeric_cast<int>(std::round(speechRate));
  
  // Update TTS engine to use new speed param
  BabTtsError err = BabTTS_SetSettings(_lpBabTTS, BABTTS_PARAM_SPEED, speed);
  if (E_BABTTS_NOERROR != err) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData.SetSettings", "Unable to set speed (%s)", BabTTS_GetErrorName(err));
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  // Initialize output buffer
  data.Init(AcapelaTTS::GetSampleRate(), AcapelaTTS::GetNumChannels());
  
  // Start processing text
  DWORD dwTextFlags = BABTTS_TEXT | BABTTS_TXT_UTF8 | BABTTS_READ_DEFAULT | BABTTS_TAG_NO;
  err = BabTTS_InsertText(_lpBabTTS, text.c_str(), dwTextFlags);
  if (E_BABTTS_NOERROR != err) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData.InsertText", "Unable to insert text (%s)", BabTTS_GetErrorName(err));
    return RESULT_FAIL;
  }
  
  AudioUtil::AudioChunk& chunk = data.GetChunk();
  
  // Poll output buffer until we run out of data
  while (1) {
    constexpr int NUM_SAMPLES = 16*1024;
    short buf[NUM_SAMPLES] = {0};
    DWORD num_samples = 0;
    err = BabTTS_ReadBuffer(_lpBabTTS, buf, NUM_SAMPLES, &num_samples);
    if (W_BABTTS_NOMOREDATA == err)  {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData.ReadBuffer", "%d new samples, no more data", num_samples);
      for (DWORD i = 0; i < num_samples; ++i) {
        chunk.push_back(buf[i]);
      }
      break;
    } else if (E_BABTTS_NOERROR == err) {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData.ReadBuffer", "%d new samples", num_samples);
      for (DWORD i = 0; i < num_samples; ++i) {
        chunk.push_back(buf[i]);
      }
    } else {
      LOG_ERROR("TextToSpeechProvider.CreateAudioData.ReadBuffer", "Error %d (%s)", err, BabTTS_GetErrorName(err));
      return RESULT_FAIL;
    }
  }
  
  return RESULT_OK;
  
}

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_OSX

