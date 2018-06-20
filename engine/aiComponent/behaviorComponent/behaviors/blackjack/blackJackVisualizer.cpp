/**
 * File: BlackJackVisualizer.cpp
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

#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackVisualizer.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/proceduralFace/proceduralFace.h"
#include "clad/types/compositeImageTypes.h"
#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/shared/spriteCache/iSpriteWrapper.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackSimulation.h"
#include "engine/clad/types/animationTypes.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dataAccessorComponent.h"

namespace Anki{
namespace Cozmo{

namespace{
// LayerLookup
const std::vector<Vision::LayerName> kPlayerCardLayers = {
  Vision::LayerName::Player_1,
  Vision::LayerName::Player_2,
  Vision::LayerName::Player_3,
  Vision::LayerName::Player_4,
  Vision::LayerName::Player_5
};
const std::vector<Vision::LayerName> kDealerCardLayers = {
  Vision::LayerName::Dealer_1,
  Vision::LayerName::Dealer_2,
  Vision::LayerName::Dealer_3,
  Vision::LayerName::Dealer_4,
  Vision::LayerName::Dealer_5
};
// SpriteBoxName lookup
const std::vector<Vision::SpriteBoxName> kPlayerCardSlots = {
  Vision::SpriteBoxName::PlayerCardSlot_1,
  Vision::SpriteBoxName::PlayerCardSlot_2,
  Vision::SpriteBoxName::PlayerCardSlot_3,
  Vision::SpriteBoxName::PlayerCardSlot_4,
  Vision::SpriteBoxName::PlayerCardSlot_5
};
const std::vector<Vision::SpriteBoxName> kDealerCardSlots = {
  Vision::SpriteBoxName::DealerCardSlot_1,
  Vision::SpriteBoxName::DealerCardSlot_2,
  Vision::SpriteBoxName::DealerCardSlot_3,
  Vision::SpriteBoxName::DealerCardSlot_4,
  Vision::SpriteBoxName::DealerCardSlot_5
};
// CardID to AssetName lookup
const std::vector<Vision::SpriteName> kPlayerCardAssets = {
  // Spades
  Vision::SpriteName::AceOfSpades_Player,
  Vision::SpriteName::TwoOfSpades_Player,
  Vision::SpriteName::ThreeOfSpades_Player,
  Vision::SpriteName::FourOfSpades_Player,
  Vision::SpriteName::FiveOfSpades_Player,
  Vision::SpriteName::SixOfSpades_Player,
  Vision::SpriteName::SevenOfSpades_Player,
  Vision::SpriteName::EightOfSpades_Player,
  Vision::SpriteName::NineOfSpades_Player,
  Vision::SpriteName::TenOfSpades_Player,
  Vision::SpriteName::JackOfSpades_Player,
  Vision::SpriteName::QueenOfSpades_Player,
  Vision::SpriteName::KingOfSpades_Player,
  // Hearts
  Vision::SpriteName::AceOfHearts_Player,
  Vision::SpriteName::TwoOfHearts_Player,
  Vision::SpriteName::ThreeOfHearts_Player,
  Vision::SpriteName::FourOfHearts_Player,
  Vision::SpriteName::FiveOfHearts_Player,
  Vision::SpriteName::SixOfHearts_Player,
  Vision::SpriteName::SevenOfHearts_Player,
  Vision::SpriteName::EightOfHearts_Player,
  Vision::SpriteName::NineOfHearts_Player,
  Vision::SpriteName::TenOfHearts_Player,
  Vision::SpriteName::JackOfHearts_Player,
  Vision::SpriteName::QueenOfHearts_Player,
  Vision::SpriteName::KingOfHearts_Player,
  // Diamonds
  Vision::SpriteName::AceOfDiamonds_Player,
  Vision::SpriteName::TwoOfDiamonds_Player,
  Vision::SpriteName::ThreeOfDiamonds_Player,
  Vision::SpriteName::FourOfDiamonds_Player,
  Vision::SpriteName::FiveOfDiamonds_Player,
  Vision::SpriteName::SixOfDiamonds_Player,
  Vision::SpriteName::SevenOfDiamonds_Player,
  Vision::SpriteName::EightOfDiamonds_Player,
  Vision::SpriteName::NineOfDiamonds_Player,
  Vision::SpriteName::TenOfDiamonds_Player,
  Vision::SpriteName::JackOfDiamonds_Player,
  Vision::SpriteName::QueenOfDiamonds_Player,
  Vision::SpriteName::KingOfDiamonds_Player,
  // Clubs
  Vision::SpriteName::AceOfClubs_Player,
  Vision::SpriteName::TwoOfClubs_Player,
  Vision::SpriteName::ThreeOfClubs_Player,
  Vision::SpriteName::FourOfClubs_Player,
  Vision::SpriteName::FiveOfClubs_Player,
  Vision::SpriteName::SixOfClubs_Player,
  Vision::SpriteName::SevenOfClubs_Player,
  Vision::SpriteName::EightOfClubs_Player,
  Vision::SpriteName::NineOfClubs_Player,
  Vision::SpriteName::TenOfClubs_Player,
  Vision::SpriteName::JackOfClubs_Player,
  Vision::SpriteName::QueenOfClubs_Player,
  Vision::SpriteName::KingOfClubs_Player
};

const std::vector<Vision::SpriteName> kDealerCardAssets = {
  // Spades
  Vision::SpriteName::AceOfSpades_Dealer,
  Vision::SpriteName::TwoOfSpades_Dealer,
  Vision::SpriteName::ThreeOfSpades_Dealer,
  Vision::SpriteName::FourOfSpades_Dealer,
  Vision::SpriteName::FiveOfSpades_Dealer,
  Vision::SpriteName::SixOfSpades_Dealer,
  Vision::SpriteName::SevenOfSpades_Dealer,
  Vision::SpriteName::EightOfSpades_Dealer,
  Vision::SpriteName::NineOfSpades_Dealer,
  Vision::SpriteName::TenOfSpades_Dealer,
  Vision::SpriteName::JackOfSpades_Dealer,
  Vision::SpriteName::QueenOfSpades_Dealer,
  Vision::SpriteName::KingOfSpades_Dealer,
  // Hearts
  Vision::SpriteName::AceOfHearts_Dealer,
  Vision::SpriteName::TwoOfHearts_Dealer,
  Vision::SpriteName::ThreeOfHearts_Dealer,
  Vision::SpriteName::FourOfHearts_Dealer,
  Vision::SpriteName::FiveOfHearts_Dealer,
  Vision::SpriteName::SixOfHearts_Dealer,
  Vision::SpriteName::SevenOfHearts_Dealer,
  Vision::SpriteName::EightOfHearts_Dealer,
  Vision::SpriteName::NineOfHearts_Dealer,
  Vision::SpriteName::TenOfHearts_Dealer,
  Vision::SpriteName::JackOfHearts_Dealer,
  Vision::SpriteName::QueenOfHearts_Dealer,
  Vision::SpriteName::KingOfHearts_Dealer,
  // Diamonds
  Vision::SpriteName::AceOfDiamonds_Dealer,
  Vision::SpriteName::TwoOfDiamonds_Dealer,
  Vision::SpriteName::ThreeOfDiamonds_Dealer,
  Vision::SpriteName::FourOfDiamonds_Dealer,
  Vision::SpriteName::FiveOfDiamonds_Dealer,
  Vision::SpriteName::SixOfDiamonds_Dealer,
  Vision::SpriteName::SevenOfDiamonds_Dealer,
  Vision::SpriteName::EightOfDiamonds_Dealer,
  Vision::SpriteName::NineOfDiamonds_Dealer,
  Vision::SpriteName::TenOfDiamonds_Dealer,
  Vision::SpriteName::JackOfDiamonds_Dealer,
  Vision::SpriteName::QueenOfDiamonds_Dealer,
  Vision::SpriteName::KingOfDiamonds_Dealer,
  // Clubs
  Vision::SpriteName::AceOfClubs_Dealer,
  Vision::SpriteName::TwoOfClubs_Dealer,
  Vision::SpriteName::ThreeOfClubs_Dealer,
  Vision::SpriteName::FourOfClubs_Dealer,
  Vision::SpriteName::FiveOfClubs_Dealer,
  Vision::SpriteName::SixOfClubs_Dealer,
  Vision::SpriteName::SevenOfClubs_Dealer,
  Vision::SpriteName::EightOfClubs_Dealer,
  Vision::SpriteName::NineOfClubs_Dealer,
  Vision::SpriteName::TenOfClubs_Dealer,
  Vision::SpriteName::JackOfClubs_Dealer,
  Vision::SpriteName::QueenOfClubs_Dealer,
  Vision::SpriteName::KingOfClubs_Dealer
};

const char* kDealAnimationName = "anim_blackjack_deal_01";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlackJackVisualizer::BlackJackVisualizer(const BlackJackGame* game)
: _game(game)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::Init(BehaviorExternalInterface& bei)
{
  auto& dataAccessorComp = bei.GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();

  // Find the time stamps for animation driven events
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if(nullptr != animContainer){
    const Animation* animPtr = animContainer->GetAnimation(kDealAnimationName);
    if(nullptr == animPtr){
      PRINT_NAMED_ERROR("BlackJackVisualizer.Init.AnimationNotFoundInContainer",
                        "Animations need to be manually loaded on engine side - %s is not",
                        kDealAnimationName);
    } else {
      const auto& track = animPtr->GetTrack<EventKeyFrame>();
      _dealCardSeqApplyAt_ms = track.GetFirstKeyFrame()->GetTriggerTime_ms();
    }
  }

  uint dealCardSeqDuration_ms = 0;
  const auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();
  if(nullptr != seqContainer){
    auto* seqPtr = seqContainer->GetSequenceByName(Vision::SpriteName::DealSequence_Dealer);
    const uint numFrames = seqPtr->GetNumFrames();
    dealCardSeqDuration_ms = numFrames * ANIM_TIME_STEP_MS;
  }

  // Time after the beginning of a deal animation at which the card has finished flipping,
  // and we want to display the fully up-to-date set of cards
  _displayDealtCardsAt_ms = _dealCardSeqApplyAt_ms + dealCardSeqDuration_ms;

  // Init an ongoing image showing all cards dealt thus far
  Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
  _compImg = std::make_unique<Vision::CompositeImage>(dataAccessorComp.GetSpriteCache(),
                                                      faceHueAndSaturation,
                                                      FACE_DISPLAY_WIDTH,
                                                      FACE_DISPLAY_HEIGHT);
  // To prevent rendering errors on the very first card, add a blank layer
  _compImg->AddEmptyLayer(dataAccessorComp.GetSpriteSequenceContainer());

  auto& compLayoutMap = *dataAccessorComp.GetCompLayoutMap(); 
  // Add the player card layout to the composite image
  {
    const auto iter = compLayoutMap.find(Vision::CompositeImageLayout::PlayerCardLayout);
    if(iter != compLayoutMap.end()){
      _compImg->MergeInImage(iter->second);
    }
  }
  // Add the dealer card layout to the composite image
  {
    const auto iter = compLayoutMap.find(Vision::CompositeImageLayout::DealerCardLayout);
    if(iter != compLayoutMap.end()){
      _compImg->MergeInImage(iter->second);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::Update(BehaviorExternalInterface& bei)
{
  // Callbacks provided by the parent behavior should only be called during the behavior update tick, in case
  // they impact the state of the parent behavior.
  if(_animCompletedLastFrame){
    _animCompletedLastFrame = false;
    if(nullptr != _animCompletedCallback){
      _animCompletedCallback();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DealToPlayer(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  const std::vector<Card>& playerHand = _game->GetPlayerHand();
  if(playerHand.size() > kPlayerCardSlots.size()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.ExcessiveHandSize",
                      "Player hand size cannot exceed %d cards",
                      static_cast<int>(kPlayerCardSlots.size()));
    return;
  }

  if(playerHand.empty()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.EmptyPlayerHand",
                      "Player hand should never be empty when displayed");
    return;
  }

  const int cardPositionIndex = static_cast<int>(playerHand.size()) - 1;
  Vision::SpriteName cardSpriteName = kPlayerCardAssets[playerHand.back().GetID()];
  Vision::SpriteBoxName cardSpriteBoxName = kPlayerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kPlayerCardLayers[cardPositionIndex];
  
  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, 
                                    cardSpriteName, _displayDealtCardsAt_ms, Vision::SpriteName::DealSequence_Player,
                                    _dealCardSeqApplyAt_ms);
  _animCompletedCallback = callback;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DealToDealer(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  const std::vector<Card>& dealerHand = _game->GetDealerHand();
  if(dealerHand.size() > kDealerCardSlots.size()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.ExcessiveHandSize",
                      "Dealer hand size cannot exceed %d cards",
                      (int)kDealerCardSlots.size());
    return;
  }

  if(dealerHand.empty()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.EmptyDealerHand",
                      "Dealer hand should never be empty when displayed");
    return;
  }

  Card card = dealerHand.back();

  // Handle face up/down display
  Vision::SpriteName cardSpriteName;
  if(card.IsFaceUp()){
    cardSpriteName = kDealerCardAssets[card.GetID()];
  } else {
    cardSpriteName = Vision::SpriteName::CardBack_Dealer;
  }

  const int cardPositionIndex = (int)dealerHand.size() - 1;
  Vision::SpriteBoxName cardSpriteBoxName = kDealerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kDealerCardLayers[cardPositionIndex];

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, cardSpriteName, 
                                    _displayDealtCardsAt_ms, Vision::SpriteName::DealSequence_Dealer, 
                                    _dealCardSeqApplyAt_ms);
  _animCompletedCallback = callback;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::SpreadPlayerCards(BehaviorExternalInterface& bei)
{
  // TODO:(str) Implement this later.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::Flop(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  _animCompletedCallback = callback;

  const std::vector<Card>& dealerHand = _game->GetDealerHand();

  if(dealerHand.empty()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.Flop.EmptyDealerHand",
                      "Dealer hand should never be empty when flopping");
    return;
  }

  // Always flipping the dealer's first card
  Card card = dealerHand.front();
  Vision::SpriteName cardSpriteName = kDealerCardAssets[card.GetID()];
  const int cardPositionIndex = 0;
  Vision::SpriteBoxName cardSpriteBoxName = kDealerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kDealerCardLayers[cardPositionIndex];

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, cardSpriteName,
                                    _displayDealtCardsAt_ms, Vision::SpriteName::FlipSequence_Dealer,
                                    _dealCardSeqApplyAt_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DisplayCharlieFrame(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  bei.GetAnimationComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK);
  _animCompletedCallback = callback;

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, Vision::LayerName::PlayerCardOverlay,
                                    Vision::SpriteBoxName::PlayerCardOverlay, Vision::SpriteName::CharlieFrame,
                                    _displayDealtCardsAt_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::PlayCompositeCardAnimationAndLock(const BehaviorExternalInterface& bei,
                                                            const char*                      compAnimName,
                                                            const Vision::LayerName&         layerName,
                                                            const Vision::SpriteBoxName&     spriteBoxName,
                                                            const Vision::SpriteName&        finalItemImageName,
                                                            const uint                       showFinalImageAt_ms,
                                                            const Vision::SpriteName&        itemAnimSeqName,
                                                            const uint                       showAnimSeqAt_ms)
{
  bei.GetAnimationComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK);

  auto& dataAccessorComp = bei.GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  auto* spriteCache = dataAccessorComp.GetSpriteCache();
  auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();

  // Set up the final state of the static image in the callback
  auto animationCallback = [this](const AnimationComponent::AnimResult res)
  {
    // Note that the anim has completed to exercise callbacks
    _animCompletedLastFrame = true;
  };

  // Set up the pre-animation image with the composite anim to get things rolling
  int outAnimationDuration_ms = 0;
  bool shouldInterrupt = true;
  bool emptySpriteBoxesAreValid = true;
  bei.GetAnimationComponent().PlayCompositeAnimation(compAnimName,
                                                     *(_compImg.get()),
                                                     ANIM_TIME_STEP_MS,
                                                     outAnimationDuration_ms,
                                                     shouldInterrupt,
                                                     emptySpriteBoxesAreValid,
                                                     animationCallback);

  Vision::CompositeImageLayer* layer;
  layer = _compImg->GetLayerByName(layerName);

  // if the user specified a png sequence card anim, display it
  if(Vision::SpriteName::Count != itemAnimSeqName){
    layer->AddToImageMap(spriteCache, seqContainer, spriteBoxName, itemAnimSeqName);
    bei.GetAnimationComponent().UpdateCompositeImage(*(_compImg.get()), showAnimSeqAt_ms);
  }

  // Set up the final image to be displayed
  layer->AddToImageMap(spriteCache, seqContainer, spriteBoxName, finalItemImageName);
  bei.GetAnimationComponent().UpdateCompositeImage(*(_compImg.get()), showFinalImageAt_ms); 

  // Lock Tracks at the end of the animation
  RobotInterface::LockAnimTracks msg((u8)AnimTrackFlag::FACE_TRACK);
  RobotInterface::EngineToRobot wrapper(std::move(msg));
  bei.GetAnimationComponent().AlterStreamingAnimationAtTime(std::move(wrapper), showFinalImageAt_ms, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::ClearCards(BehaviorExternalInterface& bei)
{
  bei.GetAnimationComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK);

  for(auto layerName : kPlayerCardLayers){
    bei.GetAnimationComponent().ClearCompositeImageLayer( layerName, 0);
  }

  for(auto layerName : kDealerCardLayers){
    bei.GetAnimationComponent().ClearCompositeImageLayer( layerName, 0);
  }

  bei.GetAnimationComponent().ClearCompositeImageLayer(Vision::LayerName::PlayerCardOverlay, 0);

  Init(bei);
}

} // namespace Cozmo
} // namespace Anki
