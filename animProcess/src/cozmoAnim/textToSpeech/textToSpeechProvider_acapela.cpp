/**
 * File: textToSpeechProvider_acapela.cpp
 *
 * Description: Declarations shared by implementations of Acapela TTS SDK
 *
 * Copyright: Anki, Inc. 2017
 *
 */

#include "textToSpeechProvider_acapela.h"

#include "util/logging/logging.h"
#include "util/math/math.h"

static const float kDurationScalarMin = 0.05f;
static const float kDurationScalarMax = 20.0f;

static const float kSpeechRateMin = 30.0f;
static const float kSpeechRateMax = 300.0f;

//
// String values are obscured by XORing with a constant.
// XOR is symmetric so doing it twice returns the original value.
//
static inline unsigned char transformer(unsigned char c) {
  return (c ^ 0xAC);
}

static std::string reveal(const std::string& val) {
  std::string s(val);
  std::transform(s.begin(), s.end(), s.begin(), transformer);
  return s;
}

//
// Numeric values are obscured by XORing with a constant.
// XOR is symmetric so doing it twice returns the original value.
//
static int reveal(int val) {
  return (val ^ 0xACADADA);
}

#ifdef notdef

//
// Helper functions used for encoding.  These aren't needed at runtime.
//
static std::string conceal(const std::string& val) {
  return reveal(val);
}

static int conceal(int val) {
  return reveal(val);
}

#endif

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

int AcapelaTTS::GetUserid() {
  // Declared volatile to prevent optimizer from revealing value
  volatile int tts_userid = 0x6ba483b0;   // conceal(0x616e596a)
  return reveal(tts_userid);
}

int AcapelaTTS::GetPassword() {
  // Declared volatile to prevent optimizer from revealing value
  volatile int tts_password = 0x0ac94a41; // conceal(0x0003909b)
  return reveal(tts_password);
}

std::string AcapelaTTS::GetLicense() {

  static const std::string tts_license = // conceal(...)
    "\216\237\233\236\224\214\234\214\306\365\302\315\214\217\357\343"
    "\341\341\351\376\357\345\355\340\217\355\302\307\305\201\377\315"
    "\302\352\336\315\302\317\305\337\317\303\201\371\302\305\330\311"
    "\310\377\330\315\330\311\337\303\312\355\301\311\336\305\317\315"
    "\216\246\370\315\337\333\301\334\370\304\231\333\350\307\373\336"
    "\332\353\236\354\335\317\371\306\353\232\330\325\333\307\307\233"
    "\371\352\315\370\224\370\356\311\300\356\326\334\350\237\342\366"
    "\303\372\231\333\313\355\215\344\374\346\366\317\354\331\324\366"
    "\312\333\313\306\312\371\364\310\326\324\355\364\326\356\354\356"
    "\373\307\345\316\303\336\336\370\217\217\246\376\353\231\310\330"
    "\232\332\340\331\352\326\347\231\315\210\312\347\316\317\315\324"
    "\317\316\371\336\317\233\215\326\371\345\305\344\374\310\336\372"
    "\301\237\302\331\215\310\346\332\325\325\316\246\373\373\353\312"
    "\313\304\230\366\376\225\334\302\335\313\332\225\301\336\300\357"
    "\355\377\217\217\246";

  return reveal(tts_license);
}


} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki
