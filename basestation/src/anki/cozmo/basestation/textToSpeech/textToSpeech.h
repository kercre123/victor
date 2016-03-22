/**
* File: textToSpeech
*
* Author: Molly Jameson
* Created: 3/21/16
*
* Description: Flite wrapper to save a wav.
*
* Copyright: Anki, inc. 2016
*
*/


#ifndef __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
#define __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__


#include "util/signals/simpleSignal_fwd.h"
#include "anki/vision/basestation/trackedFace.h"
#include <vector>

struct cst_voice_struct;

namespace Anki {

  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
namespace Cozmo {

class IExternalInterface;



class TextToSpeech {
public:
  TextToSpeech(IExternalInterface* externalInterface,Util::Data::DataPlatform* dataPlatform);
  void CleanUp();

private:
  void HandleAssignNameToFace(Vision::TrackedFace::ID_t faceID, const std::string& name);
  std::vector<Signal::SmartHandle> _signalHandles;
  cst_voice_struct* _voice;
  Util::Data::DataPlatform* _dataPlatform;
};

} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
