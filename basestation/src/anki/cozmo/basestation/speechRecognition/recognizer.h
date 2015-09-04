/**
* File: recognizer
*
* Author: damjan stulic
* Created: 9/3/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#ifndef __Anki_cozmo_Basestation_SpeechRecognition_Recognizer_H__
#define __Anki_cozmo_Basestation_SpeechRecognition_Recognizer_H__

#include <sphinxbase/ad.h>
#include <pocketsphinx.h>
#include <string>

namespace Anki {
namespace Cozmo {

class IExternalInterface;

class Recognizer {
public:
  Recognizer(IExternalInterface* externalInterface);
  ~Recognizer();
  void Init(const std::string& hmmFile, const std::string& file, const std::string& dictFile);
  void CleanUp();
  void Start();
  void Update(unsigned int millisecondsPassed);
  void Stop();

private:

  IExternalInterface* _externalInterface;

  // private members for keyword search
  static const uint32 adbuffSize = 2048;
  ps_decoder_t *ps;
  cmd_ln_t *config;
  ad_rec_t *ad;
  int16 adbuf[adbuffSize];
  bool utt_started;
  uint8 in_speech;
  int32 k;
  //char const *hyp;
};

} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_cozmo_Basestation_SpeechRecognition_Recognizer_H__
