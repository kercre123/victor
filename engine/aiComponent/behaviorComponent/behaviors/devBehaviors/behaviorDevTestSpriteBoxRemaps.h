/**
 * File: BehaviorDevTestSpriteBoxRemaps.h
 *
 * Author: Sam Russell
 * Created: 2019-04-09
 *
 * Description: Testing point for SpriteBoxRemap functionality
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestSpriteBoxRemaps__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestSpriteBoxRemaps__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/vision/shared/spritePathMap.h"

namespace Anki {
namespace Vector {

class BehaviorDevTestSpriteBoxRemaps : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevTestSpriteBoxRemaps();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevTestSpriteBoxRemaps(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  void ClearAllPositions();
  void DealNextPlayerCard();
  void DealNextDealerCard();

  using AnimRemapMap = std::unordered_map<Vision::SpriteBoxName, Vision::SpritePathMap::AssetID>;

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    AnimRemapMap remapMap;
    int playerCardIndex = 0;
    int dealerCardIndex = 0;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestSpriteBoxRemaps__
