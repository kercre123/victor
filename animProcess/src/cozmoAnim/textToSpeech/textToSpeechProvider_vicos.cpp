/**
 * File: textToSpeechProvider_vicos.cpp
 *
 * Description: Implementation-specific details of text-to-speech conversion
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_VICOS)

#include "textToSpeechProvider_vicos.h"
#include "textToSpeechProvider_acapela.h"

#include "cozmoAnim/animContext.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleMacro.h"
#include "util/environment/locale.h"
#include "util/logging/logging.h"

/* Interpolate Acapela code fragments */
#define USE_LOADINIFILE
#include "ic_babile.h"
#include "dblsman.ch"
#include "error.ch"

// Log options
#define LOG_CHANNEL    "TextToSpeech"

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

TextToSpeechProviderImpl::TextToSpeechProviderImpl(const AnimContext* context, const Json::Value& tts_platform_config)
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

  // Initialize license parameters
  const auto tts_userid = AcapelaTTS::GetUserid();
  const auto tts_passwd = AcapelaTTS::GetPassword();
  const auto tts_license = AcapelaTTS::GetLicense();

  // Initialize configuration
  _tts_config = std::make_unique<TextToSpeechProviderConfig>(locale->GetLanguageString(), tts_platform_config);

  const auto & language = _tts_config->GetLanguage();
  const auto & voice = _tts_config->GetVoice();
  const auto speed = _tts_config->GetSpeed();
  const auto shaping = _tts_config->GetShaping();
  const auto pitch = _tts_config->GetPitch();
  const auto leadingSilence_ms = _tts_config->GetLeadingSilence_ms();
  const auto trailingSilence_ms = _tts_config->GetTrailingSilence_ms();

  LOG_INFO("TextToSpeechProvider.Initialize",
           "language=%s voice=%s speed=%d shaping=%d pitch=%d",
           language.c_str(), voice.c_str(), speed, shaping, pitch);

  //
  // Load voice parameters from ini file
  //
  // VIC-293 TODO: Path to ini file should be determined by data platform & current locale
  //
  const std::string & ttsPath = dataPlatform->GetResourcePath("tts");
  const std::string & iniFile = ttsPath + "/" + voice;
  const std::string & loadParams = "*=RAM";
  const char * defaultText = NULL;
  BB_S32 synthAvail = 0;
  short synthModule = 0;
  short nlpModule = 0;
  BB_DbLs * nlpeLS = NULL;
  BB_DbLs * synthLS = NULL;
  BB_ERROR bbError;

  _BAB_LangDba = BABILE_loadIniFile(iniFile.c_str(), &nlpeLS, &synthLS, &nlpModule, &synthAvail, &synthModule,
         &defaultText, loadParams.c_str());
  if (nullptr == _BAB_LangDba)	{
    LOG_WARNING("TextToSpeechProvider.Initialize.LoadIniFile", "Failed to load ini file %s", iniFile.c_str());
    return ;
  }

  LOG_DEBUG("TextToSpeechProvider.Initialize.LoadIniFile",
           "nlpeLS=%p synthLS=%p nlpModule=%d synthAvail=%ld synthModule=%d",
           nlpeLS, synthLS, nlpModule, synthAvail, synthModule);

  //
  // Ask Babile SDK how many memory segments it needs to track, then allocate a tracker of appropriate size
  //
  const BB_S16 numAlloc = BABILE_numAlloc();
  if (numAlloc > 0) {
    _BAB_MemRec = (BB_MemRec *) calloc(numAlloc, sizeof(BB_MemRec));
  }

  // Populate init struct
  _BAB_MemParam = (BABILE_MemParam *) calloc(1, sizeof(BABILE_MemParam));
  _BAB_MemParam->sSize = sizeof(BABILE_MemParam); /* Sanity+version check*/
  _BAB_MemParam->license = (const BB_TCHAR *) tts_license.c_str();
  _BAB_MemParam->uid.passwd = tts_passwd;
  _BAB_MemParam->uid.userId = tts_userid;
  _BAB_MemParam->nlpeLS = nlpeLS;
  _BAB_MemParam->nlpModule = nlpModule;
  _BAB_MemParam->synthLS = synthLS;
  _BAB_MemParam->synthModule = synthModule;

  // Ask Babile how much memory is needed for each segment
  BABILE_alloc(_BAB_MemParam, _BAB_MemRec);

  // Allocate space for each segment
  for (BB_S16 i = 0; i < numAlloc; ++i) {
    if (_BAB_MemRec[i].size > 0 && _BAB_MemRec[i].space != BB_IALG_NONE) {
       _BAB_MemRec[i].base = malloc(_BAB_MemRec[i].size);
    }
  }

  _BAB_Obj = BABILE_init(_BAB_MemRec, _BAB_MemParam);
  if (nullptr == _BAB_Obj) {
    LOG_WARNING("TextToSpeechProvider.Initialize.Init", "Failed to initialize TTS library");
     /* D) those variables will contain initialization errors
		initializationParameters.initError;
		initializationParameters.selInitError
		initializationParameters.nlpInitError
                initializationParameters.mbrInitError
    */
    return;
  }

  {
    char version[512] = "";

    BABILE_getVersionEx(_BAB_Obj, (BB_TCHAR *) version, sizeof(version)/sizeof(char));
    BABILE_getSetting(_BAB_Obj, BABIL_PARM_VOICEFREQ, &_BAB_voicefreq);
    BABILE_getSetting(_BAB_Obj, BABIL_PARM_SAMPLESIZE, &_BAB_samplesize);
    LOG_INFO("TextToSpeechProvider.Initialize.VersionEx", "TTS library version %s (%s) freq=%ld samplesize=%ld",
    	BABILE_getVersion(), version, _BAB_voicefreq, _BAB_samplesize);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SPEED, speed);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.Initialize.SetSpeed", "Unable to set speed %d (error %ld)",
                speed, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SEL_VOICESHAPE, shaping);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.Initialize.SetVoiceShape", "Unable to set voice shape %d (error %ld)",
                shaping, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_PITCH, pitch);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.Initialize.SetPitch", "Unable to set pitch %d (error %ld)",
                pitch, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_LEADINGSILENCE, leadingSilence_ms);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.Initialize.SetLeadingSilence", "Unable to set leading silence %d (error %ld)",
               leadingSilence_ms, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_TRAILINGSILENCE, trailingSilence_ms);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.Initialize.SetTrailingSilence", "Unable to set trailing silence %d (error %ld)",
                trailingSilence_ms, bbError);
  }

  _rng = context->GetRandom();

}

TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
   // Free memory allocated by BABILE_init
   if (nullptr != _BAB_Obj) {
     BABILE_free(_BAB_Obj, _BAB_MemRec);
     _BAB_Obj = nullptr;
     _BAB_MemRec = nullptr;
   }

   // Free memory allocated by BABILE_LoadIniFile
   if (nullptr != _BAB_LangDba) {
      destroyLanguageDba(_BAB_LangDba);
      _BAB_LangDba = nullptr;
   }

   // Free memory allocated for memory tracker
   if (nullptr != _BAB_MemParam) {
     free(_BAB_MemParam);
     _BAB_MemParam = nullptr;
   }
}

Result TextToSpeechProviderImpl::CreateAudioData(const std::string& text,
                                                 float durationScalar,
                                                 TextToSpeechProviderData& data)
{
  LOG_INFO("TextToSpeechProvider.CreateAudioData", "text=%s duration=%f",
            Anki::Util::HidePersonallyIdentifiableInfo(text.c_str()),
            durationScalar);
  if (nullptr == _BAB_Obj) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData", "TTS SDK not initialized");
    return RESULT_FAIL_INVALID_OBJECT;
  }

  // Get base speed for this utterance, then adjust by duration scalar
  const auto baseSpeed = _tts_config->GetSpeed(_rng, text.size());
  const auto adjustedSpeed = AcapelaTTS::GetSpeechRate(baseSpeed, durationScalar);
  const auto speed = Anki::Util::numeric_cast<int>(std::round(adjustedSpeed));
  const auto shaping = _tts_config->GetShaping();
  const auto pitch = _tts_config->GetPitch();

  // Reset TTS processing state & error state
  BB_ERROR bbError = BABILE_reset(_BAB_Obj);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.CreateAudioData", "Unable to reset TTS (error %ld)", bbError);
  }

  BABILE_resetError(_BAB_Obj);

  // Apply current TTS params
  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SPEED, speed);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.CreateAudioData", "Unable to set speed %d (error %ld)", speed, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SEL_VOICESHAPE, shaping);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.CreateAudioData", "Unable to set shaping %d (error %ld)", shaping, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_PITCH, pitch);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.CreateAudioData", "Unable to set pitch %d (error %ld)", pitch, bbError);
  }

  data.Init(_BAB_voicefreq, 1);

  BB_TCHAR * str = (BB_TCHAR *) text.c_str();
  BB_S32 strpos = 0;
  BB_U32 totSamples = 0;
  BB_S16 samples[2048];

  while (true)
  {
    BB_U32 numSamples = 0;
    BB_S32 charRead = BABILE_readText(_BAB_Obj, &str[strpos], samples, (BB_U32)(sizeof(samples)/_BAB_samplesize), &numSamples);
    LOG_INFO("TextToSpeechProvider.CreateAudioData", "charRead=%ld numSamples=%lu", charRead, numSamples);
    if (charRead < 0) {
      LOG_ERROR("TextToSpeechProvider.CreateAudioData", "charRead=%ld", charRead);
      testError(_BAB_Obj, _BAB_MemParam, stderr);
      break;
    }
    if (charRead == 0 && numSamples == 0) {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Done");
      break;
    }
    // Advance string position
    if (charRead > 0) {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Advance by %ld characters", charRead);
      strpos += charRead;
    }
    // Add samples to result
    if (numSamples > 0) {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Add %lu samples", numSamples);
      data.AppendSamples(samples, numSamples);
      totSamples += numSamples;
    }

  }

  while (true)
  {
    // Flush internal buffers
    BB_U32 numSamples = 0;
    BABILE_readText(_BAB_Obj, NULL, samples, (BB_U32)(sizeof(samples)/_BAB_samplesize), &numSamples);
    if (numSamples <= 0) {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Done");
      break;
    }
    // Add samples to result
    if (numSamples > 0)
    {
      LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Flush %lu samples", numSamples);
      data.AppendSamples(samples, numSamples);
      totSamples += numSamples;
    }
  }

  LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Return %lu samples", totSamples);

  return RESULT_OK;
} // CreateAudioData()

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_VICOS
