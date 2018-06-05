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

class Card
{ 
public:
  Card(int cardID, bool faceUp = true);

  const int GetRank() const;
  const std::string GetString() const;

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

  void Shuffle(Anki::Util::RandomGenerator* rng);
  int PopTopCard();

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

  bool DealCards();
  bool PlayerHit(bool dealing = false);
  bool DealerHit(bool faceUp = true);

  // Show the dealers face down card
  void Flop() {_dealerHand[0].SetFaceUp(true); _hasFlopped = true;}

  const bool PlayerBusted() const {return ScoreHand(_playerHand) > kBlackJackValue;}
  const bool PlayerHasBlackJack() const {return ScoreHand(_playerHand) == kBlackJackValue;}
  const bool PlayerWasDealtAnAce() const {return _playerDealtAce;}
  const bool PlayerHasHit() const {return _playerHasHit;}
  const bool PlayerHasFiveCardCharlie() const {return (_playerHand.size() >= 5) && (!PlayerBusted());}

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

private:

  Deck _deck;
  std::vector<Card> _playerHand;
  std::vector<Card> _dealerHand;
  bool              _playerDealtAce;
  bool              _playerHasHit;
  bool              _hasFlopped;

};

}
}

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackSimumation__
 