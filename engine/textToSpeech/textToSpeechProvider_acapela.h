/**
 * File: textToSpeechProvider_acapela.h
 *
 * Description: Declarations shared by various implementations of Acapela TTS SDK
 *
 * Copyright: Anki, Inc. 2017
 *
 */


#ifndef __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_acapela_H__
#define __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_acapela_H__

#include "util/global/globalDefinitions.h"
#include "util/helpers/ankiDefines.h"

#include <string>

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

// Default parameters
#if defined(ANKI_PLATFORM_OSX)
#define TTS_DEFAULT_VOICE "Ryan22k_CO"
#elif defined(ANKI_PLATFORM_IOS)
#define TTS_DEFAULT_VOICE "enu_ryan_22k_co.fl"
#elif defined(ANKI_PLATFORM_ANDROID)
#define TTS_DEFAULT_VOICE "Ryan"
#endif

#define TTS_DEFAULT_SPEED      100
#define TTS_DEFAULT_SHAPING    100
#define TTS_DEFAULT_PITCH      100
#define TTS_DEFAULT_COZMO      "Cozmo" // No translation
#define TTS_LEADINGSILENCE_MS  50      // Minimum allowed by Acapela TTS SDK
#define TTS_TRAILINGSILENCE_MS 50      // Minimum allowed by Acapela TTS SDK
  
class AcapelaTTS
{
public:
  // Acapela Colibri voices are sampled at 22050hz
  static int GetSampleRate() { return 22050; }
  
  // Acapela Colibri voices are monophonic
  static int GetNumChannels() { return 1; }
  
  // Adjust base speed by unit scalar, then clamp to supported range
  static float GetSpeechRate(int speed, float durationScalar);

  // License info
  static int GetUserid();
  static int GetPassword();
  static std::string GetLicense();

};
  
} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_acapela_H__

