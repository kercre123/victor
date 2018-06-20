/**
 * File: BlackJackSimulation.cpp
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: BlackJack card game model and utility classes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackSimulation.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/random/randomIndexSampler.h"

namespace{
  const int kNumCardsPerSuit = 13;
  const int kNumCardsInDeck  = 52;
  const int kAceLowValue     = 1;
  const int kAceHighValue    = 11;

  static const char* ranks[] = 
  {
    "Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "Jack", "Queen", "King"
  };
  static const char* suits[] = 
  {
    "Spades", "Hearts", "Diamonds", "Clubs"
  };
}

namespace Anki {
namespace Cozmo {

#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Testing Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// For use in testing the BlackJack behavior. Due to the "5-card charlie rule", a game will consume a maximum of 10 cards
// from the deck, so specifying 10 cards specifies the maximal length game. Calling this function will ensure that the deck
// is initialized with the specified card order until "StopStackingDeck" is called, or a different sequence is specified.
// NOTE: This function is for test purposes only and DOES NOT ENFORCE RATIONAL COLLECTIONS OF CARDS. Duplicates are perfectly
// permissible.
void StackDeck(ConsoleFunctionContextRef context)
{
  std::vector<int> stackedDeckOrder;
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card1", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card2", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card3", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card4", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card5", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card6", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card7", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card8", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card9", kInvalidCardID));
  stackedDeckOrder.push_back(ConsoleArg_GetOptional_Int(context, "card10", kInvalidCardID));

  BlackJackGame::StackDeck(stackedDeckOrder);
}

CONSOLE_FUNC(StackDeck, "TestBlackJackBehavior.StackDeck(Up To Ten Comma Separated Integer Card ID's)",
             optional int card1, optional int card2, optional int card3, optional int card4, optional int card5,
             optional int card6, optional int card7, optional int card8, optional int card9, optional int card10);

void StopStackingDeck(ConsoleFunctionContextRef context)
{
  BlackJackGame::StopStackingDeck();
}

CONSOLE_FUNC(StopStackingDeck, "TestBlackJackBehavior.StopStackingDeck");
#endif // ANKI_DEV_CHEATS

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Card Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Card::Card(int cardID, bool faceUp)
: _cardID(cardID) 
, _faceUp(faceUp)
{
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const int Card::GetRank() const
{
  return (_cardID % kNumCardsPerSuit);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::string Card::GetString() const
{
  std::string nameString(ranks[GetRank()] + std::string(" of ") + suits[_cardID / kNumCardsPerSuit]);
  return nameString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Deck Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deck::Deck()
{
  _deck.reserve(kNumCardsInDeck);
  Init();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deck::Init(){
  // linear initialization
  for(int i = 0; i < 52; ++i){
    _deck[i] = i;
  }
  // Start back at the top
  _index = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deck::Shuffle(Anki::Util::RandomGenerator* rng)
{
  _deck = Anki::Util::RandomIndexSampler::CreateFullShuffledVector(*rng, kNumCardsInDeck);
  _index = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deck::Stack(const std::vector<int>* _stackedDeckVector)
{
  _deck = *_stackedDeckVector;
  _index = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Deck::PopTopCard()
{
  if(_index >= _deck.size()){
    PRINT_NAMED_WARNING("CardSimulation.EmptyDeck",
                        "Simulated deck of cards has been played out, reset and shuffle required");
    return kInvalidCardID;
  }

  return _deck[_index++];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Game Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

std::unique_ptr<std::vector<int>> BlackJackGame::_stackedDeck = nullptr;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlackJackGame::BlackJackGame()
: _lastCard(nullptr)
, _invalidCard(kInvalidCardID)
, _hasFlopped(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackGame::Init(Anki::Util::RandomGenerator* rng)
{
  if(nullptr != _stackedDeck){
    PRINT_NAMED_WARNING("BlackJackSimulation.Init.StackingDeck",
                        "Deck order was set from a console func, stacking deck per saved setting");
    _deck.Stack(_stackedDeck.get());
  } else {
    if(nullptr != rng){
      _deck.Shuffle(rng);
    } else {
      _deck.Init(); 
      PRINT_NAMED_WARNING("BlackJackSimulation.Init.NullRNG",
                          "Game was initialized without an RNG reference. Deck will not be shuffled.");
    }
  }

  _playerHand.clear();
  _dealerHand.clear();
  _hasFlopped = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool BlackJackGame::DealToPlayer(const bool faceUp)
{
  if(_playerHand.size() > kHandSizeLimit){
    PRINT_NAMED_ERROR("BlackJackGame.DealToPlayer.HandSizeLimitExceeded",
                      "Player hand size cannot exceed %d cards",
                      kHandSizeLimit);
    return false;
  }

  int topCardID = kInvalidCardID;
  topCardID = _deck.PopTopCard();
  if(kInvalidCardID != topCardID){
    _playerHand.emplace_back(topCardID);
    PRINT_NAMED_INFO("BlackJackGame.DealToPlayer",
                      "Player got the %s (CardID %d)",
                      _playerHand.back().GetString().c_str(),
                      topCardID);   
    _lastCard = &_playerHand.back();
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool BlackJackGame::DealToDealer(const bool faceUp)
{
  if(_dealerHand.size() > kHandSizeLimit){
    PRINT_NAMED_ERROR("BlackJackGame.DealToDealer.HandSizeLimitExceeded",
                      "Dealer hand size cannot exceed %d cards",
                      kHandSizeLimit);
    return false;
  }

  int topCardID = kInvalidCardID;
  topCardID = _deck.PopTopCard();
  if(kInvalidCardID != topCardID){
    _dealerHand.emplace_back(topCardID, faceUp);
    std::string faceUpString;
    faceUpString = faceUp ? "face up." : "face down.";
    PRINT_NAMED_INFO("BlackJackGame.DealToDealer",
                      "Dealer got the %s (CardID %d) %s",
                      _dealerHand.back().GetString().c_str(),
                      topCardID,
                      faceUpString.c_str());
    _lastCard = &_dealerHand.back();
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Card& BlackJackGame::LastCard()
{
  if(nullptr == _lastCard){
    PRINT_NAMED_ERROR("BlackJackSimulation.NullLastCard",
                      "LastCard() should not be called before DealTo_() has been called");
    return _invalidCard; 
  } else {
    return *_lastCard; 
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int BlackJackGame::ScoreCard(const Card& card)
{
  int val = card.GetRank() + 1;
  return (val > 10) ? 10 : val; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int BlackJackGame::ScoreHand(const std::vector<Card>& hand)
{
  int score = 0;
  bool handHasAce = false;
  for(const Card& card : hand){
    int cardValue = ScoreCard(card);
    // Watch for aces, account for them after scoring is finished.
    if(cardValue == kAceLowValue){
      handHasAce = true;
    }
    score += cardValue;
  }

  if(handHasAce && (score <= kAceHighValue)){
    // Swap one Ace to its high value. 
    // Doesn't matter if there's more than one ace, only one can ever be converted without busting.
    score += kAceHighValue - kAceLowValue;
  }

  PRINT_NAMED_INFO("BlackJackSimulation.ScoreHand",
                   "Hand has score of %d",
                   score);
  return score;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackGame::StackDeck(const std::vector<int>& stackedDeckOrder)
{
  std::vector<int> tempVec;
  for(auto& val : stackedDeckOrder){
    // Break when we reach the first unset card
    if(kInvalidCardID == val){
      break;
    }

    // Error out if we encounter an invalid CardID
    if(kNumCardsInDeck <= val || kInvalidCardID > val){
      tempVec.clear();
      break;
    }

    tempVec.push_back(val);
  }

  if(tempVec.size() == 0){
    PRINT_NAMED_WARNING("BlackJackSimulation.StackDeck.InvalidInput",
                        "StackDeck input should be a series of one to ten positive, comma separated integers in the range [0,51]");
    return;
  }

  _stackedDeck = std::make_unique<std::vector<int>>(std::move(tempVec));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackGame::StopStackingDeck()
{
  _stackedDeck = nullptr;
}

} // namespace Cozmo
} // namespace Anki
