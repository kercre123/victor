/**
 * File: textToSpeechProvider_acapela.cpp
 *
 * Description: Declarations shared by implementations of Acapela TTS SDK
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "anki/cozmo/basestation/textToSpeech/textToSpeechProvider_acapela.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

float AcapelaTTS::GetSpeechRate(float durationScalar)
{
  // Convert Anki unit scale to Acapela percentage scale, then clamp to allowed range
  static const float kSpeechRateMin = 30.0f;
  static const float kSpeechRateMax = 300.0f;
    
  return Anki::Util::Clamp(durationScalar * 100.0f, kSpeechRateMin, kSpeechRateMax);
    
}

  
} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

