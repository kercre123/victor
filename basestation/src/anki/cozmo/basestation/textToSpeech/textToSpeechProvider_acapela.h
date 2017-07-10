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

namespace Anki {
namespace Cozmo {
namespace TextToSpeech {
  
class AcapelaTTS
{
public:
  // Acapela Colibri voices are sampled at 22050hz
  static int GetSampleRate() { return 22050; }
  
  // Acapela Colibri voices are monophonic
  static int GetNumChannels() { return 1; }
  
  // Convert Anki unit scale (default 1) to Acapela percentage scale (default 100),
  // then clamp to supported range (30-300)
  static float GetSpeechRate(float durationScalar);
};
  
} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki

#endif // __Anki_cozmo_basestation_textToSpeech_textToSpeechProvider_acapela_H__

