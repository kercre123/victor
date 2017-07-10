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

#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_android.h"

#include "util/logging/logging.h"

// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

// Flite declarations
extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include "flite.h"
#pragma GCC diagnostic pop
  cst_voice* register_cmu_us_rms(const char* voxdir);
  void unregister_cmu_us_rms(cst_voice* vox);
}

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
  
TextToSpeechProviderImpl::TextToSpeechProviderImpl(const CozmoContext* context)
{
  flite_init();
  _voice = register_cmu_us_rms(nullptr);
  if (nullptr == _voice) {
    LOG_ERROR("TextToSpeechProviderImpl.RegisterVoice", "Unable to register voice");
  }
}
  
TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
  // There is no tear-down for flite =(
  unregister_cmu_us_rms(_voice);
}

Result TextToSpeechProviderImpl::CreateAudioData(const std::string& text,
                                                 float durationScalar,
                                                 TextToSpeechProviderData& data)
{
  // Add Duration Stretch feature to voice
  // If durationScalar is too small, the flite code crashes, so we clamp it here
  feat_set_float(_voice->features, "duration_stretch", std::max(durationScalar, 0.05f));
  
  // Generate wave data
  cst_wave* wave = flite_text_to_wave(text.c_str(), _voice);
  if (nullptr == wave) {
    LOG_ERROR("TextToSpeechProviderImpl.CreateAudioData", "Error converting text to wave");
    return RESULT_FAIL;
  }
  
  // Initialize return data
  data.Init(wave->num_samples, wave->num_channels);
  
  AudioUtil::AudioChunk& chunk = data.GetChunk();
  
  for (size_t n = 0; n < wave->num_samples; ++n) {
    chunk.push_back(wave->samples[n]);
  }
  
  delete_wave(wave);
  
  return RESULT_OK;
  
} // CreateAudioData()

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_ANDROID


