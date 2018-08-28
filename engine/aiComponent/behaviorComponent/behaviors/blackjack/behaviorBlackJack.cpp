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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorLookAtFaceInFront.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/clad/types/animationTypes.h"

#define SET_STATE(s) do{ \
                          _dVars.state = EState::s; \
                          PRINT_NAMED_INFO("BehaviorBlackJack.State", "State = %s", #s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
  const bool kFaceUp = true;
  const bool kFaceDown = false;

  static const UserIntentTag affirmativeIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag negativeIntent = USER_INTENT(imperative_negative);
  static const UserIntentTag playerHitIntent = USER_INTENT(blackjack_hit);
  static const UserIntentTag playerStandIntent = USER_INTENT(blackjack_stand);
  static const UserIntentTag playAgainIntent = USER_INTENT(blackjack_playagain);
}  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::DynamicVariables::DynamicVariables()
: state(EState::TurnToFace)
, dealingState(EDealingState::PlayerFirstCard)
, outcome(EOutcome::Tie)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::BehaviorBlackJack(const Json::Value& config)
 : ICozmoBehavior(config)
 , _game()
 , _visualizer(&_game)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBlackJack::~BehaviorBlackJack()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.hitOrStandPromptBehavior.get());
  delegates.insert(_iConfig.playAgainPromptBehavior.get());
  delegates.insert(_iConfig.ttsBehavior.get());
  delegates.insert(_iConfig.lookAtFaceInFrontBehavior.get());
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

  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(BlackJackLookAtFaceInFront),
                                  BEHAVIOR_CLASS(LookAtFaceInFront),
                                  _iConfig.lookAtFaceInFrontBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  _game.Init(&GetRNG());
  _visualizer.Init(GetBEI());

  TransitionToTurnToFace();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::OnBehaviorDeactivated()
{
  _visualizer.ReleaseControlAndClearState(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  _visualizer.Update(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToTurnToFace()
{
  SET_STATE(TurnToFace);
  if(_iConfig.lookAtFaceInFrontBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.lookAtFaceInFrontBehavior.get(), &BehaviorBlackJack::TransitionToGetIn);
  } else {
    TransitionToGetIn();
  }
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
  switch(_dVars.dealingState){
    case EDealingState::PlayerFirstCard:
    {
      _game.DealToPlayer(kFaceUp);
      _visualizer.DealToPlayer(GetBEI(),[this]()
        {
          _dVars.dealingState = EDealingState::DealerFirstCard;
          // keep an eye out for Aces
          if(_game.LastCard().IsAnAce()){
            DelegateIfInControl(SetUpSpeakingBehavior("Good luck!"),
                                &BehaviorBlackJack::TransitionToDealing);
          } else {
            TransitionToDealing();
          }
        });
      break;
    }
    case EDealingState::DealerFirstCard:
    {
      _game.DealToDealer(kFaceDown);
      _visualizer.DealToDealer(GetBEI(), [this]()
        {
          _dVars.dealingState = EDealingState::PlayerSecondCard;
          TransitionToDealing();
        });
      break;
    }
    case EDealingState::PlayerSecondCard:
    {
      _game.DealToPlayer(kFaceUp);
      _visualizer.DealToPlayer(GetBEI(), [this]()
        {
          _dVars.dealingState = EDealingState::DealerSecondCard;
          // Keep an eye out for natural BlackJack
          if(_game.PlayerHasBlackJack()){
            _dVars.outcome = EOutcome::VictorLosesBlackJack;
            DelegateIfInControl(SetUpSpeakingBehavior("BlackJack!"),
                                &BehaviorBlackJack::TransitionToEndGame);

          } else {
            TransitionToDealing();
          }
        });
      break;
    }
    case EDealingState::DealerSecondCard:
    {
      _game.DealToDealer(kFaceUp);
      _visualizer.DealToDealer(GetBEI(), [this]()
        {
          _dVars.dealingState = EDealingState::Finished;
          // Respond to BlackJack for the player
          if(_game.PlayerHasBlackJack()){
            _dVars.outcome = EOutcome::VictorLosesBlackJack;
            DelegateIfInControl(SetUpSpeakingBehavior("BlackJack, You win!"),
                                &BehaviorBlackJack::TransitionToEndGame);
          } else {
            TransitionToReactToPlayerCard();
          }
        });
      break;
    }
    case EDealingState::Finished:
    {
      PRINT_NAMED_ERROR("BehaviorBlackJack.InvalidDealingState",
                        "Should never enter TransitionToDealing() when DealingState is: Finished");
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToReactToPlayerCard()
{
  SET_STATE(ReactToPlayerCard);
  if(_game.PlayerBusted()) {
    // Build the card value and bust string and action
    std::string playerScoreString(std::to_string(_game.GetPlayerScore()) + ". You busted!");
    _dVars.outcome = EOutcome::VictorWins;
    DelegateIfInControl(SetUpSpeakingBehavior(playerScoreString), 
                        &BehaviorBlackJack::TransitionToEndGame);
  } else if(_game.PlayerHasCharlie()) {
    std::string scoreString(std::to_string(_game.GetPlayerScore()));
    DelegateIfInControl(SetUpSpeakingBehavior(scoreString), [this]()
      {
        _visualizer.DisplayCharlieFrame(GetBEI(), [this]()
          {
            _dVars.outcome = EOutcome::VictorLosesBlackJack;
            DelegateIfInControl(SetUpSpeakingBehavior("5 Card Charlie! You win!"), &BehaviorBlackJack::TransitionToEndGame);
          }
        );
      }
    );
  }else if(_game.PlayerHasBlackJack()){
    // Player got a BlackJack, but Victor could still tie
    DelegateIfInControl(SetUpSpeakingBehavior("21!"), 
                        &BehaviorBlackJack::TransitionToVictorsTurn);
  } else {
    // Build the card value string and read out action
    std::string playerScoreString(std::to_string(_game.GetPlayerScore()));
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

  if(uic.IsUserIntentPending(playerHitIntent)){
    uic.DropUserIntent(playerHitIntent);
    _game.DealToPlayer();
    _visualizer.DealToPlayer(GetBEI(), std::bind(&BehaviorBlackJack::TransitionToReactToPlayerCard, this));
  } else {
    // Stand if:
    // 1. We received a valid playerStandIntent
    if (uic.IsUserIntentPending(playerStandIntent)) {
      uic.DropUserIntent(playerStandIntent);
    }

    // 2. We didn't receive any intents at all
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Response),
      [this](){
        TransitionToVictorsTurn();
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToVictorsTurn()
{
  SET_STATE(VictorsTurn);

  // TODO:(str) work out whether a "spread" is happening or not, dependent on dynamic layouts
  // _visualizer.VisualizeSpread();

  DelegateIfInControl(SetUpSpeakingBehavior("Dealers Turn!"),
    [this](){
      _game.Flop();
      _visualizer.Flop(GetBEI(), std::bind(&BehaviorBlackJack::TransitionToDealToVictor, this));
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToDealToVictor()
{
  SET_STATE(DealToVictor);

  _game.DealToDealer();
  _visualizer.DealToDealer(GetBEI(), std::bind(&BehaviorBlackJack::TransitionToReactToDealerCard, this));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToReactToDealerCard()
{
  SET_STATE(ReactToDealerCard);

  if(_game.DealerBusted()){
    // Announce score and bust
    _dVars.outcome = EOutcome::VictorBusts;
    std::string dealerScoreString(std::to_string(_game.GetDealerScore()) + ". Dealer Busted.");
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString), &BehaviorBlackJack::TransitionToEndGame);
  } else if(_game.DealerTied()){
    _dVars.outcome = EOutcome::Tie;
    std::string tieString(std::to_string(_game.GetDealerScore()) + ". Push." );
    DelegateIfInControl(SetUpSpeakingBehavior(tieString),
                        &BehaviorBlackJack::TransitionToEndGame);
  } else if(_game.DealerHasBlackJack()){
    _dVars.outcome = EOutcome::VictorWinsBlackJack;
    DelegateIfInControl(SetUpSpeakingBehavior("21!"),
                        &BehaviorBlackJack::TransitionToEndGame);
  } else if(_game.DealerHasWon()){
    _dVars.outcome = EOutcome::VictorWins;
    std::string dealerScoreString(std::to_string(_game.GetDealerScore()) + ". Dealer Wins!");
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString), &BehaviorBlackJack::TransitionToEndGame);
  } else if(_game.DealerHasTooManyCards() || _game.DealerShouldStandPerVegasRules()) {
    std::string dealerScoreString(std::to_string(_game.GetDealerScore()) + ". You win!" );
    _dVars.outcome = EOutcome::VictorLoses;
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString),
                        &BehaviorBlackJack::TransitionToEndGame);
  } else {
    // Announce score and Hit again
    std::string dealerScoreString( std::to_string(_game.GetDealerScore()) );
    DelegateIfInControl(SetUpSpeakingBehavior(dealerScoreString), &BehaviorBlackJack::TransitionToDealToVictor);
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
    case EOutcome::VictorBusts:
    {
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorBust);
      break;
    }
    case EOutcome::VictorLoses:
    {
      endGameAction = new TriggerAnimationAction(AnimationTrigger::BlackJack_VictorLose);
      break;
    }
  }

  _visualizer.SwipeToClearFace(GetBEI(),
    [this, endGameAction](){
      DelegateIfInControl(endGameAction, &BehaviorBlackJack::TransitionToPlayAgainPrompt);
    });
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

  if(uic.IsUserIntentPending(playAgainIntent)){     
    uic.DropUserIntent(playAgainIntent);
    OnBehaviorActivated();
  } else if(uic.IsUserIntentPending(affirmativeIntent)){
    uic.DropUserIntent(affirmativeIntent);
    OnBehaviorActivated();
  } else {
    if (uic.IsUserIntentPending(negativeIntent)){
      uic.DropUserIntent(negativeIntent);
    }
    TransitionToGetOut();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBlackJack::TransitionToGetOut()
{
  _visualizer.ClearCards(GetBEI());
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::BlackJack_Quit), [this]()
    {
      CancelSelf();
    }
  );
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

} // namespace Vector
} // namespace Anki
