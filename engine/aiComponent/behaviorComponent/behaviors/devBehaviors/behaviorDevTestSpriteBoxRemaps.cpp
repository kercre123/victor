/**
 * File: BehaviorDevTestSpriteBoxRemaps.cpp
 *
 * Author: Sam Russell
 * Created: 2019-04-09
 *
 * Description: Testing point for SpriteBoxRemap functionality
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevTestSpriteBoxRemaps.h"

#include "engine/components/animationComponent.h"

namespace Anki {
namespace Vector {

namespace{
// SpriteBoxName lookup
const std::vector<Vision::SpriteBoxName> kPlayerDealingSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_1,
  Vision::SpriteBoxName::SpriteBox_4,
  Vision::SpriteBoxName::SpriteBox_7,
  Vision::SpriteBoxName::SpriteBox_10,
  Vision::SpriteBoxName::SpriteBox_13
};
const std::vector<Vision::SpriteBoxName> kPlayerShowCardSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_2,
  Vision::SpriteBoxName::SpriteBox_5,
  Vision::SpriteBoxName::SpriteBox_8,
  Vision::SpriteBoxName::SpriteBox_11,
  Vision::SpriteBoxName::SpriteBox_14
};
const std::vector<Vision::SpriteBoxName> kPlayerCardSpriteBoxes = {
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
const std::vector<Vision::SpriteBoxName> kDealerShowCardSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_17,
  Vision::SpriteBoxName::SpriteBox_20,
  Vision::SpriteBoxName::SpriteBox_23,
  Vision::SpriteBoxName::SpriteBox_26,
  Vision::SpriteBoxName::SpriteBox_29
};
const std::vector<Vision::SpriteBoxName> kDealerCardSpriteBoxes = {
  Vision::SpriteBoxName::SpriteBox_18,
  Vision::SpriteBoxName::SpriteBox_21,
  Vision::SpriteBoxName::SpriteBox_24,
  Vision::SpriteBoxName::SpriteBox_27,
  Vision::SpriteBoxName::SpriteBox_30
};

const std::vector<Vision::SpritePathMap::AssetID> kPlayerCards = {
  Vision::SpritePathMap::GetAssetID("blackjack_player_spadeace"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade3"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade5"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade7"),
  Vision::SpritePathMap::GetAssetID("blackjack_player_spade9")
};

const std::vector<Vision::SpritePathMap::AssetID> kDealerCards = {
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade2"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade4"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade6"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade8"),
  Vision::SpritePathMap::GetAssetID("blackjack_vector_spade10")
};

const Vision::SpriteBoxName kCharlieFrameSpriteBox = Vision::SpriteBoxName::SpriteBox_31;
const Vision::SpritePathMap::AssetID kCharlieFrameAssetID = Vision::SpritePathMap::GetAssetID("charlieframe");
const std::string kDealAnimationName = "anim_test_spriteboxremaps";

const Vision::SpritePathMap::AssetID kDealPlayerSpriteSeqID =
  Vision::SpritePathMap::GetAssetID("blackjack_player_back");
const Vision::SpritePathMap::AssetID kDealDealerSpriteSeqID =
  Vision::SpritePathMap::GetAssetID("blackjack_vector_back");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestSpriteBoxRemaps::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestSpriteBoxRemaps::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestSpriteBoxRemaps::BehaviorDevTestSpriteBoxRemaps(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevTestSpriteBoxRemaps::~BehaviorDevTestSpriteBoxRemaps()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDevTestSpriteBoxRemaps::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  ClearAllPositions();
  DealNextPlayerCard();


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::ClearAllPositions()
{
  _dVars.remapMap.clear();

  for(const auto& spriteBox : kPlayerDealingSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kPlayerShowCardSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kPlayerCardSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerDealingSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerShowCardSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  for(const auto& spriteBox : kDealerCardSpriteBoxes){
    _dVars.remapMap.insert({spriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
  }

  _dVars.remapMap.insert({kCharlieFrameSpriteBox, Vision::SpritePathMap::kEmptySpriteBoxID});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::DealNextPlayerCard()
{
  if(_dVars.playerCardIndex >= kPlayerCards.size()){
    _dVars.remapMap[kCharlieFrameSpriteBox] = kCharlieFrameAssetID;

    auto animationCallback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded){
      CancelSelf();
    };
    const bool interruptRunning = true;
    GetBEI().GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kDealAnimationName,
                                                                 _dVars.remapMap,
                                                                 interruptRunning,
                                                                 animationCallback);
  }
  else{
    // Show the animated card dealing
    _dVars.remapMap[kPlayerShowCardSpriteBoxes[_dVars.playerCardIndex]] = kPlayerCards[_dVars.playerCardIndex];
    _dVars.remapMap[kPlayerDealingSpriteBoxes[_dVars.playerCardIndex]] = kDealPlayerSpriteSeqID;

    auto animationCallback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded){
      DealNextDealerCard();
    };
    const bool interruptRunning = true;
    GetBEI().GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kDealAnimationName,
                                                                 _dVars.remapMap,
                                                                 interruptRunning,
                                                                 animationCallback);

    // Clear the dealing animation...
    _dVars.remapMap[kPlayerShowCardSpriteBoxes[_dVars.playerCardIndex]] = Vision::SpritePathMap::kEmptySpriteBoxID;
    _dVars.remapMap[kPlayerDealingSpriteBoxes[_dVars.playerCardIndex]] = Vision::SpritePathMap::kEmptySpriteBoxID;
    // ...and store the dealt card
    _dVars.remapMap[kPlayerCardSpriteBoxes[_dVars.playerCardIndex]] = kPlayerCards[_dVars.playerCardIndex];
    ++_dVars.playerCardIndex;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevTestSpriteBoxRemaps::DealNextDealerCard()
{
  // Show the animated card dealing
  _dVars.remapMap[kDealerShowCardSpriteBoxes[_dVars.dealerCardIndex]] = kDealerCards[_dVars.dealerCardIndex];
  _dVars.remapMap[kDealerDealingSpriteBoxes[_dVars.dealerCardIndex]] = kDealPlayerSpriteSeqID;

  auto animationCallback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded){
    DealNextPlayerCard();
  };
  const bool interruptRunning = true;
  GetBEI().GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(kDealAnimationName,
                                                               _dVars.remapMap,
                                                               interruptRunning,
                                                               animationCallback);

  // Clear the dealing animation...
  _dVars.remapMap[kDealerShowCardSpriteBoxes[_dVars.dealerCardIndex]] = Vision::SpritePathMap::kEmptySpriteBoxID;
  _dVars.remapMap[kDealerDealingSpriteBoxes[_dVars.dealerCardIndex]] = Vision::SpritePathMap::kEmptySpriteBoxID;
  // ...and store the dealt card
  _dVars.remapMap[kDealerCardSpriteBoxes[_dVars.dealerCardIndex]] = kDealerCards[_dVars.dealerCardIndex];
  ++_dVars.dealerCardIndex;
}

}
}
