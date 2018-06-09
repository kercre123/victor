/**
 * File: BehaviorBlackJack.cpp
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: BlackJack
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/behaviorBlackJack.h"

#include "clad/types/behaviorComponent/behaviorStats.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/robotStatsTracker.h"

#define SET_STATE(s) do{ \
                          _dVars.state = EState::s; \
                          PRINT_NAMED_INFO("BehaviorBlackJack.State", "State = %s", #s); \
                        } while(0);

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::DynamicVariables::DynamicVariables()
: state(EState::TurnToFace)
, outcome(EOutcome::Tie)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::BehaviorBlackJack(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::~BehaviorBlackJack()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.hitOrStandPromptBehavior.get());
  delegates.insert(_iConfig.playAgainPromptBehavior.get());
  delegates.insert(_iConfig.ttsBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBlackJack::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(BlackJackHitOrStandPrompt),
                                  BEHAVIOR_CLASS(PromptUserForVoiceCommand),
                                  _iConfig.hitOrStandPromptBehavior );

  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(BlackJackRequestToPlayAgain),
                                  BEHAVIOR_CLASS(PromptUserForVoiceCommand),
                                  _iConfig.playAgainPromptBehavior );

  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(BlackJackTextToSpeech),
                                  BEHAVIOR_CLASS(TextToSpeechLoop),
                                  _iConfig.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  //   // TODO: insert any possible root-level json keys that this class is expecting.
  //   // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.game.Init(&GetRNG());

  TransitionToTurnToFace();
 }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToTurnToFace()
{
  SET_STATE(TurnToFace);
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(), &BehaviorBlackJack::TransitionToGetIn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToGetIn()
{
  SET_STATE(GetIn);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_GetIn),
                      &BehaviorBlackJack::TransitionToDealing);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToDealing()
{
  SET_STATE(Dealing);
  _dVars.game.DealCards();
  _dVars.visualizer.VisualizeDealing();

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Deal),
                      &BehaviorBlackJack::TransitionToReactToPlayerCard);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToReactToPlayerCard()
{
  SET_STATE(ReactToPlayerCard);

  if(!_dVars.game.PlayerHasHit()){
    // Only do these immediately after Dealing
    if(_dVars.game.PlayerHasBlackJack()){
      // "Natural" BlackJack, player wins
      // TODO:(str) switch to polished behavior * * * * *
      _dVars.outcome = EOutcome::VictorLosesBlackJack;
      DelegateIfInControl(SetUpSpeakingBehavior("Natural BlackJack, You win!"), 
                          &BehaviorBlackJack::TransitionToEndGame);
    } else if(_dVars.game.PlayerWasDealtAnAce()){
      // TODO:(str) switch to polished behavior * * * * *
      DelegateIfInControl(SetUpSpeakingBehavior("Good luck!"), 
                          &BehaviorBlackJack::TransitionToHitOrStandPrompt);
    }
  } else if(_dVars.game.PlayerHasBlackJack()){
    // Player got a BlackJack, but Victor could still tie
    // TODO:(str) switch to polished behavior * * * * *
    DelegateIfInControl(SetUpSpeakingBehavior("21! BlackJack!"), 
                        &BehaviorBlackJack::TransitionToVictorsTurn);
  } else if(_dVars.game.PlayerBusted()) {
    // Build the card value and bust string and action
    // TODO:(str) switch to polished behavior * * * * *
    std::string playerScoreString = std::to_string(_dVars.game.GetPlayerScore());
    playerScoreString += std::string(" . You busted!");
    _dVars.outcome = EOutcome::VictorWins;
    DelegateIfInControl(SetUpSpeakingBehavior(playerScoreString), 
                        &BehaviorBlackJack::TransitionToEndGame);
  } else {
    // Build the card value string and read out action
    // TODO:(str) switch to polished behavior * * * * *
    std::string playerScoreString("You have ");
    playerScoreString += std::to_string(_dVars.game.GetPlayerScore());
    DelegateIfInControl(SetUpSpeakingBehavior(playerScoreString),
                        &BehaviorBlackJack::TransitionToHitOrStandPrompt);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToHitOrStandPrompt()
{
  SET_STATE(HitOrStandPrompt);
  if(_iConfig.hitOrStandPromptBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.hitOrStandPromptBehavior.get(), &BehaviorBlackJack::TransitionToHitOrStand);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToHitOrStand()
{
  SET_STATE(HitOrStand);
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();

  //TODO:(str) Use BlackJack Intents * * * * *
  static const UserIntentTag playerHitIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag playerStandIntent = USER_INTENT(imperative_negative); 
  static const UserIntentTag quitIntent = USER_INTENT(cancel_timer);

  if(uic.IsUserIntentPending(playerHitIntent)){     
    uic.DropUserIntent(playerHitIntent);
    _dVars.game.PlayerHit();
    _dVars.visualizer.UpdateVisualization();

    // TODO:(str) switch to polished behavior * * * * *
    DelegateIfInControl(SetUpSpeakingBehavior("Ok, hit."),
      [this](){
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Deal),
                            &BehaviorBlackJack::TransitionToReactToPlayerCard);
      }
    );
  } else if (uic.IsUserIntentPending(quitIntent)){
    uic.DropUserIntent(quitIntent);
    TransitionToGetOut();
  } else {
    // Stand if:
    // 1. We received a valid playerStandIntent
    if (uic.IsUserIntentPending(playerStandIntent)) {
      uic.DropUserIntent(playerStandIntent);
    }

    // 2. We didn't receive any intents at all
    _dVars.visualizer.VisualizeSpread();
    // TODO:(str) switch to polished behavior * * * * *
    std::string standScoreString("Ok, Stand on ");
    standScoreString += std::to_string(_dVars.game.GetPlayerScore());
    DelegateIfInControl(SetUpSpeakingBehavior(standScoreString),
      [this](){
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Spread),
                            &BehaviorBlackJack::TransitionToVictorsTurn);
      }
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToVictorsTurn()
{
  SET_STATE(VictorsTurn);

  if(!_dVars.game.DealerHasFlopped()){
    _dVars.game.Flop();
    _dVars.visualizer.VisualizeFlop();
    TransitionToReactToDealerCard();
  } else {
    // The only reason Victor takes a turn after the flop is to hit
    _dVars.game.DealerHit();
    _dVars.visualizer.UpdateVisualization();
    // TODO:(str) switch to polished behavior * * * * *
    DelegateIfInControl(SetUpSpeakingBehavior("Dealer will hit."),
      [this](){
        DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Deal),
                            &BehaviorBlackJack::TransitionToReactToDealerCard);
      }
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToReactToDealerCard()
{
  SET_STATE(ReactToDealerCard);

  if(_dVars.game.DealerHasBlackJack()){
    if(_dVars.game.PlayerHasBlackJack()){
      // TODO:(str) switch to polished behavior * * * * *
      _dVars.outcome = EOutcome::Tie;
      DelegateIfInControl(SetUpSpeakingBehavior("Dealer got BlackJack too. Push."),
                          &BehaviorBlackJack::TransitionToEndGame);
    } else {
      // TODO:(str) switch to polished behavior * * * * *
      _dVars.outcome = EOutcome::VictorWinsBlackJack;
      DelegateIfInControl(SetUpSpeakingBehavior("Dealer got BlackJack. Dealer wins!"),
                          &BehaviorBlackJack::TransitionToEndGame);
    }
  } else if(_dVars.game.DealerBusted()){
    // Announce score and bust
    // TODO:(str) switch to polished behavior * * * * *
    _dVars.outcome = EOutcome::VictorLoses;
    std::string dealerScoreString("Dealer has ");
    dealerScoreString += std::to_string(_dVars.game.GetDealerScore());
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString),
      [this](){
        DelegateIfInControl(SetUpSpeakingBehavior("Dealer Busted."), 
                            &BehaviorBlackJack::TransitionToEndGame);
      }
    );

  } else if(_dVars.game.DealerHasLead()){
    // Announce score and lead
    // TODO:(str) switch to polished behavior * * * * *
    _dVars.outcome = EOutcome::VictorWins;
    std::string dealerScoreString("Dealer has ");
    dealerScoreString += std::to_string(_dVars.game.GetDealerScore());
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString),
      [this](){
        DelegateIfInControl(SetUpSpeakingBehavior("Dealer Wins!"),
                            &BehaviorBlackJack::TransitionToEndGame);
      }
    );

  } else {
    // Announce score and Hit again
    // TODO:(str) switch to polished behavior * * * * *
    std::string dealerScoreString("Dealer has ");
    dealerScoreString += std::to_string(_dVars.game.GetDealerScore());
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString), &BehaviorBlackJack::TransitionToVictorsTurn);
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToEndGame(){

  SET_STATE(EndGame);

  GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::BlackjackGameComplete);

  TriggerAnimationAction* endGameAction = nullptr;
  switch(_dVars.outcome){
    case EOutcome::Tie:
    {
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorPush);
      break;
    }
    case EOutcome::VictorWinsBlackJack:
    {
      GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::BlackjackDealerWon);
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorBlackJackWin);
      break;
    }
    case EOutcome::VictorWins:
    {
      GetBehaviorComp<RobotStatsTracker>().IncrementBehaviorStat(BehaviorStat::BlackjackDealerWon);
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorWin);
      break;
    }
    case EOutcome::VictorLosesBlackJack:
    {
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorBlackJackLose);
      break;
    }
    case EOutcome::VictorLoses:
    {
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorLose);
      break;
    }
  }

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Swipe),
    [this, endGameAction](){
      DelegateIfInControl(endGameAction, &BehaviorBlackJack::TransitionToPlayAgainPrompt);
    }
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToPlayAgainPrompt()
{
  SET_STATE(PlayAgainPrompt);
  if(_iConfig.playAgainPromptBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.playAgainPromptBehavior.get(), &BehaviorBlackJack::TransitionToPlayAgain);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToPlayAgain()
{
  SET_STATE(PlayAgain);
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();

  //TODO:(str) Use BlackJack Intents * * * * *
  static const UserIntentTag playAgainIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag dontPlayAgainIntent = USER_INTENT(imperative_negative); 
  static const UserIntentTag quitIntent = USER_INTENT(cancel_timer);

  if(uic.IsUserIntentPending(playAgainIntent)){     
    uic.DropUserIntent(playAgainIntent);
    OnBehaviorActivated();
  } else {

    if (uic.IsUserIntentPending(dontPlayAgainIntent)){
      uic.DropUserIntent(quitIntent);
    } else if(uic.IsUserIntentPending(quitIntent)){
      uic.DropUserIntent(quitIntent);
    }

    TransitionToGetOut();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToGetOut()
{
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Quit));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorBlackJack::SetUpSpeakingBehavior(const std::string& vocalizationString)
{
  _iConfig.ttsBehavior->SetTextToSay(vocalizationString);
  ANKI_VERIFY(_iConfig.ttsBehavior->WantsToBeActivated(),
              "BehaviorBlackjack.TTSError",
              "The TTSLoop behavior did not want to be activated, this indicates a usage error");
  return _iConfig.ttsBehavior.get();
}

} // namespace Cozmo
} // namespace Anki
