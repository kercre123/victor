/**
 * File: BehaviorTextToSpeechLoop.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-17
 *
 * Description: Play a looping animation while receiting text to speech
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/actions/sayTextAction.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::BehaviorTextToSpeechLoop(const Json::Value& config)
: BehaviorAnimGetInLoop(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTextToSpeechLoop::~BehaviorTextToSpeechLoop()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTextToSpeechLoop::AnimBehaviorUpdate()
{
  if(!IsActivated() || IsControlDelegated()){
    return;
  }


  if(!_textToSay.empty()){
    // Play the text to speech action with an infinitely looping animation
    auto* ttsAction = new SayTextAction(_textToSay, SayTextIntent::Text);
    ttsAction->SetAnimationTrigger(GetLoopTrigger());
    DelegateIfInControl(ttsAction);
    _ttsStage = TTSStage::SayingText;
    _textToSay.clear();
  }else{
    if(_ttsStage == TTSStage::SayingText){
      // Call parent update loop to play getout
      RequestLoopEnd();
      _ttsStage = TTSStage::PlayingGetOut;
    }else{
      CancelSelf();
    }
  }
}


}
}
