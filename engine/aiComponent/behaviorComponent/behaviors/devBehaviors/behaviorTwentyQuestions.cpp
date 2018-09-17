/**
 * File: behaviorTwentyQuestions.cpp
 *
 * Author: ross
 * Created: 2018-09-15
 *
 * Description: 20q
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorTwentyQuestions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/weather/behaviorDisplayWeather.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Vector {
  
  namespace {
    constexpr int8_t kResetAndStart = -1;
    
#define SET_STATE(s) do{ \
_dVars.state = State::s; \
PRINT_NAMED_INFO("WHATNOW", "State = %s", #s); \
} while(0);
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTwentyQuestions::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTwentyQuestions::DynamicVariables::DynamicVariables()
{
  questionNum = 0;
  state = State::NotStarted;
  exactPhraseLock = false;
  outOfQuestions = false;
  allowMoreThan20 = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTwentyQuestions::BehaviorTwentyQuestions(const Json::Value& config)
 : ICozmoBehavior(config)
{
  SubscribeToTags({ExternalInterface::MessageGameToEngineTag::TwentyQuestionsQuestions});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTwentyQuestions::~BehaviorTwentyQuestions()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTwentyQuestions::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  return uic.IsUserIntentPending( USER_INTENT(twenty_questions) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.promptBehavior.get());
  delegates.insert(_iConfig.ttsBehavior.get());
  delegates.insert(_iConfig.weatherBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

void BehaviorTwentyQuestions::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(TwentyQuestionsPrompt),
                                  BEHAVIOR_CLASS(PromptUserForVoiceCommand),
                                  _iConfig.promptBehavior );
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(WeatherTextToSpeech),
                                  BEHAVIOR_CLASS(TextToSpeechLoop),
                                  _iConfig.ttsBehavior );
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(WeatherCloudyGeneric),
                                 BEHAVIOR_CLASS(DisplayWeather),
                                 _iConfig.weatherBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  SmartActivateUserIntent( USER_INTENT(twenty_questions) );
  
  // tell cloud process to start the questioning
  SendCloudMsg( kResetAndStart );
  
  DelegateIfInControl( SetUpSpeakingBehavior("Alright."), &BehaviorTwentyQuestions::TransitionToWaitingForQuestion );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  if( (_dVars.state == State::NotStarted || _dVars.state == State::EmotingToAnswer)
      && !IsControlDelegated() )
  {
    if( !_dVars.question.empty() ) {
      TransitionToQuestionPrompt();
    } else if( _dVars.outOfQuestions ) {
      // note: this can also happen if the action gets canceled for some reason.
      PRINT_NAMED_WARNING("WHATNOW", "Not delegated, but no question. done");
      TransitionToDone();
    }
    // todo: timeout from cloud
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorTwentyQuestions::SetUpSpeakingBehavior(const std::string& vocalizationString)
{
  _iConfig.ttsBehavior->SetTextToSay( vocalizationString );
  if( !ANKI_VERIFY( _iConfig.ttsBehavior->WantsToBeActivated(),
                   "BehaviorTwentyQuestions.TTSERROR",
                 "WHATNOW The TTSLoop behavior did not want to be activated, this indicates a usage error")){
    CancelSelf();
  }
  return _iConfig.ttsBehavior.get();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::SendCloudMsg(int8_t response) const
{
  RobotInterface::TwentyQuestionsInput msg{ response };
  RobotInterface::EngineToRobot wrapper(std::move(msg));
  GetBEI().GetRobotInfo().SendRobotMessage( wrapper );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::TransitionToWaitingForQuestion()
{
  SET_STATE(WaitingForQuestion);
  if( !_dVars.question.empty() || _dVars.outOfQuestions ) {
    // already has question
    TransitionToQuestionPrompt();
  } else {
    // once control is no longer delegated, BehaviorUpdate takes over (todo: should be callback)
    DelegateIfInControl( GetHmmAction(), [](){} );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::TransitionToQuestionPrompt()
{
  SET_STATE(Prompting);
  
  std::string prompt = _dVars.question;
  _dVars.question = "";
  
  // standardize first question
  const static std::string kExpectedInitialQuestion = "Is it classified as Animal, Vegetable or Mineral?";
  if( prompt == kExpectedInitialQuestion ) {
    prompt = "Is it an animal, vegetable, or mineral?";
  }
  
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  std::set<std::string> exactPhrases{ _dVars.answerList.begin(), _dVars.answerList.end() };
  if( exactPhrases.find("Right") != exactPhrases.end() ) {
    exactPhrases.insert("Yes");
  }
  if( exactPhrases.find("Wrong") != exactPhrases.end() ) {
    exactPhrases.insert("No");
  }
  // Saying "no" is often heard as "now" but with imperative_negative, so always map imperative_negative
  // to "no" if there is not an exact phrase mach
  std::map<UserIntentTag, std::string> matchedIntentMap;
  if( exactPhrases.find("Yes") != exactPhrases.end() ) {
    matchedIntentMap.emplace( USER_INTENT(imperative_affirmative), "Yes" );
  }
  if( exactPhrases.find("No") != exactPhrases.end() ) {
    matchedIntentMap.emplace( USER_INTENT(imperative_negative), "No" );
  }
  uic.SetMustMatchExactPhrases( exactPhrases, GetDebugLabel(), matchedIntentMap );
  _dVars.exactPhraseLock = true;
  
  _iConfig.promptBehavior->SetPrompt( prompt );
  
  auto afterDisplayNumber = [&]() {
    
    if( _iConfig.promptBehavior->WantsToBeActivated() ) {
      PRINT_NAMED_WARNING("WHATNOW", "Prompting");
      DelegateIfInControl( _iConfig.promptBehavior.get(), &BehaviorTwentyQuestions::CheckAnswer );
    } else {
      CancelSelf();
    }
  };
  
  _iConfig.weatherBehavior->SetInput( _dVars.questionNum );
  if( _iConfig.weatherBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.weatherBehavior.get(), afterDisplayNumber );
  } else {
    PRINT_NAMED_WARNING("WHATNOW", "skipping displaying of weather");
    afterDisplayNumber();
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::CheckAnswer()
{
  CancelDelegates(false); // shouldnt be needed
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsUserIntentPending( USER_INTENT(exact_phrase) ) ) {
    uic.UnSetMustMatchExactPhrases( GetDebugLabel() );
    _dVars.exactPhraseLock = false;
    
    const auto* intent = uic.GetPendingUserIntent();
    const std::string phrase = intent->intent.Get_exact_phrase().phrase;
    PRINT_NAMED_WARNING("WHATNOW", "answered %s", phrase.c_str());
    uic.DropUserIntent( USER_INTENT(exact_phrase) );
    if( _dVars.state == State::AskToContinue ) {
      if( Util::StringCaseInsensitiveEquals(phrase, "Yes") ) {
        _dVars.allowMoreThan20 = true;
        TransitionToEmotingAnswer( _dVars.answerOnLastQuestion );
      } else {
        TransitionToDone();
      }
    } else {
      for( int i=0; i<_dVars.answerList.size(); ++i ) {
        const auto& possibleAns = _dVars.answerList[i];
        const bool match = (phrase == possibleAns) || (Util::StringCaseInsensitiveEquals(phrase, "Yes") &&
                                                       Util::StringCaseInsensitiveEquals(possibleAns, "Right"))
                                                   || (Util::StringCaseInsensitiveEquals(phrase, "No") &&
                                                       Util::StringCaseInsensitiveEquals(possibleAns, "Wrong"));
        if( match ) {
          const bool guessedCorrectly = Util::StringCaseInsensitiveEquals(possibleAns, "Right");
          if( guessedCorrectly ) {
            DelegateIfInControl( new TriggerAnimationAction{AnimationTrigger::BlackJack_VictorWin}, [&](ActionResult res){
              // there's some bug where there's an unclaimed intent. normally this is ok, but doesnt
              // fit if the robot won
              auto& uic = GetBehaviorComp<UserIntentComponent>();
              if( uic.WasUserIntentUnclaimed() ) {
                uic.ResetUserIntentUnclaimed();
              }
              TransitionToDone();
            });
          } else if ( (_dVars.questionNum >= 20) && !_dVars.allowMoreThan20 ) {
            _dVars.answerOnLastQuestion = i;
            DelegateIfInControl( new TriggerAnimationAction{AnimationTrigger::BlackJack_VictorBust}, [this](ActionResult res){
              TransitionToAskToContinue();
            });
          } else {
            TransitionToEmotingAnswer( i );
          }
          return;
        }
      }
      PRINT_NAMED_WARNING("WHATNOW", "no answer match");
      // todo: check substrings if nothing matches
      TransitionToDone();
    }
  } else {
    PRINT_NAMED_WARNING("WHATNOW", "Got no response from user");
    // animation
    TransitionToDone();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::TransitionToEmotingAnswer(int answerIdx)
{
  SET_STATE(EmotingToAnswer);
  SendCloudMsg( (int8_t)answerIdx );
  
  
  
  // once control is no longer delegates, BehaviorUpdate takes over (todo: should be callback)
  DelegateIfInControl( GetHmmAction(), [](){} );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::TransitionToAskToContinue()
{
  SET_STATE(AskToContinue);
  
  const std::string prompt = "Should I keep guessing?";
  
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  std::set<std::string> exactPhrases;
  exactPhrases.insert("Yes");
  exactPhrases.insert("No");
  uic.SetMustMatchExactPhrases( exactPhrases, GetDebugLabel() );
  _dVars.exactPhraseLock = true;
  
  _iConfig.promptBehavior->SetPrompt( prompt );
  
  
  if( _iConfig.promptBehavior->WantsToBeActivated() ) {
    PRINT_NAMED_WARNING("WHATNOW", "Prompting");
    DelegateIfInControl( _iConfig.promptBehavior.get(), &BehaviorTwentyQuestions::CheckAnswer );
  } else {
    TransitionToDone();
  }
  
  

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::TransitionToDone()
{
  CancelSelf();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorTwentyQuestions::GetHmmAction()
{
  auto* action = new CompoundActionSequential();
  action->AddAction( new TriggerLiftSafeAnimationAction{AnimationTrigger::PlanningGetIn} );
  _dVars.waitingAnimLoop = action->AddAction( new ReselectingLoopAnimationAction{AnimationTrigger::PlanningLoop} );
  action->AddAction( new TriggerLiftSafeAnimationAction{AnimationTrigger::PlanningGetOut} );
  return action;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::CompleteHmmAction()
{
  auto action = _dVars.waitingAnimLoop.lock();
  if( action != nullptr ) {
    PRINT_NAMED_WARNING("WHATNOW", "Got lock");
    auto castPtr = std::dynamic_pointer_cast<ReselectingLoopAnimationAction>(action);
    if( castPtr != nullptr ) {
      PRINT_NAMED_WARNING("WHATNOW", "Got cast");
      castPtr->StopAfterNextLoop();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::HandleWhileActivated(const GameToEngineEvent& event)
{
  if( event.GetData().GetTag() == GameToEngineTag::TwentyQuestionsQuestions ) {
    _dVars.questionNum = event.GetData().Get_TwentyQuestionsQuestions().questionNum;
    _dVars.question = std::move(event.GetData().Get_TwentyQuestionsQuestions().question);
    Util::StringReplace( _dVars.question, "I am guessing that it is", "Is it" );
    _dVars.answerList = std::move(event.GetData().Get_TwentyQuestionsQuestions().answers);
    std::stringstream ss;
    for( const auto& answer : _dVars.answerList ) {
      ss << answer << ",";
    }
    PRINT_NAMED_WARNING("WHATNOW", "QUESTION %d: \"%s\" (%s)", _dVars.questionNum, _dVars.question.c_str(), ss.str().c_str());
    if( _dVars.question.empty() ) {
      _dVars.outOfQuestions = true;
    }
    if( IsControlDelegated() ) {
      CompleteHmmAction();
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTwentyQuestions::OnBehaviorDeactivated()
{
  if( _dVars.exactPhraseLock ) {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    uic.UnSetMustMatchExactPhrases( GetDebugLabel() );
    _dVars.exactPhraseLock = false;
  }
}

}
}
