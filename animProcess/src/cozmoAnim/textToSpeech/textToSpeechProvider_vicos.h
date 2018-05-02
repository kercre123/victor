/**
 * File: textToSpeechProvider_vicos.h
 *
 * Description: Implementation-specific wrapper to generate audio data from a given string.
 * This class insulates engine and audio code from details of text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2017
 *
 */


#ifndef __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_vicos_H__
#define __Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_vicos_H__

#include "util/helpers/ankiDefines.h"

#if defined(ANKI_PLATFORM_VICOS)

#include "textToSpeechProvider.h"
#include <string>

// Acapela SDK declarations
#include "i_babile.h"

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

//
// TextToSpeechProviderImpl: Platform-specific implementation of text-to-speech provider
//

class TextToSpeechProviderImpl
{
public:
  TextToSpeechProviderImpl(const AnimContext* context, const Json::Value& tts_platform_config);
  ~TextToSpeechProviderImpl();

  Result CreateAudioData(const std::string& text, float durationScalar, TextToSpeechProviderData& data);

private:
  // TTS configuration
  std::string _tts_voice;
  int _tts_speed;
  int _tts_shaping;
  int _tts_pitch;

  //
  // BABILE Object State
  //
  // These structs must stay in memory while SDK is in use.
  // They will be allocated by class constructor and
  // freed by class destructor.
  //
  BB_DbLs * _BAB_LangDba = nullptr;
  BB_MemRec * _BAB_MemRec = nullptr;
  BABILE_MemParam * _BAB_MemParam = nullptr;
  BABILE_Obj * _BAB_Obj = nullptr;

  //
  // BABILE Voice State
  //
  // Stash some parameters describing current voice.
  // These are fetched when voice is loaded, then used
  // when generating speech.
  //
  BB_S32 _BAB_voicefreq = 0;
  BB_S32 _BAB_samplesize = 0;

}; // class TextToSpeechProviderImpl

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // ANKI_PLATFORM_VICOS

#endif //__Anki_cozmo_cozmoAnim_textToSpeech_textToSpeechProvider_vicos_H__
