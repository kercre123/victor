/**
 * File: BlackJackVisualizer.h
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: Class to encapsulate the View functionality of the BlackJack behavior. Displays
 *              appropriate card art and animations to Victor's display during BlackJack.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__

#include "coretech/vision/shared/spritePathMap.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace Anki{

// Fwd Declaration
namespace Vision{
class CompositeImage;
enum class SpriteBoxName : uint8_t;
}

namespace Vector{

// Fwd Declarations
class BehaviorExternalInterface;
class BlackJackGame;

class BlackJackVisualizer{
public:
  BlackJackVisualizer(const BlackJackGame* game);

  void Init(BehaviorExternalInterface& bei);

  // This should only be called by the BlackJack behavior from inside the BehaviorUpdate funtion
  // to allow actions in the visualizer that are synced with the behavior system update tick
  void Update(BehaviorExternalInterface& bei);

  void DealToPlayer(BehaviorExternalInterface& bei, std::function<void()> callback = nullptr);
  void DealToDealer(BehaviorExternalInterface& bei, std::function<void()> callback = nullptr);

  // TODO:(str) implement this later depending on whether we get dynamic layouts in time
  void SpreadPlayerCards(BehaviorExternalInterface& bei);

  void Flop(BehaviorExternalInterface& bei, std::function<void()> callback = nullptr);
  void DisplayCharlieFrame(BehaviorExternalInterface& bei, std::function<void()> callback = nullptr);
  void SwipeToClearFace(BehaviorExternalInterface& bei, std::function<void()> callback = nullptr);
  void ResetHands();

  // If the visualizer has control of parts of the robot (face track etc) this function
  // will release that control (probably becasue the behavior is ending) and clear out any lasting
  // state like callbacks and bools
  void ReleaseControlAndClearState(BehaviorExternalInterface& bei);


private:
  // No copy, no default construction
  BlackJackVisualizer();

  void DealCard(const BehaviorExternalInterface& bei,
                const Vision::SpriteBoxName& dealSeqSBName,
                const Vision::SpritePathMap::AssetID dealSpriteSeqName,
                const Vision::SpriteBoxName& revealSBName,
                const Vision::SpriteBoxName& finalSBName,
                const Vision::SpritePathMap::AssetID cardSpriteName);

  const BlackJackGame* _game;

  using AnimRemapMap = std::unordered_map<Vision::SpriteBoxName, Vision::SpritePathMap::AssetID>;
  AnimRemapMap _cardAssetMap;

  uint32_t _dealCardSeqApplyAt_ms = 0;
  uint32_t _displayDealtCardsAt_ms = 0;
  uint32_t _clearCardsDuringSwipeAt_ms = 0;

  bool                  _shouldClearLocksOnCallback = false;
  bool                  _animCompletedLastFrame = false;
  std::function<void()> _animCompletedCallback = nullptr;
};

} //namespace Vector
} //namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__
