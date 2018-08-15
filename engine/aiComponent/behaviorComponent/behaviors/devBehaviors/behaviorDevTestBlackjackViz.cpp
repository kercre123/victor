/**
 * File: BehaviorDevTestBlackjackViz.cpp
 *
 * Author: Sam Russell
 * Created: 2018-06-04
 *
 * Description: Simple input point for testing out all of the BlackjackVisualizer functionality
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevTestBlackjackViz.h"

#include "clad/types/compositeImageTypes.h"
#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/shared/spriteCache/iSpriteWrapper.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "engine/actions/animActions.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

namespace{
Anki::Vector::BehaviorDevTestBlackjackViz* sThis = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DealToPlayer(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->DealToPlayer();
  }
}

CONSOLE_FUNC(DealToPlayer, "BlackJackTestViz.DealToPlayer");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DealToDealer(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->DealToDealer();
  }
}

CONSOLE_FUNC(DealToDealer, "BlackJackTestViz.DealToDealer");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Flop(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->Flop();
  }
}

CONSOLE_FUNC(Flop, "BlackJackTestViz.Flop");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpreadPlayerCards(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->SpreadPlayerCards();
  }
}

CONSOLE_FUNC(SpreadPlayerCards, "BlackJackTestViz.SpreadPlayerCards");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Charlie(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->Charlie();
  }
}

CONSOLE_FUNC(Charlie, "BlackJackTestViz.Charlie");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ClearCardsToSortedDeck(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->ClearCards();
  }
}

CONSOLE_FUNC(ClearCardsToSortedDeck, "BlackJackTestViz.ClearCardsToSortedDeck");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ClearCardsAndShuffle(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->ClearCards(true);
  }
}

CONSOLE_FUNC(ClearCardsAndShuffle, "BlackJackTestViz.ClearCardsAndShuffle");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnnouncePlayerScore(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->AnnouncePlayerScore();
  }
}

CONSOLE_FUNC(AnnouncePlayerScore, "BlackJackTestViz.AnnouncePlayerScore");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnnounceDealerScore(ConsoleFunctionContextRef context)
{
  if(nullptr != sThis){
    sThis->AnnounceDealerScore();
  }
}

CONSOLE_FUNC(AnnounceDealerScore, "BlackJackTestViz.AnnounceDealerScore");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestBlackjackViz::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestBlackjackViz::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestBlackjackViz::BehaviorDevTestBlackjackViz(const Json::Value& config)
 : ICozmoBehavior(config)
 , _game()
 , _visualizer(&_game)
{
  sThis = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestBlackjackViz::~BehaviorDevTestBlackjackViz()
{
  sThis = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevTestBlackjackViz::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  _game.Init(&GetRNG());
  _visualizer.Init(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::BehaviorUpdate() 
{
  if( IsActivated() ) {
    _visualizer.Update(GetBEI());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::DealToPlayer()
{
  _game.DealToPlayer();
  _visualizer.DealToPlayer(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::DealToDealer()
{
  _game.DealToDealer(!_dVars.firstDealerCard);
  _visualizer.DealToDealer(GetBEI());
  _dVars.firstDealerCard = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::Flop()
{
  _game.Flop();
  _visualizer.Flop(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::SpreadPlayerCards()
{
  _visualizer.SpreadPlayerCards(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::Charlie()
{
  _visualizer.DisplayCharlieFrame(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::ClearCards(const bool shuffle)
{
  if(shuffle){
    _game.Init(&GetRNG());
  } else {
    _game.Init();
  }
  _visualizer.ClearCards(GetBEI());
  _dVars.firstDealerCard = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::AnnouncePlayerScore()
{
  _game.GetPlayerScore();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestBlackjackViz::AnnounceDealerScore()
{
  _game.GetDealerScore();
}

} // namespace Vector
} // namespace Anki
