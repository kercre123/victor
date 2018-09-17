/**
 * File: BehaviorTwentyQuestions.h
 *
 * Author: ross
 * Created: 2018-09-15
 *
 * Description: 20q
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTwentyQuestions__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTwentyQuestions__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
  class BehaviorPromptUserForVoiceCommand;
  class BehaviorTextToSpeechLoop;
  class BehaviorDisplayWeather;
  class IActionRunner;

class BehaviorTwentyQuestions : public ICozmoBehavior
{
public: 
  virtual ~BehaviorTwentyQuestions();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTwentyQuestions(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void HandleWhileActivated(const GameToEngineEvent& event) override;

private:
  
  void TransitionToWaitingForQuestion();
  void TransitionToQuestionPrompt();
  void TransitionToEmotingAnswer(int answerIdx);
  void TransitionToDone();
  void TransitionToAskToContinue();
  
  void CheckAnswer();
  
  IBehavior* SetUpSpeakingBehavior(const std::string& vocalizationString);
  
  IActionRunner* GetHmmAction();
  void CompleteHmmAction();
  
  void SendCloudMsg(int8_t response) const;
  
  enum class State : uint8_t {
    NotStarted=0,
    WaitingForQuestion,
    Prompting,
    EmotingToAnswer,
    AskToContinue,
  };
  
  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorPromptUserForVoiceCommand> promptBehavior;
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;
    std::shared_ptr<BehaviorDisplayWeather> weatherBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    State state;
    
    int questionNum;
    std::string question;
    std::vector<std::string> answerList;
    
    bool exactPhraseLock;
    bool outOfQuestions;
    bool allowMoreThan20;
    int answerOnLastQuestion;
    
    std::weak_ptr<IActionRunner> waitingAnimLoop;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTwentyQuestions__
