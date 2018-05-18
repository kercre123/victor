/**
 * File: BehaviorTextToSpeechLoop.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-05-17
 *
 * Description: Play a looping animation while receiting text to speech
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"

namespace Anki {
namespace Cozmo {

class BehaviorTextToSpeechLoop : public BehaviorAnimGetInLoop
{
public: 
  virtual ~BehaviorTextToSpeechLoop();

  void SetTextToSay(const std::string& textToSay){ _textToSay = textToSay;}

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTextToSpeechLoop(const Json::Value& config);  

  virtual void AnimBehaviorUpdate() override;

private:
  enum class TTSStage{
    SayingText,
    PlayingGetOut
  };

  std::string _textToSay;
  TTSStage _ttsStage;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTextToSpeechLoop__
