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

#include "util/logging/logging.h"
#include "util/random/randomIndexSampler.h"

namespace{
  const int kNumCardsPerSuit = 13;
  const int kNumCardsInDeck  = 52;
  const int kAceRank         = 0;
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
  std::string nameString;
  nameString += ranks[ _cardID % kNumCardsPerSuit];
  nameString += " of ";
  nameString += suits[ _cardID / kNumCardsPerSuit];

  return nameString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Deck Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deck::Deck()
: _index(0)
{
  // linear initialization
  _deck.reserve(kNumCardsInDeck);
  for(int i = 0; i < 52; ++i){
    _deck[i] = i;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Deck::Shuffle(Anki::Util::RandomGenerator* rng)
{
  _deck = Anki::Util::RandomIndexSampler::CreateFullShuffledVector(*rng, kNumCardsInDeck);
  _index = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Deck::PopTopCard()
{
  if(_index >= kNumCardsInDeck){
    PRINT_NAMED_WARNING("CardSimulation.EmptyDeck",
                        "Simulated deck of cards has been played out, reset and shuffle required");
    return -1;
  }

  return _deck[_index++];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Game Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlackJackGame::BlackJackGame()
: _playerDealtAce(false)
, _playerHasHit(false)
, _hasFlopped(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackGame::Init(Anki::Util::RandomGenerator* rng)
{
  if(nullptr != rng){
    _deck.Shuffle(rng);
  } else {
    PRINT_NAMED_WARNING("BlackJackSimulation.NullRNG",
                        "Game was initialized without an RNG reference. Deck will not be shuffled.");
  }

  _playerHand.clear();
  _dealerHand.clear();
  _hasFlopped = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlackJackGame::DealCards()
{
  const bool kIsDealing = true;
  const bool kDealFaceDown = false;

  // Deal first card to Player
  if(!PlayerHit(kIsDealing)){
    return false;
  }

  // Deal face down to Dealer
  if(!DealerHit(kDealFaceDown)){
    return false;
  }

  // Deal second card to Player
  if(!PlayerHit(kIsDealing)){
    return false;
  }

  // Face Up
  if(!DealerHit()){
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlackJackGame::PlayerHit(bool dealing)
{
  _playerHasHit = true;

  int topCardID = -1;
  topCardID = _deck.PopTopCard();
  if(topCardID){
    _playerHand.emplace_back(topCardID);
    PRINT_NAMED_INFO("BlackJackGame.PlayerHit",
                      "Player got the %s",
                      _playerHand.back().GetString().c_str());   
    if(dealing && kAceRank == topCardID){
      _playerDealtAce = true;
    }
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlackJackGame::DealerHit(bool faceUp)
{
  int topCardID = -1;
  topCardID = _deck.PopTopCard();
  if(topCardID){
    _dealerHand.emplace_back(topCardID, faceUp);
    std::string faceUpString;
    faceUpString = faceUp ? "face up." : "face down.";
    PRINT_NAMED_INFO("BlackJackGame.DealerHit",
                      "Dealer got the %s %s",
                      _dealerHand.back().GetString().c_str(),
                      faceUpString.c_str());
    return true;
  }

  return false;
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

  return score;
}

} // namespace Cozmo
} // namespace Anki