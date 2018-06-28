/***********************************************************************************************************************
 *
 *  TextToSpeechWrapper
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 5/21/2018
 *
 *  Description
 *  + Simple wrapper for prepping and playing TTS
 *  + Essentially just tracks TextToSpeechCoordinator states for you and simplifies things
 *  + Meant to be used in cases where BehaviorTextToSpeechLoop is too heavy handed, or you're rolling your own anims
 *  * Only supports Robot_Vic_External_Processing::Default_Processed, but would be
 *    easy to extend if the need arose
 *
 **********************************************************************************************************************/

#ifndef __Engine_Components_TextToSpeechWrapper_H__
#define __Engine_Components_TextToSpeechWrapper_H__

#include "components/textToSpeech/textToSpeechTypes.h"
#include <functional>
#include <string>


namespace Anki {
namespace Cozmo {

class TextToSpeechCoordinator;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TextToSpeechWrapper
{
public:
  TextToSpeechWrapper( UtteranceTriggerType triggerType );
  ~TextToSpeechWrapper();

  TextToSpeechWrapper() = delete;

  // only needs to be called once, subsequent calls to Reset() will maintain initialization
  void Initialize( TextToSpeechCoordinator& ttsCoordinator );

  // resets the state, and cancels any currently playing TTS
  // maintains initialization for re-use
  void Reset();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // returns true when the utterance is ready to be played, false if it fails during generation
  using OnUtteranceReadyCallback        = std::function<void(bool)>;
  // returns true when the utterance is finished playing, false if it fails during playback
  using OnUtteranceCompleteCallback     = std::function<void(bool)>;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ... API ...

  // start prepping the utterance text for playback (calls Reset() internally)
  // note: OnUtteranceCompleteCallback is generally only needed when trigger type is "immediate", but you do you.
  // ** once you have begun utterance generation, it is up to you to ensure that either the utterance plays through,
  //    or you cancel it, else the .wav file will orphaned and taking up memory
  bool SetUtteranceText( const std::string& text, OnUtteranceReadyCallback = {}, OnUtteranceCompleteCallback = {} );

  // start playback of the previously generated utterance
  bool PlayUtterance( OnUtteranceCompleteCallback = {} );

  // utterances can be cancelled at any point during their life cycle
  void CancelUtterance();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Access

  // "not valid" can mean there was a failure, or you haven't started generating the utterance yet
  bool IsValid() const { return ( UtteranceState::Invalid != _state ); }

  // should be self explanitory
  bool IsReadyToPlay() const { return ( UtteranceState::Ready == _state ); }
  bool IsPlaying() const { return ( UtteranceState::Playing == _state ); }
  bool IsFinished() const { return ( UtteranceState::Finished == _state ); }


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // callback whenever the TTS Coordinator updates the state of the utterance
  void OnUtteranceUpdated( const UtteranceState& );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  TextToSpeechCoordinator*    _ttsCoordinator;

  UtteranceState              _state;
  UtteranceId                 _id;
  UtteranceTriggerType        _triggerType;

  OnUtteranceReadyCallback    _readyCallback;
  OnUtteranceCompleteCallback _finishedCallback;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Components_TextToSpeechWrapper_H__
