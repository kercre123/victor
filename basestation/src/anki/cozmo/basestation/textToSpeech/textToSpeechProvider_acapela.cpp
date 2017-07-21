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

static const float kDurationScalarMin = 0.05f;
static const float kDurationScalarMax = 20.0f;

static const float kSpeechRateMin = 30.0f;
static const float kSpeechRateMax = 300.0f;


namespace Anki {
namespace Cozmo {
namespace TextToSpeech {

float AcapelaTTS::GetSpeechRate(int speed, float durationScalar)
{
  // Clamp duration scalar to something reasonable before we divide
  durationScalar = Anki::Util::Clamp(durationScalar, kDurationScalarMin, kDurationScalarMax);
  
  // Adjust base rate (100%) by duration scalar
  const float speechRate = speed / durationScalar;
  
  // Clamp adjusted rate to allowable range
  return Anki::Util::Clamp(speechRate, kSpeechRateMin, kSpeechRateMax);
    
}
  
} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

