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
#include "util/hashing/hashing.h"

const char* filePrefix = "TextToSpeech_";

namespace Anki {
namespace Cozmo {

  TextToSpeech::TextToSpeech(Util::Data::DataPlatform* dataPlatform,
                             Audio::AudioController* audioController)
: _dataPlatform(dataPlatform)
, _audioController(audioController)
{
  flite_init();
  
  _voice = register_cmu_us_kal(NULL);
}

TextToSpeech::~TextToSpeech()
{
  // Any tear-down needed for flite? (Un-init or unregister_cmu_us_kal?)
}

std::string TextToSpeech::MakeFullPath(const std::string& text)
{
  // In case text contains sensitive data (e.g. player name), hash it so we don't save names
  // into logs if/when there's a message about this filename
  // TODO: Do we need something more secure?
  u32 hashedValue = 0;
  for(auto c : text) {
    Util::AddHash(hashedValue, c, "TextToSpeech.MakeFullPath.HashText");
  }

  // Note that this text-to-hash mapping is only printed in debug mode!   
  PRINT_NAMED_DEBUG("TextToSpeech.MakeFullPath.TextToHash",
                    "'%s' hashed to %d", text.c_str(), hashedValue);
                    
  std::string fullPath = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, 
                                                       filePrefix + std::to_string(hashedValue) + ".wav");
  
  return fullPath;
}
  
std::string TextToSpeech::CreateSpeech(const std::string& text)
{
  auto lutIter = _filenameLUT.find(text);
  
  if(lutIter == _filenameLUT.end())
  {
    // TODO: if not in LUT, also check disk before creating
    
    // Don't already have a wave for this text string: make it now
    // TODO: Make asynchronous
    PRINT_NAMED_DEBUG("TextToSpeech.CacheSpeech", "Text: %s", text.c_str());
    std::string fullPath = MakeFullPath(text);
    flite_text_to_speech(text.c_str(),_voice,fullPath.c_str());
    auto insertResult = _filenameLUT.emplace(text, fullPath);
    lutIter = insertResult.first;
  }
  
  return lutIter->second;
} // CreateSpeech()
  
Result TextToSpeech::PrepareToSay(const std::string& text, SayTextStyle style, ReadyCallback readyCallback)
{
  using namespace Audio;
  
  // Get (or create) the path for a wave file for this text
  // TODO: Make these two asynchronous
  const std::string fullPath = CreateSpeech(text);
  const bool success = _waveFileReader.LoadWaveFile(fullPath, text);
  
  if ( !success ) {
     // Fail =(
    PRINT_NAMED_ERROR("TextToSpeech.PrepareToSay.LoadWaveFileFailed", "Failed to Load file: %s", fullPath.c_str());
    return RESULT_FAIL;
  }
  
  // Set Wave Portal Plugin buffer
  const AudioWaveFileReader::StandardWaveDataContainer* data = _waveFileReader.GetCachedWaveDataWithKey(text);
  
  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  ASSERT_NAMED(pluginInterface != nullptr, "TextToSpeech.PrepareToSay.NullAudioControllerPluginInterface");
  
  if ( pluginInterface->WavePortalHasAudioDataInfo() ) {
    pluginInterface->ClearWavePortalAudioDataInfo();
  }
  
  // TODO: Make use of specified SayTextStyle 
  pluginInterface->SetWavePortalAudioDataInfo( data->sampleRate,
                                               data->numberOfChannels,
                                               data->duration_ms,
                                               data->audioBuffer,
                                               static_cast<uint32_t>(data->bufferSize) );
                                               
  return RESULT_OK;
} // PrepareToSay()

} // end namespace Cozmo
} // end namespace Anki

