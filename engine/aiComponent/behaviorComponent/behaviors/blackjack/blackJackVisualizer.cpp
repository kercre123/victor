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

#include "clad/types/compositeImageTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackSimulation.h"
#include "engine/clad/types/animationTypes.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/movementComponent.h"

namespace Anki{
namespace Vector{

namespace{

// SpriteBoxName lookup
const Vision::SpriteBoxName kCharlieFrameSpriteBox = Vision::SpriteBoxName::SpriteBox_31;
const std::vector<Vision::SpriteBoxName> kPlayerDealingSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_1,
  Vision::SpriteBoxName::SpriteBox_4,
  Vision::SpriteBoxName::SpriteBox_7,
  Vision::SpriteBoxName::SpriteBox_10,
  Vision::SpriteBoxName::SpriteBox_13
};
const std::vector<Vision::SpriteBoxName> kPlayerRevealSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_2,
  Vision::SpriteBoxName::SpriteBox_5,
  Vision::SpriteBoxName::SpriteBox_8,
  Vision::SpriteBoxName::SpriteBox_11,
  Vision::SpriteBoxName::SpriteBox_14
};
const std::vector<Vision::SpriteBoxName> kPlayerFinalSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_3,
  Vision::SpriteBoxName::SpriteBox_6,
  Vision::SpriteBoxName::SpriteBox_9,
  Vision::SpriteBoxName::SpriteBox_12,
  Vision::SpriteBoxName::SpriteBox_15
};
const std::vector<Vision::SpriteBoxName> kDealerDealingSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_16,
  Vision::SpriteBoxName::SpriteBox_19,
  Vision::SpriteBoxName::SpriteBox_22,
  Vision::SpriteBoxName::SpriteBox_25,
  Vision::SpriteBoxName::SpriteBox_28
};
const std::vector<Vision::SpriteBoxName> kDealerRevealSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_17,
  Vision::SpriteBoxName::SpriteBox_20,
  Vision::SpriteBoxName::SpriteBox_23,
  Vision::SpriteBoxName::SpriteBox_26,
  Vision::SpriteBoxName::SpriteBox_29
};
const std::vector<Vision::SpriteBoxName> kDealerFinalSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_18,
  Vision::SpriteBoxName::SpriteBox_21,
  Vision::SpriteBoxName::SpriteBox_24,
  Vision::SpriteBoxName::SpriteBox_27,
  Vision::SpriteBoxName::SpriteBox_30
};

// Asset Lookup
const Vision::SpritePathMap::AssetID kCharlieFrameAssetID = Vision::SpritePathMap::GetAssetID("charlieframe");

// These vectors provide index-based conversion from CardID(0-51) to string AssetName
const std::vector<Vision::SpritePathMap::AssetID> kPlayerCardAssets = {
  Vision::SpritePathMap::GetAssetID("blackjack_player_spadeace"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade2"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade3"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade4"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade5"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade6"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade7"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade8"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade9"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade10"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spadejack"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spadequeen"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spadeking"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heartace"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart2"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart3"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart4"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart5"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart6"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart7"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart8"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart9"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heart10"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heartjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heartqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_heartking"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamondace"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond2"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond3"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond4"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond5"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond6"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond7"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond8"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond9"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamond10"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamondjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamondqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_diamondking"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubsace"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs2"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs3"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs4"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs5"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs6"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs7"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs8"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs9"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubs10"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubsjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubsqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_clubsking")
};

const Vision::SpritePathMap::AssetID kDealerCardBackSpriteID =
  Vision::SpritePathMap::GetAssetID("blackjack_vector_card_back");
const std::vector<Vision::SpritePathMap::AssetID> kDealerCardAssets = {
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spadeace"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade2"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade3"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade4"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade5"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade6"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade7"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade8"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade9"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade10"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spadejack"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spadequeen"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spadeking"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heartace"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart2"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart3"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart4"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart5"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart6"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart7"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart8"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart9"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heart10"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heartjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heartqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_heartking"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamondace"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond2"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond3"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond4"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond5"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond6"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond7"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond8"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond9"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamond10"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamondjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamondqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_diamondking"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubsace"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs2"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs3"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs4"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs5"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs6"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs7"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs8"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs9"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubs10"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubsjack"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubsqueen"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_clubsking")
};

