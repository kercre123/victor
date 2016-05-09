/**
* File: KeyWordRecognizer
*
* Author: damjan stulic
* Created: 7/4/16
*
* Description: 
*
* Copyright: Anki, inc. 2016
*
*/


#ifndef __Anki_cozmo_Basestation_SpeechRecognition_KeyWordRecognizer_H__
#define __Anki_cozmo_Basestation_SpeechRecognition_KeyWordRecognizer_H__

#include <string>

namespace Anki {
namespace Cozmo {

class IExternalInterface;

namespace SpeechRecognition {


class KeyWordRecognizer {
public:
  KeyWordRecognizer(IExternalInterface* externalInterface) {};
  void Init(const std::string& hmmFile, const std::string& file, const std::string& dictFile) {};
  void CleanUp() {};
  void Start() {};
  void Update(unsigned int millisecondsPassed) {};
  void Stop() {};
};

} // end namespace SpeechRecognition
} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_cozmo_Basestation_SpeechRecognition_KeyWordRecognizer_H__
