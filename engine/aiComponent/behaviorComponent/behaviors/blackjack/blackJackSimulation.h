/**
 * File: BlackJackSimulation.h
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: BlackJack card game model and utility classes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackSimumation__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackSimumation__

#include <string>
#include <vector>

// Fwd Declarations
namespace Anki{
namespace Util{
  class RandomGenerator;
}
}

namespace Anki{
namespace Vector{

const int kInvalidCardID = -1;
const int kAceRank       = 0;
const int kHandSizeLimit   = 5;

class Card
{ 
public:
  Card(int cardID, bool faceUp = true);

  int GetID() const {return _cardID;}
  int GetRank() const;
  std::string GetString() const;

  bool IsAnAce() const {return kAceRank == GetRank();}
  bool IsFaceUp() const {return _faceUp;}
  void SetFaceUp(bool faceUp) {_faceUp = faceUp;}

private:
  Card(); //Don't allow default construction. Cards should always have an identity

  int _cardID;
  bool _faceUp;

};

class Deck
{
public:
  Deck();

  void Init();
  void Shuffle(Anki::Util::RandomGenerator* rng);
  int PopTopCard();

  // Debug/Test
  void Stack(const std::vector<int>* deckOrder);

private:
  std::vector<int>  _deck;
  int               _index; // current card in dealing order

};

class BlackJackGame
{
public:
  static const int kVegasRulesLimit = 17;
  static const int kBlackJackValue = 21;

  BlackJackGame();

  void Init(Anki::Util::RandomGenerator* rng = nullptr);

  bool DealToPlayer(const bool faceUp = true);
  bool DealToDealer(const bool faceUp = true);

  // Show the dealers face down card
  void Flop() {_dealerHand[0].SetFaceUp(true); _hasFlopped = true;}

  const Card& LastCard();

  const std::vector<Card>& GetPlayerHand() const {return _playerHand;}
  const std::vector<Card>& GetDealerHand() const {return _dealerHand;}

  int GetPlayerScore() const {return _playerScore;}
  int GetDealerScore() const {return _dealerScore;}

  bool PlayerBusted() const {return _playerScore > kBlackJackValue;}
  bool PlayerHasBlackJack() const {return _playerScore == kBlackJackValue;}
  bool PlayerHasCharlie() const {return (_playerHand.size() >= kHandSizeLimit) && (!PlayerBusted());}

  bool DealerHasFlopped() const {return _hasFlopped;}
  bool DealerBusted() const {return _dealerScore > kBlackJackValue;}
  bool DealerHasBlackJack() const {return _dealerScore == kBlackJackValue;}
  bool DealerHasTooManyCards() const {return _dealerHand.size() >= kHandSizeLimit;}
  bool DealerShouldStandPerVegasRules() const {return _dealerScore >= kVegasRulesLimit;}
  bool DealerTied() const {
    return (DealerHasTooManyCards() || DealerShouldStandPerVegasRules()) && (_dealerScore == _playerScore);
  }
  bool DealerHasWon() const {
    return (DealerHasTooManyCards() || DealerShouldStandPerVegasRules()) && (_dealerScore > _playerScore);
  }
  bool DealerHasLost() const {
    return (DealerHasTooManyCards() || DealerShouldStandPerVegasRules()) && (_dealerScore < _playerScore);
  }

  // Debug/Test
  static void StackDeck(const std::vector<int>& stackedDeckOrder);
  static void StopStackingDeck();

private:
  int ScoreCard(const Card& card) const;
  // Returns true if an Ace was found during scoring
  bool ScoreHandAndReportAces(const std::vector<Card>& hand, int& outScore);
  void ScorePlayerHand();
  void ScoreDealerHand();

  Deck _deck;
  std::vector<Card> _playerHand;
  std::vector<Card> _dealerHand;
  int               _playerScore;
  int               _dealerScore;
  Card*             _lastCard;
  Card              _invalidCard;
  bool              _hasFlopped;

  // Debug/Test
  static std::unique_ptr<std::vector<int>> _stackedDeck;
};

} // namespace Vector
} // namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackSimumation__
 
