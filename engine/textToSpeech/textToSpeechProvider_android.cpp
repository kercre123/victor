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

#include "engine/cozmoContext.h"
#include "engine/textToSpeech/textToSpeechProvider_android.h"
#include "engine/textToSpeech/textToSpeechProvider_acapela.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "util/environment/locale.h"
#include "util/jni/includeJni.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

//
// To extract JNI method signatures:
//   javap -s `find . -name CozmoTextToSpeech.class`
//
// public static int loadVoice(java.lang.String, java.lang.String, int, int, java.lang.String, int, int);
// descriptor: (Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;II)I
//
// public static int createAudioData(java.lang.String, int);
// descriptor: (Ljava/lang/String;I)I
//
constexpr static const char * kClassName = "com/anki/cozmo/CozmoTextToSpeech";
constexpr static const char * kLoadVoiceName = "loadVoice";
constexpr static const char * kLoadVoiceArgs = "(Ljava/lang/String;Ljava/lang/String;IILjava/lang/String;II)I";

constexpr static const char * kCreateAudioName = "createAudioData";
constexpr static const char * kCreateAudioArgs = "(Ljava/lang/String;I)I";

extern "C"
{
  // This pointer is used to pass data between static callback function (below)
  // and C++ caller.  A better solution would be to pass some sort of
  // callback context in to JNI and then back to C++ when the callback is
  // invoked, but the JNI interface is dark and full of terrors.
  //
  // @lee suggests something like a static map and integer ID, where you
  // increment the ID and associate it with a given callback pointer each time
  // you make the call to get speech samples, then clear the entry in the map
  // when the callback is received.
  //
  static Anki::AudioUtil::AudioChunk * sAudioChunkPtr = nullptr;

  //
  // Native C++ callback to be invoked by CozmoTextToSpeech.java
  // Note declaration and signature here must match the declaration in java.
  //
  JNIEXPORT void JNICALL
  Java_com_anki_cozmo_CozmoTextToSpeech_callback(JNIEnv* env, jobject obj, jshortArray jdata, jlong jcount)
  {
    //LOG_DEBUG("TextToSpeechProvider.callback", "received %ld samples", (long) jcount);

    // This pointer is always set before generating audio
    DEV_ASSERT(nullptr != sAudioChunkPtr, "TextToSpeechProvider.callback.InvalidCallbackPtr");

    // Allocate space for incoming data, then fill it java array
    // https://developer.android.com/training/articles/perf-jni.html#region_calls
    const size_t numElements = sAudioChunkPtr->size();
    sAudioChunkPtr->resize(numElements + (size_t) jcount);
    env->GetShortArrayRegion(jdata, 0, (jsize) jcount, &(*sAudioChunkPtr)[numElements]);
  }

}

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
  
TextToSpeechProviderImpl::TextToSpeechProviderImpl(const CozmoContext* context, const Json::Value& tts_platform_config)
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

  // Allow language configuration to override defaults
  const std::string& language = locale->GetLanguageString();
  Json::Value tts_language_config = tts_platform_config[language.c_str()];

  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kVoiceKey, _tts_voice);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kSpeedKey, _tts_speed);
  JsonTools::GetValueOptional(tts_language_config, TextToSpeechProvider::kShapingKey, _tts_shaping);

  LOG_DEBUG("TextToSpeechProvider.Initialize", "language=%s voice=%s speed=%d shaping=%d",
            language.c_str(), _tts_voice.c_str(), _tts_speed, _tts_shaping);

  // Get a handle to the TTS class & methods
  auto envWrapper = Util::JNIUtils::getJNIEnvWrapper();
  auto * env = envWrapper->GetEnv();
  Util::JClassHandle clazz(envWrapper->FindClassInProject(kClassName), env);
  if (nullptr == clazz) {
    LOG_ERROR("TextToSpeechProvider.Initialize.NoClass",
              "Unable to find java class %s",
              kClassName);
    return;
  }

  const jmethodID loadVoiceID = env->GetStaticMethodID(clazz.get(), kLoadVoiceName, kLoadVoiceArgs);
  if (NULL == loadVoiceID) {
    LOG_ERROR("TextToSpeechProvider.Initialize.NoMethod",
              "Unable to get java method %s",
              kLoadVoiceName);
    return;
  }

  // Call JNI method CozmoTextToSpeech.loadVoice(path, voice, userid, password, license, speed, shaping)
  const jstring jpath = env->NewStringUTF(_tts_path.c_str());
  const jstring jvoice = env->NewStringUTF(_tts_voice.c_str());
  const jint juserid = AcapelaTTS::GetUserid();
  const jint jpassword = AcapelaTTS::GetPassword();
  const jstring jlicense = env->NewStringUTF(AcapelaTTS::GetLicense().c_str());

  const jint jresult = env->CallStaticIntMethod(clazz.get(),
                                                loadVoiceID,
                                                jpath,
                                                jvoice,
                                                juserid,
                                                jpassword,
                                                jlicense,
                                                _tts_speed,
                                                _tts_shaping);
  if (0 != jresult) {
    LOG_ERROR("TextToSpeechProvider.Initialize.LoadVoice", "Unable to load voice (error=%d)", jresult);
    return;
  }

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

  // Get a handle to the TTS class & methods
  auto envWrapper = Util::JNIUtils::getJNIEnvWrapper();
  auto * env = envWrapper->GetEnv();
  Util::JClassHandle clazz(envWrapper->FindClassInProject(kClassName), env);
  if (nullptr == clazz) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData.NoClass",
              "Unable to find java class %s",
              kClassName);
    return RESULT_FAIL_INVALID_OBJECT;
  }

  const jmethodID createAudioID = env->GetStaticMethodID(clazz.get(), kCreateAudioName, kCreateAudioArgs);
  if (NULL == createAudioID) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData.NoMethod",
              "Unable to get java method %s",
              kCreateAudioName);
    return RESULT_FAIL_INVALID_OBJECT;
  }

  // Initialize return data
  const int sampleRate = AcapelaTTS::GetSampleRate();
  const int numChannels = AcapelaTTS::GetNumChannels();
  data.Init(sampleRate, numChannels);

  // Marshal arguments for JNI
  const jstring jtext = env->NewStringUTF(text.c_str());
  const jint jspeed = AcapelaTTS::GetSpeechRate(_tts_speed, durationScalar);

  // Set singleton ptr, execute method, then reset singleton ptr
  sAudioChunkPtr = &data.GetChunk();
  const jint jresult = env->CallStaticIntMethod(clazz.get(),
                                                createAudioID,
                                                jtext,
                                                jspeed);
  sAudioChunkPtr = nullptr;

  if (0 != jresult) {
    LOG_ERROR("TextToSpeechProvider.CreateAudioData.NoData",
              "Unable to create audio (error=%d)", jresult);
    return ((Anki::Result)jresult);
  }

  LOG_DEBUG("TextToSpeechProvider.CreateAudioData", "Return %zu samples",
            data.GetNumSamples());

  return RESULT_OK;
  
} // CreateAudioData()

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki



#endif // ANKI_PLATFORM_ANDROID


