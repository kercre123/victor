/**
 * File: BehaviorUserDefinedBehaviorSelector.cpp
 *
 * Author: Hamzah Khan
 * Created: 2018-07-17
 *
 * Description: A behavior that enables a user to select a reaction to a condition that becomes the new default behavior to respond to that trigger.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/userDefinedBehaviorTree/behaviorUserDefinedBehaviorSelector.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/userDefinedBehaviorTreeComponent/userDefinedBehaviorTreeComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"


namespace Anki {
namespace Vector {

namespace {
  static const UserIntentTag affirmativeIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag negativeIntent = USER_INTENT(imperative_negative);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorSelector::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorSelector::DynamicVariables::DynamicVariables()
 : currentBehaviorId(BehaviorID::Anonymous)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorSelector::BehaviorUserDefinedBehaviorSelector(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorSelector::~BehaviorUserDefinedBehaviorSelector()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorUserDefinedBehaviorSelector::WantsToBeActivatedBehavior() const
{
  // always wants to be activated if the user-defined behavior tree is enabled
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();
  return userBehaviorTreeComp.IsEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();
  
  // add all possible delegates of the router
  userBehaviorTreeComp.GetAllDelegates(delegates);

  // add text to speech behaviors
  delegates.insert(_iConfig.ttsBehavior.get());
  delegates.insert(_iConfig.confirmationBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::InitBehavior()
{
  const auto& behaviorContainer = GetBEI().GetBehaviorContainer();
  behaviorContainer.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(UserDefinedBehaviorTreeTextToSpeech),
                                                 BEHAVIOR_CLASS(TextToSpeechLoop),
                                                 _iConfig.ttsBehavior );
  behaviorContainer.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(UserDefinedBehaviorTreeConfirmNewBehavior),
                                                 BEHAVIOR_CLASS(PromptUserForVoiceCommand),
                                                 _iConfig.confirmationBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::OnBehaviorActivated()
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();
  BEIConditionType condType = userBehaviorTreeComp.GetCurrentCondition();
  std::set<BehaviorID> behaviorIds = userBehaviorTreeComp.GetConditionOptions(condType);

  // get preset behavior (is anonymous if no preset)
  ICozmoBehaviorPtr delegationBehaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(condType);
  _dVars.currentBehaviorId = (nullptr != delegationBehaviorPtr)? delegationBehaviorPtr->GetID() : BehaviorID::Anonymous;

  // for each behavior, add it to the empty demo queue
  _dVars.demoQueue = std::queue<BehaviorID>();
  for(const auto& behaviorId : behaviorIds) {
    _dVars.demoQueue.emplace(behaviorId);
  }

  // if no behaviors pushed into queue, throw an error
  if(_dVars.demoQueue.empty()) {
    PRINT_NAMED_ERROR("BehaviorUserDefinedBehaviorTreeSelector.OnBehaviorActivated.NoBehaviorsAvailable",
                      "No behavior options available for condition %s.",
                      BEIConditionTypeToString(condType));
    return;
  }

  DemoBehaviorRouter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::DemoBehaviorRouter()
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  // get BEIConditionType and next BehaviorID out of queue
  auto demoBehaviorId = _dVars.demoQueue.front();
  _dVars.demoQueue.pop();
  BEIConditionType condType = userBehaviorTreeComp.GetCurrentCondition();

  // if the queue is now empty, then after demoing the final behavior, the selector should cancel itself
  auto callback = &BehaviorUserDefinedBehaviorSelector::DemoBehaviorCallback;

  // cancel any remaining delegates to avoid weird behavior tree changes
  CancelDelegates();

  // get a pointer to ICozmoBehavior to demo
  userBehaviorTreeComp.AddCondition(condType, demoBehaviorId);
  auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(condType);

  // delegate as desired, to either the cancellation or the selection interaction mechanism
  if(behaviorPtr->WantsToBeActivated()) {
    DelegateNow(behaviorPtr.get(), callback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::DemoBehaviorCallback()
{
  // prompt the user to approve/disapprove of the behavior
  if(_iConfig.confirmationBehavior->WantsToBeActivated()) {
    DelegateNow(_iConfig.confirmationBehavior.get(), &BehaviorUserDefinedBehaviorSelector::PromptYesOrNoCallback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::PromptYesOrNoCallback()
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
  BEIConditionType condType = userBehaviorTreeComp.GetCurrentCondition();

  // if the user likes the behavior, then break out of the selection behavior
  // else if the user doesn't like it, keep trying to demo behaviors
  // otherwise, break out of the loop and reset the original behavior to be delegated.
  // if no original behavior, use the last behavior demoed.
  if(uic.IsUserIntentPending(affirmativeIntent)) {     
    uic.DropUserIntent(affirmativeIntent);
    CancelDelegatesAndSelf();
  }
  else if(uic.IsUserIntentPending(negativeIntent)) {
    const std::string negativeString = "Ok, let's try another one.";
    const std::string noSelectionString = "Looks like we've run out of options. Let's try again.";
    std::string responseString = negativeString;
    auto callback = &BehaviorUserDefinedBehaviorSelector::DemoBehaviorRouter;

    // if no behaviors left to demo, restart the demo loop
    if(_dVars.demoQueue.empty()) {
      responseString = noSelectionString;
      userBehaviorTreeComp.AddCondition(condType, _dVars.currentBehaviorId);
      callback = &BehaviorUserDefinedBehaviorSelector::OnBehaviorActivated;
    }

    uic.DropUserIntent(negativeIntent);
    
    // say what we need to say
    DelegateNow(SetUpSpeakingBehavior(responseString), callback);
  }
  else {
    const std::string noResponseString = "I didn't hear anything. ";
    const std::string noBehaviorString = "I'll just choose something.";
    const std::string sameThingString = "I'll just do the same thing next time.";
    std::string responseString;

    // exit the behavior
    if(_dVars.currentBehaviorId == BehaviorID::Anonymous) {
      // uses the last behavior demoed as the new mapping
      responseString = noResponseString + noBehaviorString;
    }
    else {
      responseString = noResponseString + sameThingString;
      userBehaviorTreeComp.AddCondition(condType, _dVars.currentBehaviorId);
    }
     
    DelegateNow(SetUpSpeakingBehavior(responseString), &BehaviorUserDefinedBehaviorSelector::CancelDelegatesAndSelf);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorSelector::CancelDelegatesAndSelf()
{
  // cancel self also cancels delegates
  CancelSelf();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorUserDefinedBehaviorSelector::SetUpSpeakingBehavior(const std::string& vocalizationString)
{
  _iConfig.ttsBehavior->SetTextToSay(vocalizationString);
  ANKI_VERIFY(_iConfig.ttsBehavior->WantsToBeActivated(),
              "BehaviorBlackjack.TTSError",
              "The TTSLoop behavior did not want to be activated, this indicates a usage error");
  return _iConfig.ttsBehavior.get();
}

}
}
