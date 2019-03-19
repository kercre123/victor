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
#include "engine/components/movementComponent.h"

namespace Anki{
namespace Vector{

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
const char* kCharlieFrameSpriteName = "charlieframe";
// These vectors provide index-based conversion from CardID(0-51) to string AssetName
const std::vector<const char* const> kPlayerCardAssets = {
  // Spades
  "blackjack_player_spadeace",
  "blackjack_player_spade2",
  "blackjack_player_spade3",
  "blackjack_player_spade4",
  "blackjack_player_spade5",
  "blackjack_player_spade6",
  "blackjack_player_spade7",
  "blackjack_player_spade8",
  "blackjack_player_spade9",
  "blackjack_player_spade10",
  "blackjack_player_spadejack",
  "blackjack_player_spadequeen",
  "blackjack_player_spadeking",
  // Hearts
  "blackjack_player_heartace",
  "blackjack_player_heart2",
  "blackjack_player_heart3",
  "blackjack_player_heart4",
  "blackjack_player_heart5",
  "blackjack_player_heart6",
  "blackjack_player_heart7",
  "blackjack_player_heart8",
  "blackjack_player_heart9",
  "blackjack_player_heart10",
  "blackjack_player_heartjack",
  "blackjack_player_heartqueen",
  "blackjack_player_heartking",
  // Diamonds
  "blackjack_player_diamondace",
  "blackjack_player_diamond2",
  "blackjack_player_diamond3",
  "blackjack_player_diamond4",
  "blackjack_player_diamond5",
  "blackjack_player_diamond6",
  "blackjack_player_diamond7",
  "blackjack_player_diamond8",
  "blackjack_player_diamond9",
  "blackjack_player_diamond10",
  "blackjack_player_diamondjack",
  "blackjack_player_diamondqueen",
  "blackjack_player_diamondking",
  // Clubs
  "blackjack_player_clubsace",
  "blackjack_player_clubs2",
  "blackjack_player_clubs3",
  "blackjack_player_clubs4",
  "blackjack_player_clubs5",
  "blackjack_player_clubs6",
  "blackjack_player_clubs7",
  "blackjack_player_clubs8",
  "blackjack_player_clubs9",
  "blackjack_player_clubs10",
  "blackjack_player_clubsjack",
  "blackjack_player_clubsqueen",
  "blackjack_player_clubsking"
};

const char* kDealerCardBackSpriteName = "blackjack_vector_card_back";
const std::vector<const char* const> kDealerCardAssets = {
  // Spades
  "blackjack_vector_spadeace",
  "blackjack_vector_spade2",
  "blackjack_vector_spade3",
  "blackjack_vector_spade4",
  "blackjack_vector_spade5",
  "blackjack_vector_spade6",
  "blackjack_vector_spade7",
  "blackjack_vector_spade8",
  "blackjack_vector_spade9",
  "blackjack_vector_spade10",
  "blackjack_vector_spadejack",
  "blackjack_vector_spadequeen",
  "blackjack_vector_spadeking",
  // Hearts
  "blackjack_vector_heartace",
  "blackjack_vector_heart2",
  "blackjack_vector_heart3",
  "blackjack_vector_heart4",
  "blackjack_vector_heart5",
  "blackjack_vector_heart6",
  "blackjack_vector_heart7",
  "blackjack_vector_heart8",
  "blackjack_vector_heart9",
  "blackjack_vector_heart10",
  "blackjack_vector_heartjack",
  "blackjack_vector_heartqueen",
  "blackjack_vector_heartking",
  // Diamonds
  "blackjack_vector_diamondace",
  "blackjack_vector_diamond2",
  "blackjack_vector_diamond3",
  "blackjack_vector_diamond4",
  "blackjack_vector_diamond5",
  "blackjack_vector_diamond6",
  "blackjack_vector_diamond7",
  "blackjack_vector_diamond8",
  "blackjack_vector_diamond9",
  "blackjack_vector_diamond10",
  "blackjack_vector_diamondjack",
  "blackjack_vector_diamondqueen",
  "blackjack_vector_diamondking",
  // Clubs
  "blackjack_vector_clubsace",
  "blackjack_vector_clubs2",
  "blackjack_vector_clubs3",
  "blackjack_vector_clubs4",
  "blackjack_vector_clubs5",
  "blackjack_vector_clubs6",
  "blackjack_vector_clubs7",
  "blackjack_vector_clubs8",
  "blackjack_vector_clubs9",
  "blackjack_vector_clubs10",
  "blackjack_vector_clubsjack",
  "blackjack_vector_clubsqueen",
  "blackjack_vector_clubsking"
};

// Robot motion animations
const char* kDealAnimationName = "anim_blackjack_deal_01";
const char* kSwipeAnimationName = "anim_blackjack_swipe_01";
const char* kTrackLockingKey = "BlackJackVisualizer_track_lock";

// Card SpriteSequences
const char* kPlayerCardFlipSeqName    = "blackjack_player_back";
const char* kDealerCardFlipSeqName    = "blackjack_vector_back";
const char* kDealerCardTurnSeqName    = "blackjack_vector_flipover";

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
    const Animation* dealAnimPtr = animContainer->GetAnimation(kDealAnimationName);
    if(nullptr == dealAnimPtr){
      PRINT_NAMED_ERROR("BlackJackVisualizer.Init.AnimationNotFoundInContainer",
                        "Animations need to be manually loaded on engine side - %s is not",
                        kDealAnimationName);
    } else {
      const auto& track = dealAnimPtr->GetTrack<EventKeyFrame>();
      _dealCardSeqApplyAt_ms = track.GetFirstKeyFrame()->GetTriggerTime_ms();
    }

