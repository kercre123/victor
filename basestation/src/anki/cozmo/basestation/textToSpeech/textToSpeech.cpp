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

const char* filePostfix = "_TextToSpeech.wav";

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
  
std::string TextToSpeech::CacheSpeech(const std::string& text)
{
  auto cacheIter = _cachedSpeech.find(text);
  
  if(cacheIter == _cachedSpeech.end())
  {
    // Don't already have a wave for this text string: make it now
    PRINT_NAMED_DEBUG("TextToSpeech.CacheSpeech", "Text: %s", text.c_str());
    // TODO: create filename that doesn't contain the text itself, in case that text is a name and we display the filename in a logged message at some point (privacy)
    std::string fullPath = _dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, text + filePostfix);
    flite_text_to_speech(text.c_str(),_voice,fullPath.c_str());
    auto insertResult = _cachedSpeech.emplace(text, fullPath);
    cacheIter = insertResult.first;
  }
  
  return cacheIter->second;
} // CacheText()

  
Result TextToSpeech::PrepareToSay(const std::string& text, SayTextStyle style) 
{
  using namespace Audio;
  
  // Get (or create) the path for a wave file for this text
  const std::string fullPath = CacheSpeech(text);
  
  // TODO: May need to be asynchronous
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