// Robot motion animations
const char* kDealAnimationName = "anim_blackjack_deal_01_temp_remap";
const char* kSwipeAnimationName = "anim_blackjack_swipe_01_temp_remap";
// TODO(str): VIC-14450 Convert BlackJack anims to use SpriteBoxes ^^
// Hand(engineer) built placeholder animations made by adding SpriteBoxKeyFrames to the original
// animations are providing BlackJack functionality for now. This way the animation team can convert
// the existing animations (if desired) and they can be substituted when ready without an interruption
// to BlackJack's functionality in Master, at which time the placeholders can be deleted.

const char* kTrackLockingKey = "BlackJackVisualizer_track_lock";

// Card SpriteSequences
const Vision::SpritePathMap::AssetID kDealPlayerSpriteSeqID =
  Vision::SpritePathMap::GetAssetID("blackjack_player_back");
const Vision::SpritePathMap::AssetID kDealDealerSpriteSeqID =
  Vision::SpritePathMap::GetAssetID("blackjack_vector_back");
const Vision::SpritePathMap::AssetID kDealerFlopSpriteSeqID =
  Vision::SpritePathMap::GetAssetID("blackjack_vector_flipover");

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlackJackVisualizer::BlackJackVisualizer(const BlackJackGame* game)
: _game(game)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::Init(BehaviorExternalInterface& bei)
{
  ResetHands();
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
  if(playerHand.size() > kPlayerFinalSpriteBoxes.size()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.ExcessiveHandSize",
                      "Player hand size cannot exceed %d cards",
                      static_cast<int>(kPlayerFinalSpriteBoxes.size()));
    return;
  }

  if(playerHand.empty()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.EmptyPlayerHand",
                      "Player hand should never be empty when displayed");
    return;
  }

  const int cardPositionIndex = static_cast<int>(playerHand.size()) - 1;
  const Vision::SpritePathMap::AssetID cardSpriteID = kPlayerCardAssets[playerHand.back().GetID()];
  Vision::SpriteBoxName dealSeqSBName = kPlayerDealingSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName revealSBName = kPlayerRevealSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName finalSBName = kPlayerFinalSpriteBoxes[cardPositionIndex];

  _animCompletedCallback = callback;
  DealCard(bei, dealSeqSBName, kDealPlayerSpriteSeqID, revealSBName, finalSBName, cardSpriteID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DealToDealer(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  const std::vector<Card>& dealerHand = _game->GetDealerHand();
  if(dealerHand.size() > kDealerFinalSpriteBoxes.size()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.ExcessiveHandSize",
                      "Dealer hand size cannot exceed %d cards",
                      (int)kDealerFinalSpriteBoxes.size());
    return;
  }

  if(dealerHand.empty()){
    PRINT_NAMED_ERROR("BlackJackVisualizer.EmptyDealerHand",
                      "Dealer hand should never be empty when displayed");
    return;
  }

  Card card = dealerHand.back();

  // Handle face up/down display
  Vision::SpritePathMap::AssetID cardSpriteID;
  if(card.IsFaceUp()){
    cardSpriteID = kDealerCardAssets[card.GetID()];
  } else {
    cardSpriteID = kDealerCardBackSpriteID;
  }

  const int cardPositionIndex = static_cast<int>(dealerHand.size()) - 1;
  Vision::SpriteBoxName dealSeqSBName = kDealerDealingSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName revealSBName = kDealerRevealSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName finalSBName = kDealerFinalSpriteBoxes[cardPositionIndex];

  _animCompletedCallback = callback;
  DealCard(bei, dealSeqSBName, kDealDealerSpriteSeqID, revealSBName, finalSBName, cardSpriteID);
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
  Vision::SpritePathMap::AssetID cardSpriteID = kDealerCardAssets[card.GetID()];
  const int cardPositionIndex = 0;

  Vision::SpriteBoxName dealSeqSBName = kDealerDealingSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName revealSBName = kDealerRevealSpriteBoxes[cardPositionIndex];
  Vision::SpriteBoxName finalSBName = kDealerFinalSpriteBoxes[cardPositionIndex];

  // Clear off the card back image holding the dealer's first card position before starting the animation,
  _cardAssetMap[finalSBName] = Vision::SpritePathMap::kEmptySpriteBoxID;
  DealCard(bei, dealSeqSBName, kDealerFlopSpriteSeqID, revealSBName, finalSBName, cardSpriteID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DealCard(const BehaviorExternalInterface& bei,
                                   const Vision::SpriteBoxName& dealSeqSBName,
                                   const Vision::SpritePathMap::AssetID dealSpriteSeqName,
                                   const Vision::SpriteBoxName& revealSBName,
                                   const Vision::SpriteBoxName& finalSBName,
                                   const Vision::SpritePathMap::AssetID cardSpriteName)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);

  // Set up to show the animated card dealing
  _cardAssetMap[dealSeqSBName] = dealSpriteSeqName;
  _cardAssetMap[revealSBName] = cardSpriteName;

  auto animationCallback = [this, &bei](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded)
  {
    if(_shouldClearLocksOnCallback){
      bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
    }
    // Note that the anim has completed to exercise callbacks
    _animCompletedLastFrame = true;
  };
  
  // Play the animation with appropriate remaps
  const bool interruptRunning = true;
  bei.GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kDealAnimationName,
                                                          _cardAssetMap,
                                                          interruptRunning,
                                                          animationCallback,
                                                          kTrackLockingKey);

  // Clear the dealing animation assets and reveal spriteBox...
  _cardAssetMap[dealSeqSBName] = Vision::SpritePathMap::kEmptySpriteBoxID;
  _cardAssetMap[revealSBName] = Vision::SpritePathMap::kEmptySpriteBoxID;
  // ...and store the dealt card in the final spritebox so its there in future animations
  _cardAssetMap[finalSBName] = cardSpriteName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::DisplayCharlieFrame(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
  _animCompletedCallback = callback;

  _cardAssetMap[kCharlieFrameSpriteBox] = kCharlieFrameAssetID;

  auto animationCallback = [this, &bei](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded)
  {
    if(_shouldClearLocksOnCallback){
      bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
    }
    // Note that the anim has completed to exercise callbacks
    _animCompletedLastFrame = true;
  };

  const bool interruptRunning = true;
  bei.GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kDealAnimationName,
                                                          _cardAssetMap,
                                                          interruptRunning,
                                                          animationCallback,
                                                          kTrackLockingKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::SwipeToClearFace(BehaviorExternalInterface& bei, std::function<void()> callback)
{
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
  _animCompletedCallback = callback;

  auto animationCallback = [this, &bei](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded)
  {
    if(_shouldClearLocksOnCallback){
      bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);
    }
    // Note that the anim has completed to exercise callbacks
    _animCompletedLastFrame = true;
  };

  // Just grab the final card positions since the swipe anim doesn't have SB's for deal/reveal 
  AnimRemapMap swipeRemaps;
  for(const auto& spriteBox : kPlayerFinalSpriteBoxes){
    swipeRemaps[spriteBox] = _cardAssetMap[spriteBox];
  }
  for(const auto& spriteBox : kDealerFinalSpriteBoxes){
    swipeRemaps[spriteBox] = _cardAssetMap[spriteBox];
  }
  swipeRemaps[kCharlieFrameSpriteBox] = _cardAssetMap[kCharlieFrameSpriteBox];

  const bool interruptRunning = true;
  bei.GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kSwipeAnimationName,
                                                          swipeRemaps,
                                                          interruptRunning,
                                                          animationCallback);

  ResetHands();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::ResetHands()
{
  _cardAssetMap.clear();

  for(const auto& spriteBox : kPlayerDealingSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kPlayerRevealSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kPlayerFinalSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerDealingSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerRevealSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerFinalSpriteBoxes){
    _cardAssetMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  _cardAssetMap.insert({kCharlieFrameSpriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlackJackVisualizer::ReleaseControlAndClearState(BehaviorExternalInterface& bei)
{
  ResetHands();
  bei.GetMovementComponent().UnlockTracks((u8)AnimTrackFlag::FACE_TRACK, kTrackLockingKey);

  // Hack: Assume the parent behavior is being interrupted. If that's the case we will want to unlock the face from the
  // animCompletedCallback for any potentially orphaned CompositeAnimations that might currently be running. This will 
  // have no effect if no anim is currently in progress. See VIC-5926
  _shouldClearLocksOnCallback = true;

  _animCompletedLastFrame = false;
  _animCompletedCallback = nullptr;
}

} // namespace Vector
} // namespace Anki
