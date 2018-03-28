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



//
// Helper functions used for encoding.  These aren't needed at runtime.
//
#ifdef notdef
static std::string conceal(const std::string& val) {
  return reveal(val);
}
#endif

#ifdef notdef
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

//
// Returns license string from Acapela. See also
//   EXTERNALS/anki-thirdparty/acapela/AcapelaTTS_for_LinuxEmbedded_V8.511/license/babile/babjYna.h
//
// Note that license string includes embedded quote and newline characters which must be preserved
// as part of the string.
//

std::string AcapelaTTS::GetLicense() {

#ifdef notdef
  // Enable this stuff to test encoding of a new license string
  std::string plain = "...";
  std::string line;
  for (size_t i = 0; i < plain.size(); ++i) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "\\%o", transformer(plain[i]));
    line += buf;
    if (i % 16 == 15) {
      LOG_INFO("AcapelaTTS.GetLicense.Concealed", "%s", line.c_str());
      line.clear();
    }
  }
  LOG_INFO("AcapelaTTS.GetLicense.Concealed", "%s", line.c_str());
#endif

  static const std::string concealed =
    "\216\233\231\232\230\214\234\214\306\365\302\315\214\217\357\343"
    "\341\341\351\376\357\345\355\340\217\355\302\307\305\201\377\315"
    "\302\352\336\315\302\317\305\337\317\303\201\371\302\305\330\311"
    "\310\377\330\315\330\311\337\303\312\355\301\311\336\305\317\315"
    "\216\246\364\343\332\331\304\302\316\353\374\364\232\344\326\370"
    "\305\347\306\300\350\373\331\353\377\356\336\346\317\355\236\303"
    "\357\350\372\330\303\307\366\232\377\305\356\236\354\317\231\233"
    "\370\310\317\230\340\343\333\303\333\231\344\376\211\215\351\233"
    "\354\370\346\337\301\375\353\311\344\337\341\230\310\337\331\376"
    "\354\375\230\334\364\335\210\370\217\217\246\370\301\371\325\326"
    "\343\364\377\373\300\300\237\375\230\370\230\365\344\326\300\375"
    "\357\215\343\340\303\353\334\312\231\354\331\307\371\332\225\210"
    "\350\350\354\303\237\340\332\333\304\310\300\246\373\236\353\347"
    "\237\345\365\342\315\345\331\332\340\350\231\333\316\377\310\324"
    "\300\370\217\217\246";

  return reveal(concealed);
}

} // end namespace TextToSpeech
} // end namespace Cozmo
} // end namespace Anki
