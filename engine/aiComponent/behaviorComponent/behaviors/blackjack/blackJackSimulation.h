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
namespace Cozmo{

const int kInvalidCardID = -1;
const int kAceRank       = 0;
const int kHandSizeLimit   = 5;

class Card
{ 
public:
  Card(int cardID, bool faceUp = true);

  const int GetID() const {return _cardID;}
  const int GetRank() const;
  const std::string GetString() const;

  const bool IsAnAce() const {return kAceRank == GetRank();}
  const bool IsFaceUp() const {return _faceUp;}
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
  static const int kBlackJackValue = 21;

  BlackJackGame();

  void Init(Anki::Util::RandomGenerator* rng = nullptr);

  const bool DealToPlayer(const bool faceUp = true);
  const bool DealToDealer(const bool faceUp = true);

  // Show the dealers face down card
  void Flop() {_dealerHand[0].SetFaceUp(true); _hasFlopped = true;}

  const Card& LastCard();

  const bool PlayerBusted() const {return ScoreHand(_playerHand) > kBlackJackValue;}
  const bool PlayerHasBlackJack() const {return ScoreHand(_playerHand) == kBlackJackValue;}
  const bool PlayerHasCharlie() const {return (_playerHand.size() >= kHandSizeLimit) && (!PlayerBusted());}

  const bool DealerHasFlopped() const {return _hasFlopped;}
  const bool DealerBusted() const {return ScoreHand(_dealerHand) > kBlackJackValue;}
  const bool DealerHasBlackJack() const {return ScoreHand(_dealerHand) == kBlackJackValue;}
  const bool DealerHasLead() const {return ScoreHand(_dealerHand) > ScoreHand(_playerHand);}

  const std::vector<Card>& GetPlayerHand() const {return _playerHand;}
  const std::vector<Card>& GetDealerHand() const {return _dealerHand;}

  const int GetPlayerScore(){return ScoreHand(_playerHand);}
  const int GetDealerScore(){return ScoreHand(_dealerHand);}

  static int ScoreCard(const Card& card);
  static int ScoreHand(const std::vector<Card>& hand);

  // Debug/Test
  static void StackDeck(const std::vector<int>& stackedDeckOrder);
  static void StopStackingDeck();

private:

  Deck _deck;
  std::vector<Card> _playerHand;
  std::vector<Card> _dealerHand;
  Card*             _lastCard;
  Card              _invalidCard;
  bool              _hasFlopped;

  // Debug/Test
  static std::unique_ptr<std::vector<int>> _stackedDeck;
};

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackSimumation__
 