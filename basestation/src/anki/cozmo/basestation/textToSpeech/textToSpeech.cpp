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
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"

const char* filePostfix = "_PlayerName.wav";

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
    _signalHandles.push_back(externalInterface->Subscribe(ExternalInterface::MessageGameToEngineTag::PrepareFaceNameAnimation,
                                                        [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                                                        {
                                                          const ExternalInterface::PrepareFaceNameAnimation& msg = event.GetData().Get_PrepareFaceNameAnimation();
                                                          HandlePlayFaceNameAnimation(msg.faceID, msg.name);
                                                        }));
}


void TextToSpeech::CleanUp()
{

}

void TextToSpeech::HandleAssignNameToFace(Vision::FaceID_t faceId, const std::string& name)
{
  PRINT_NAMED_DEBUG("TextToSpeech.HandleAssignNameToFace", "FaceId %d Name: %s", faceId, name.c_str());
  std::string full_path = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, name + filePostfix);
  flite_text_to_speech(name.c_str(),_voice,full_path.c_str());
}
  
void TextToSpeech::HandlePlayFaceNameAnimation(Vision::FaceID_t faceId,
                                               const std::string& name)
{
  PRINT_NAMED_DEBUG("TextToSpeech.HandlePlayFaceNameAnimation", "FaceId %d Name: %s", faceId, name.c_str());
  
  using namespace Audio;
  ASSERT_NAMED(_audioController != nullptr, "Must Set Audio Controller before preforming text to speach event");
  
  // Check if the WavePortal Plugin is available
  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  if ( pluginInterface->WavePortalIsActive() ) {
    PRINT_NAMED_ERROR("TextToSpeech.HandlePlayFaceNameAnimation", "Wave Portal Plugin is already active faceId %d", faceId);
    
    // Don't change Wave Protal plugin's Audio Data
    return;
  }
  
  // Load file
  // TODO: Need to investigate if this needs to load asynchronously
  std::string fileName = name + filePostfix;
  std::string fullPath = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, fileName );
  bool success = _waveFileReader.LoadWaveFile(fullPath, fileName);
  
  if ( !success ) {
     // Fail =(
    PRINT_NAMED_ERROR("TextToSpeech.HandlePlayFaceNameAnimation", "Failed to Load file: %s", fullPath.c_str());
    return;
  }
  // Set Wave Portal Plugin buffer
  const AudioWaveFileReader::StandardWaveDataContainer* data = _waveFileReader.GetCachedWaveDataWithKey(fileName);
  
  ASSERT_NAMED(pluginInterface != nullptr, "AudioControllerPluginInterface Must be allocated before using it!");
  if ( pluginInterface->WavePortalHasAudioDataInfo() ) {
    pluginInterface->ClearWavePortalAudioDataInfo();
  }
  pluginInterface->SetWavePortalAudioDataInfo( data->sampleRate,
                                               data->numberOfChannels,
                                               data->duration_ms,
                                               data->audioBuffer,
                                               static_cast<uint32_t>(data->bufferSize) );
}

} // end namespace Cozmo
} // end namespace Anki

