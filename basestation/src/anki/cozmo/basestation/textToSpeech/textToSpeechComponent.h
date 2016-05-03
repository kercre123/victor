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

// TODO: Need to finish implement SayTextStyle in all methods!!


#ifndef __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
#define __Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__

#include "anki/cozmo/basestation/audio/standardWaveDataContainer.h"
#include "anki/common/types.h"
#include "clad/types/sayTextStyles.h"
#include "util/helpers/templateHelpers.h"
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

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
  
// Forward declaration
class CozmoContext;
namespace Audio {
  class AudioController;
}
  
class TextToSpeechComponent
{
public:
  
  // Text to Speech data state
  enum class PhraseState {
    None,       // Does NOT exist
    Preparing,  // In process of creating data
    Ready       // Data is ready to use
  };
  
  
  TextToSpeechComponent(const CozmoContext* context);
  ~TextToSpeechComponent();

  // Asynchronous create the wave data for the given text and style, to be played later
  // Use GetSpeechState() to check if wave data is Ready
  // Return state of text/style pair
  PhraseState CreateSpeech(const std::string& text, SayTextStyle style);
  
  // Get the current state of the text/style pair
  PhraseState GetSpeechState(const std::string& text, const SayTextStyle style);
  
  // Set up Audio controller to play text's audio data
  // Return false if the audio has NOT been created or is not yet ready
  bool PrepareToSay(const std::string& text, SayTextStyle style);
  
  // Clear loaded text/style pair audio data from memory
  void ClearLoadedSpeechData(const std::string& text, SayTextStyle style);
  
  // Clear ALL loaded text audio data from memory
  void ClearAllLoadedAudioData();


private:
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Text Phrase Bundle contains wave data for text phrase
  struct TextPhraseBundle {
  public:
    
    // State of audio data for style
    PhraseState GetPhraseState(const SayTextStyle style)
    {
      PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
      if ( phraseBinding == nullptr ) {
        return PhraseState::None;
      }
      return phraseBinding->state;
    }
    
    // Get wave data for style
    Audio::StandardWaveDataContainer* GetWaveData(const SayTextStyle style)
    {
      PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
      if ( phraseBinding == nullptr ) {
        return nullptr;
      }
      return phraseBinding->waveData;
    }
    
    // Prepare text style for audio creation
    // Return the state of a style
    PhraseState PrepareTextPhrase(const SayTextStyle style)
    {
      TextAudioCreationStyle creationStyle = GetTextAudioCreationStyle( style );
      auto iter = _phraseStyleMap.find( creationStyle );
      if ( iter == _phraseStyleMap.end() ) {
        // Create entry for text creation style
        PhraseCreationStyleBinding binding;
        binding.state = PhraseState::Preparing;
        iter = _phraseStyleMap.emplace( creationStyle, binding ).first;
      }
      // Track Styles using Phrase Style Binding
      iter->second.registeredStyleSet.insert( style );
      
      return iter->second.state;
    }
    
    // Store (retain) wave data for style
    // Return false if text audio style is not prepared first, callers is responsible to handle wave data memory
    bool SetStandardWaveDataContainer(Audio::StandardWaveDataContainer* waveData , const SayTextStyle style)
    {
      ASSERT_NAMED( waveData != nullptr, "WaveData can NOT == nullptr" );
      // Check if audio data already exist
      PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
      if ( phraseBinding == nullptr ) {
        // Test audio style is NOT prepared
        return false;
      }
      const auto styleIter = phraseBinding->registeredStyleSet.find( style );
      if ( styleIter == phraseBinding->registeredStyleSet.end() ) {
        PRINT_NAMED_WARNING("TextPhraseBundle.SetStandardWaveDataContainer",
                            "Setting wave data for SayTextStyle [%hhu] that has not been registered. We are able to \
                             continue because another SayTextStyle has already created the necessary Wave Data", style);
      }

      // Add Wave data
      phraseBinding->state = PhraseState::Ready;
      phraseBinding->waveData = waveData;
      
      return true;
    }
    
    // Remove style & audio data for style
    void ClearTextPhrase(const SayTextStyle style)
    {
      // Find TextAudioCreationStyle PhraseCreationStyleBinding for SayTextStyle
      auto phraseStyleIter = _phraseStyleMap.find( GetTextAudioCreationStyle( style ) );
      if ( phraseStyleIter == _phraseStyleMap.end() ) {
        PRINT_NAMED_WARNING("TextPhraseBundle.ClearTextPhrase", "Could NOT find TextAudioCreationStyle for SayTextStyle [%hhu]", style);
        return;
      }
      // Find registered SayTextStyle in PhraseCreationStyleBinding
      auto& registeredStyleSet = phraseStyleIter->second.registeredStyleSet;
      const auto iter = registeredStyleSet.find( style );
      if ( iter == registeredStyleSet.end() ) {
        PRINT_NAMED_WARNING("TextPhraseBundle.ClearTextPhrase", "Could NOT find registered SayTextStyle [%hhu]", style);
      }
      
      // Unregister SayTextStyle in PhraseCreationStyleBinding
      registeredStyleSet.erase(iter);
      // If no other styles are using Creation Style, delete entry
      if ( registeredStyleSet.empty() ) {
        _phraseStyleMap.erase( phraseStyleIter );
      }
    }

  private:
    
    enum class TextAudioCreationStyle
    {
      Normal // TODO: Add More styles
    };
    
    // Bundle stat, SayTextStyles & wave data for a TextAudioCreationStyle
    struct PhraseCreationStyleBinding {
      PhraseState state = PhraseState::None;
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
    TextAudioCreationStyle GetTextAudioCreationStyle(const SayTextStyle style) const
    {
      // TODO: need method to map from SayTextStyle -> TextAudioCreationStyle
      return TextAudioCreationStyle::Normal;
    }
    
    // Helper
    // Find corresponding PhraseCreationStyleBinding for style
    PhraseCreationStyleBinding* GetPhraseCreationStyleBinding(const SayTextStyle style)
    {
      auto phraseStyleIter = _phraseStyleMap.find( GetTextAudioCreationStyle( style ) );
      if ( phraseStyleIter == _phraseStyleMap.end() ) {
        return nullptr;
      }
      return &phraseStyleIter->second;
    }
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
  
  // Helpers
  // Find TextPhraseBundle for text
  TextPhraseBundle* GetTextPhraseBundle(const std::string& text);
  
  // Create text that can be used to send over Network and used in Logs
  std::string CreateHashForText(const std::string& text);

}; // class TextToSpeech


} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_cozmo_Basestation_textToSpeech_textToSpeech_H__
