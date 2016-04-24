/**
* File: textToSpeech
*
* Author: Molly Jameson
* Created: 3/21/16
*
* Update: Andrew Stein, 4/22/16
*
* Description: Flite wrapper to save a wave made from a given text string.
*
* Copyright: Anki, inc. 2016
*
*/


#ifndef __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
#define __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__

#include "anki/cozmo/basestation/audio/audioWaveFileReader.h"
#include "anki/common/types.h"
#include "clad/types/sayTextStyles.h"

#include <unordered_map>

// Forward decl for f-lite
struct cst_voice_struct;

namespace Anki {

  // Forward declaration
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {
  
  // Forward declarations
  namespace Audio {
    class AudioController;
    class AudioWaveFileReader;
  }
  
class TextToSpeech
{
public:
  TextToSpeech(Util::Data::DataPlatform* dataPlatform,
               Audio::AudioController* audioController);
  ~TextToSpeech();

  // Creates the wave file for the given text, to be played later.
  // Returns the full path to the created file.
  std::string CacheSpeech(const std::string& text);
  
  // Sets up the Audio Controller to play the associated text. If the wave file
  // has already been created, uses that one. Otherwise, creates it first.
  Result PrepareToSay(const std::string& text, SayTextStyle style);
  
private:
  
  cst_voice_struct*                  _voice;
  Util::Data::DataPlatform*          _dataPlatform;
  Audio::AudioController*            _audioController;
  Audio::AudioWaveFileReader         _waveFileReader;
  
  // Maps text to filename where it's stored
  std::unordered_map<std::string, std::string> _cachedSpeech;
  
}; // class TextToSpeech

} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
