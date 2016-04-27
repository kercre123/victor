/**
* File: textToSpeechComponent.h
*
* Author: Molly Jameson
* Created: 3/21/16
*
* Overhaul: Andrew Stein / Jordan Rivas, 4/22/16
*
* Description: Flite wrapper to save a wave made from a given text string.
*
* Copyright: Anki, inc. 2016
*
*/

// TODO: Need to implement SayTextStyle in all methods!!


#ifndef __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
#define __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__

#include "anki/cozmo/basestation/audio/audioWaveFileReader.h"
#include "anki/common/types.h"
#include "clad/types/sayTextStyles.h"
#include <unordered_map>
#include <vector>

// Forward decl for f-lite
struct cst_voice_struct;

namespace Anki {

// Forward declaration
namespace Util {
namespace Data {
  class DataPlatform;
}
  
namespace Dispatch {
  class Queue;
}
}
  
namespace Cozmo {
  
// Forward declaration
class CozmoContext;
namespace Audio {
  class AudioController;
  class AudioWaveFileReader;
}
  
class TextToSpeechComponent
{
public:
  
  using CompletionFunc = std::function<void(bool success, const std::string& text, const std::string& fileName)>;
  using SimpleCompletionFunc = std::function<void(void)>;
  
  
  TextToSpeechComponent(const CozmoContext* context);
  ~TextToSpeechComponent();

  // Asynchronous create the wave file for the given text, to be played later.
  // The completion callback will be called after the file has been stored on disk.
  void CreateSpeech(const std::string& text, CompletionFunc completion = nullptr);
  
  // Asynchronous load text's audio data into memory, if the text's associated .wav file for hasn't already been created
  // it will perform CreateSpeech() first.
  // The completion callback is run once the text's audio data is loaded into memory.
  void LoadSpeechData(const std::string& text, SayTextStyle style, CompletionFunc completion = nullptr);
  
  // Unload text's audio data from memory.
  void UnloadSpeechData(const std::string& text, SayTextStyle style);
  
  // Set up Audio controller to play text's audio data.
  // Return false if the text's .wav is not created or LoadSpeechData() method has not been called before performing
  // this method, out_duration_ms will NOT be valid.
  bool PrepareToSay(const std::string& text, SayTextStyle style, float& out_duration_ms);
  
  // Asynchronous clear loaded text's audio data from memory
  void ClearLoadedSpeechData(const std::string& text, SayTextStyle style, SimpleCompletionFunc completion = nullptr);
  
  // Asynchronous clear ALL loaded text audio data from memory, if deleteStaleFiles is true all text to speech files
  // on disk will be deleted.
  void ClearAllLoadedAudioData(bool deleteStaleFiles, SimpleCompletionFunc completion = nullptr);


private:
  
  Util::Data::DataPlatform*       _dataPlatform = nullptr;
  Util::Dispatch::Queue*          _dispatchQueue = nullptr;
  Audio::AudioController*         _audioController = nullptr;
  Audio::AudioWaveFileReader      _waveFileReader;
  cst_voice_struct*               _voice = nullptr;
  
  // Maps text to filename where it's stored
  std::unordered_map<std::string, std::string> _filenameLUT;
  
  // Helper to obscure filenames via a hash, in case text contains sensitive 
  // data like a name
  std::string MakeFullPath(const std::string& text);
  
  // Find all files in directory that begin with prefix "TextToSpeech_"
  std::vector<std::string> FindAllTextToSpeechFiles();
  
}; // class TextToSpeech


} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
