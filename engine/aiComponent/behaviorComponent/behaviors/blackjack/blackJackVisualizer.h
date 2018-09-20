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

#include "coretech/vision/shared/compositeImage/compositeImage.h"

#include <memory>

namespace Anki{

// Fwd Declaration
namespace Vision{
class CompositeImage;
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
  void ClearCards(BehaviorExternalInterface& bei, uint32_t applyAt_ms = 0);

  // If the visualizer has control of parts of the robot (face track etc) this function
  // will release that control (probably becasue the behavior is ending) and clear out any lasting
  // state like callbacks and bools
  void ReleaseControlAndClearState(BehaviorExternalInterface& bei);


private:
  // No copy, no default construction
  BlackJackVisualizer();

  void PlayCompositeCardAnimationAndLock(const BehaviorExternalInterface& bei,
                                         const char*                      compAnimName,
                                         const Vision::LayerName&         layerName,
                                         const Vision::SpriteBoxName&     spriteBoxName,
                                         const Vision::SpriteName&        cardImageName,
                                         const uint                       showDealtCardAt_ms,
                                         const Vision::SpriteName&        cardAnimSeqName = Vision::SpriteName::Count,
                                         const uint                       applyCardSeqAt_ms = 0);

  const BlackJackGame* _game;
  std::unique_ptr<Vision::CompositeImage> _compImg;

  uint _dealCardSeqApplyAt_ms = 0;
  uint _displayDealtCardsAt_ms = 0;
  uint _clearCardsDuringSwipeAt_ms = 0;

  bool                  _shouldClearLocksOnCallback = false;
  bool                  _animCompletedLastFrame = false;
  std::function<void()> _animCompletedCallback = nullptr;
};

} //namespace Vector
} //namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__
