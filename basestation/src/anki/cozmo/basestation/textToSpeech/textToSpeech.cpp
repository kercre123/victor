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
extern "C" {
  
#include "flite.h"
  
  cst_voice* register_cmu_us_kal(const char*);
}

#include "anki/cozmo/basestation/textToSpeech/textToSpeech.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"



namespace Anki {
namespace Cozmo {

TextToSpeech::TextToSpeech(IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform)
{
  _dataPlatform = dataPlatform;
  flite_init();
  _voice = register_cmu_us_kal(NULL);
  
  _signalHandles.push_back(externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::AssignNameToFace,
                                                        [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                                                        {
                                                          const ExternalInterface::AssignNameToFace& msg = event.GetData().Get_AssignNameToFace();
                                                          HandleAssignNameToFace(msg.faceID, msg.name);
                                                        }));
}


void TextToSpeech::CleanUp()
{

}

void TextToSpeech::HandleAssignNameToFace(Vision::TrackedFace::ID_t faceID, const std::string& name)
{
  std::string full_path = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, name + "_PlayerName.wav");
  flite_text_to_speech(name.c_str(),_voice,full_path.c_str());
}

} // end namespace Cozmo
} // end namespace Anki

