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
namespace Vector {
namespace TextToSpeech {

TextToSpeechProviderImpl::TextToSpeechProviderImpl(const AnimContext* context, const Json::Value& tts_platform_config)
{
  // Caller must provide context & RNG
  DEV_ASSERT(context != nullptr, "TextToSpeechProviderImpl.InvalidContext");
  DEV_ASSERT(context->GetRandom() != nullptr, "TextToSpeechProviderImpl.InvalidRNG");

  // Check for valid data platform before we do any work
  const auto * dataPlatform = context->GetDataPlatform();
  if (nullptr == dataPlatform) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProviderImpl.InvalidDataPlatform", "Missing data platform");
    return;
  }

  // Check for valid locale before we do any work
  const auto * locale = context->GetLocale();
  if (nullptr == locale) {
    // This may happen during unit tests
    LOG_WARNING("TextToSpeechProviderImpl.InvalidLocale", "Missing locale");
    return;
  }

  // Hold on to anything we will need later
  _tts_resource_path = dataPlatform->GetResourcePath("tts");
  _tts_platform_config = tts_platform_config;
  _rng = context->GetRandom();

  // Initialize with default locale
  const auto & localeString = locale->GetLocaleString();

  const Result result = Initialize(localeString);
  if (result != RESULT_OK) {
    LOG_ERROR("TextToSpeechProviderImpl.InitFailed",
      "Unable to initialize with locale %s (error %d)",
      localeString.c_str(), result);
    return;
  }

}

TextToSpeechProviderImpl::~TextToSpeechProviderImpl()
{
  Cleanup();
}

void TextToSpeechProviderImpl::Cleanup()
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

  // Reset current locale, current language
  _locale.clear();
  _language.clear();
}

Result TextToSpeechProviderImpl::Initialize(const std::string & locale)
{
  LOG_DEBUG("TextToSpeechProvider.Initialize", "Initialize locale %s", locale.c_str());

  if (locale == _locale) {
    LOG_DEBUG("TextToSpeechProvider.Initialize", "Already using locale %s", locale.c_str());
    return RESULT_OK;
  }

  Cleanup();

  std::string language = Anki::Util::Locale::LocaleFromString(locale).GetLanguageString();
  if (language.empty()) {
    LOG_ERROR("TextToSpeechProvider.Initialize", "Unable to get language from locale %s", locale.c_str());
    language = "en";
  }

  // Initialize configuration
  _tts_config = std::make_unique<TextToSpeechProviderConfig>(language, _tts_platform_config);

  // Initialize license parameters
  const auto tts_userid = AcapelaTTS::GetUserid();
  const auto tts_passwd = AcapelaTTS::GetPassword();
  const auto tts_license = AcapelaTTS::GetLicense();

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
  const std::string & iniFile = _tts_resource_path + "/" + voice;
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
    return RESULT_FAIL_INVALID_PARAMETER;
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
    return RESULT_FAIL_INVALID_OBJECT;
  }

  {
    char version[512] = "";

    BABILE_getVersionEx(_BAB_Obj, (BB_TCHAR *) version, sizeof(version)/sizeof(char));
    BABILE_getSetting(_BAB_Obj, BABIL_PARM_VOICEFREQ, (BB_SPTR*) &_BAB_voicefreq);
    BABILE_getSetting(_BAB_Obj, BABIL_PARM_SAMPLESIZE, (BB_SPTR*) &_BAB_samplesize);
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

  _locale = locale;
  _language = language;

  LOG_INFO("TextToSpeechProviderImpl.Initialize", "Initialized locale %s language %s", _locale.c_str(), _language.c_str());

  return RESULT_OK;
}

Result TextToSpeechProviderImpl::SetLocale(const std::string & locale)
{
  return Initialize(locale);
}


Result TextToSpeechProviderImpl::GetFirstAudioData(const std::string & text,
                                                   float durationScalar,
                                                   TextToSpeechProviderData & data,
                                                   bool & done)
{
  LOG_INFO("TextToSpeechProvider.GetFirstAudioData", "text=%s duration=%f",
            Anki::Util::HidePersonallyIdentifiableInfo(text.c_str()),
            durationScalar);
  if (nullptr == _BAB_Obj) {
    LOG_ERROR("TextToSpeechProvider.GetFirstAudioData", "TTS SDK not initialized");
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
    LOG_WARNING("TextToSpeechProvider.GetFirstAudioData", "Unable to reset TTS (error %ld)", bbError);
  }

  BABILE_resetError(_BAB_Obj);

  // Apply current TTS params
  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SPEED, speed);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.GetFirstAudioData", "Unable to set speed %d (error %ld)", speed, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_SEL_VOICESHAPE, shaping);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.GetFirstAudioData", "Unable to set shaping %d (error %ld)", shaping, bbError);
  }

  bbError = BABILE_setSetting(_BAB_Obj, BABIL_PARM_PITCH, pitch);
  if (BB_OK != bbError) {
    LOG_WARNING("TextToSpeechProvider.GetFirstAudioData", "Unable to set pitch %d (error %ld)", pitch, bbError);
  }

  _str = text;
  _strlen = text.size();
  _strpos = 0;

  return GetNextAudioData(data, done);
}

Result TextToSpeechProviderImpl::GetNextAudioData(TextToSpeechProviderData & data, bool & done)
{
  // If we are still processing text, return pointer to next text, else null
  BB_TCHAR * str = (BB_TCHAR *) ((_strpos < _strlen) ? &_str[_strpos] : nullptr);

  BB_S16 samples[2048];
  BB_U32 numWanted = (sizeof(samples)/_BAB_samplesize);
  BB_U32 numSamples = 0;

  const BB_S32 charRead = BABILE_readText(_BAB_Obj, str, samples, numWanted, &numSamples);

  LOG_DEBUG("TextToSpeechProvider.GetNextAudioData", "charRead=%ld numSamples=%lu", charRead, numSamples);

  if (charRead < 0) {
    LOG_ERROR("TextToSpeechProvider.GetNextAudioData", "charRead=%ld", charRead);
    testError(_BAB_Obj, _BAB_MemParam, stderr);
    return RESULT_FAIL;
  }

  if (charRead == 0 && numSamples == 0) {
    LOG_DEBUG("TextToSpeechProvider.GetNextAudioData", "Done");
    done = true;
    return RESULT_OK;
  }

  if (charRead > 0) {
    // Advance string position
    LOG_DEBUG("TextToSpeechProvider.GetNextAudioData", "Advance by %ld characters", charRead);
    _strpos += charRead;
  }

  if (numSamples > 0) {
    // Add samples to result
    LOG_DEBUG("TextToSpeechProvider.GetNextAudioData", "Add %lu samples", numSamples);
    data.Init(_BAB_voicefreq, 1);
    data.AppendSamples(samples, numSamples);
  }

  return RESULT_OK;

} // GetNextAudioData()

} // end namespace TextToSpeech
} // end namespace Vector
} // end namespace Anki

#endif // ANKI_PLATFORM_VICOS