    const Animation* swipeAnimPtr = animContainer->GetAnimation(kSwipeAnimationName);
    if(nullptr == swipeAnimPtr){
      PRINT_NAMED_ERROR("BlackJackVisualizer.Init.AnimationNotFoundInContainer",
                        "Animations need to be manually loaded on engine side - %s is not",
                        kSwipeAnimationName);
    } else {
      const auto& track = swipeAnimPtr->GetTrack<EventKeyFrame>();
      _clearCardsDuringSwipeAt_ms = track.GetFirstKeyFrame()->GetTriggerTime_ms();
    }
  }

  uint dealCardSeqDuration_ms = 0;
  const auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();
  if(nullptr != seqContainer){
    auto* seqPtr = seqContainer->GetSpriteSequence(kDealerCardFlipSeqName);
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
  // Hack: if we've received an Update call from the parent behavior, it's active. Don't clear locks on anim callbacks
  // See VIC-5926
  _shouldClearLocksOnCallback = false;

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
  const std::string cardSpriteName(kPlayerCardAssets[playerHand.back().GetID()]);
  Vision::SpriteBoxName cardSpriteBoxName = kPlayerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kPlayerCardLayers[cardPositionIndex];

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, 
                                    cardSpriteName, _displayDealtCardsAt_ms, kPlayerCardFlipSeqName,
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
  std::string cardSpriteName;
  if(card.IsFaceUp()){
    cardSpriteName = kDealerCardAssets[card.GetID()];
  } else {
    cardSpriteName = kDealerCardBackSpriteName;
  }

  const int cardPositionIndex = (int)dealerHand.size() - 1;
  Vision::SpriteBoxName cardSpriteBoxName = kDealerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kDealerCardLayers[cardPositionIndex];

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, cardSpriteName, 
                                    _displayDealtCardsAt_ms, kDealerCardFlipSeqName, 
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
  const std::string cardSpriteName = kDealerCardAssets[card.GetID()];
  const int cardPositionIndex = 0;
  Vision::SpriteBoxName cardSpriteBoxName = kDealerCardSlots[cardPositionIndex];
  Vision::LayerName cardLayerName = kDealerCardLayers[cardPositionIndex];

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, cardLayerName, cardSpriteBoxName, cardSpriteName,
                                    _displayDealtCardsAt_ms, kDealerCardTurnSeqName,
                                    _dealCardSeqApplyAt_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DisplayCharlieFrame(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
  _animCompletedCallback = callback;

  PlayCompositeCardAnimationAndLock(bei, kDealAnimationName, Vision::LayerName::PlayerCardOverlay,
                                    Vision::SpriteBoxName::PlayerCardOverlay, kCharlieFrameSpriteName,
                                    _displayDealtCardsAt_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::PlayCompositeCardAnimationAndLock(const BehaviorExternalInterface& bei,
                                                            const char*                      compAnimName,
                                                            const Vision::LayerName&         layerName,
                                                            const Vision::SpriteBoxName&     spriteBoxName,
                                                            const std::string&               finalItemImageName,
                                                            const uint                       showFinalImageAt_ms,
                                                            const std::string&               itemAnimSeqName,
                                                            const uint                       showAnimSeqAt_ms)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);

  auto& dataAccessorComp = bei.GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  auto* spriteCache = dataAccessorComp.GetSpriteCache();
  auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();

  // Set up the final state of the static image in the callback
  auto animationCallback = [this, &bei](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded)
  {
    if(_shouldClearLocksOnCallback){
      bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
    }
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

  // Get a copy of the layer we'll be updating from the base image
  Vision::CompositeImageLayer* baseImageLayer = _compImg->GetLayerByName(layerName);
  Vision::CompositeImageLayer intentionalCopyLayer = *baseImageLayer;

  // Add the copied layer to a delta image
  Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
  Vision::CompositeImage compImageDelta(dataAccessorComp.GetSpriteCache(),
                                        faceHueAndSaturation,
                                        FACE_DISPLAY_WIDTH,
                                        FACE_DISPLAY_HEIGHT);
  compImageDelta.AddLayer(std::move(intentionalCopyLayer));
  Vision::CompositeImageLayer* deltaLayer = compImageDelta.GetLayerByName(layerName);

  // if the user specified a png sequence card anim, display it as a delta
  if(!itemAnimSeqName.empty()){
    deltaLayer->AddToImageMap(spriteCache, seqContainer, spriteBoxName, itemAnimSeqName);
    bei.GetAnimationComponent().UpdateCompositeImage(compImageDelta, showAnimSeqAt_ms);
  }

  // Set up the image to be displayed once the card has finished flipping as a delta 
  deltaLayer->AddToImageMap(spriteCache, seqContainer, spriteBoxName, finalItemImageName);
  bei.GetAnimationComponent().UpdateCompositeImage(compImageDelta, showFinalImageAt_ms); 

  // Store the changes to the base image that we'll start with for the next card 
  baseImageLayer->AddToImageMap(spriteCache, seqContainer, spriteBoxName, finalItemImageName);

  // Lock Tracks at the end of the animation 
  bei.GetMovementComponent().LockTracksAtStreamTime((u8)AnimTrackFlag::FACE_TRACK, showFinalImageAt_ms, false, kTrackLockingKey, kTrackLockingKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::SwipeToClearFace(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  _animCompletedCallback = callback;

  auto animationCallback = [this, &bei](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded)
  {
    if(_shouldClearLocksOnCallback){
      bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
    }
    // Note that the anim has completed to exercise callbacks
    _animCompletedLastFrame = true;
  };

  // Set up the pre-animation image with the composite anim to get things rolling
  int outAnimationDuration_ms = 0;
  bool shouldInterrupt = true;
  bool emptySpriteBoxesAreValid = true;
  bei.GetAnimationComponent().PlayCompositeAnimation(kSwipeAnimationName,
                                                     *(_compImg.get()),
                                                     ANIM_TIME_STEP_MS,
                                                     outAnimationDuration_ms,
                                                     shouldInterrupt,
                                                     emptySpriteBoxesAreValid,
                                                     animationCallback);

  ClearCards(bei, _clearCardsDuringSwipeAt_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::ClearCards(BehaviorExternalInterface& bei, uint32_t applyAt_ms)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);

  for(auto layerName : kPlayerCardLayers){
    bei.GetAnimationComponent().ClearCompositeImageLayer( layerName, applyAt_ms);
  }

  for(auto layerName : kDealerCardLayers){
    bei.GetAnimationComponent().ClearCompositeImageLayer( layerName, applyAt_ms);
  }

  bei.GetAnimationComponent().ClearCompositeImageLayer(Vision::LayerName::PlayerCardOverlay, applyAt_ms);

  Init(bei);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::ReleaseControlAndClearState(BehaviorExternalInterface& bei)
{
  ClearCards(bei);
  // Hack: Assume the parent behavior is being interrupted. If that's the case we will want to unlock the face from the
  // animCompletedCallback for any potentially orphaned CompositeAnimations that might currently be running. This will 
  // have no effect if no anim is currently in progress. See VIC-5926
  _shouldClearLocksOnCallback = true;

  _animCompletedLastFrame = false;
  _animCompletedCallback = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::VerifySpriteAssets(BehaviorExternalInterface& bei)
{
  auto& dataAccessorComp = bei.GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  auto* spritePathMap = dataAccessorComp.GetSpriteCache()->GetSpritePathMap();

  for(auto& assetName : kPlayerCardAssets){
    ANKI_VERIFY(spritePathMap->IsValidAssetName(assetName),
                "BlackJack.VerifySpriteAssets.InvalidAssetName",
                "Asset name: %s does not map to a valid sprite",
                assetName);
  }

  for(auto& assetName : kDealerCardAssets){
    ANKI_VERIFY(spritePathMap->IsValidAssetName(assetName),
                "BlackJack.VerifySpriteAssets.InvalidAssetName",
                "Asset name: %s does not map to a valid sprite",
                assetName);
  }
}

} // namespace Vector
} // namespace Anki
