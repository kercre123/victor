/**
* File: textToSpeechComponent.h
*
* Author: Molly Jameson
* Created: 3/21/16
*
* Overhaul: Andrew Stein / Jordan Rivas, 5/02/16
*
* Description: Flite wrapper to generate, cache and use wave data from a given string and style.
*
* Copyright: Anki, inc. 2016
*
*/


#ifndef __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
#define __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__

#include "anki/cozmo/basestation/audio/standardWaveDataContainer.h"
#include "anki/common/types.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/types/sayTextStyles.h"
#include "util/helpers/templateHelpers.h"
#include <mutex>
#include <set>
#include <unordered_map>


// Forward decl for f-lite
struct cst_voice_struct;

namespace Anki {

// Forward declaration
namespace Util {
namespace Dispatch {
  class Queue;
}
}
  
namespace Cozmo {
  
class CozmoContext;
namespace Audio {
  class AudioController;
}
  
class TextToSpeechComponent
{
public:
  
  // Text to Speech data state
  enum class SpeechState {
    None,       // Does NOT exist
    Preparing,  // In process of creating data
    Ready       // Data is ready to use
  };
  
  
  TextToSpeechComponent(const CozmoContext* context);
  ~TextToSpeechComponent();

  // Asynchronous create the wave data for the given text and style, to be played later
  // Use GetSpeechState() to check if wave data is Ready
  // Return state of text/style pair
  SpeechState CreateSpeech(const std::string& text, SayTextStyle style);
  
  // Get the current state of the text/style pair
  SpeechState GetSpeechState(const std::string& text, const SayTextStyle style);
  
  // Set up Audio controller to play text's audio data
  // out_durration_ms provides approximate durration of event before proccessing in audio engine
  // Return false if the audio has NOT been created or is not yet ready, out_duration_ms will NOT be valid.
  bool PrepareToSay(const std::string& text,
                    SayTextStyle style,
                    Audio::GameObjectType audioGameObject,
                    float& out_duration_ms);
  
  // Clear loaded text/style pair audio data from memory
  void ClearLoadedSpeechData(const std::string& text, SayTextStyle style);
  
  // Clear ALL loaded text audio data from memory
  void ClearAllLoadedAudioData();
  
  // Get appropriate Audio Event for SayTextStyle
  Audio::GameEvent::GenericEvent GetAudioEvent(SayTextStyle style);


private:
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Text Phrase Bundle contains wave data for text phrases styles
  class TextPhraseBundle {
  public:
    
    // State of audio data for style
    SpeechState GetSpeechState(const SayTextStyle style);
    
    // Get wave data for style
    Audio::StandardWaveDataContainer* GetWaveData(const SayTextStyle style);
    
    // Prepare text style for audio creation
    // Return the state of a style
    SpeechState PrepareTextPhrase(const SayTextStyle style);
    
    // Store (retain) wave data for style
    // Return false if text audio style is not prepared first, callers is responsible to handle wave data memory
    bool SetStandardWaveDataContainer(Audio::StandardWaveDataContainer* waveData , const SayTextStyle style);
    
    // Remove style & audio data for style
    void ClearTextPhrase(const SayTextStyle style);

  private:
    
    enum class TextAudioCreationStyle
    {
      Normal // TODO: Add More styles
    };
    
    // Bundle stat, SayTextStyles & wave data for a TextAudioCreationStyle
    struct PhraseCreationStyleBinding {
      SpeechState state = SpeechState::None;
      std::set<SayTextStyle> registeredStyleSet;
      Audio::StandardWaveDataContainer* waveData = nullptr;
      
      ~PhraseCreationStyleBinding()
      {
        Util::SafeDelete( waveData );
      }
    };
    
    // Hold TextAudioCreationStyle data
    std::unordered_map< TextAudioCreationStyle, PhraseCreationStyleBinding, Util::EnumHasher > _phraseStyleMap;
    
    // Map SayTextStyle to TextAudioCreationStyles
    TextAudioCreationStyle GetTextAudioCreationStyle(const SayTextStyle style) const;
    
    // Helper
    // Find corresponding PhraseCreationStyleBinding for style
    PhraseCreationStyleBinding* GetPhraseCreationStyleBinding(const SayTextStyle style);
  };
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Vars
  Util::Dispatch::Queue*          _dispatchQueue = nullptr;
  Audio::AudioController*         _audioController = nullptr;
  cst_voice_struct*               _voice = nullptr;
  
  std::unordered_map<std::string, TextPhraseBundle> _textPhraseCache;
  std::mutex _lock;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Methods
  
  // Use Text to Speech lib to create audio data & reformat into StandardWaveData format
  // Return nullptr if Text to Speech lib fails to create audio data
  Audio::StandardWaveDataContainer* CreateAudioData(const std::string& text, const SayTextStyle style);
  
  // To improve how cozmo says words we provide the length of the audio to the audio engine
  void PrepareSpeechAudioRtpc(const SayTextStyle style, Audio::GameObjectType audioGameObject, float duration_ms);
  
  // Helpers
  // Find TextPhraseBundle for text
  TextPhraseBundle* GetTextPhraseBundle(const std::string& text);
  
  // Create text that can be used to send over Network and used in Logs
  std::string CreateHashForText(const std::string& text);

}; // class TextToSpeech


} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
