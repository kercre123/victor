/**
 * File: BehaviorDevTestBlackjackViz.h
 *
 * Author: Sam Russell
 * Created: 2018-06-04
 *
 * Description: Simple input point for testing out all of the BlackjackVisualizer functionality
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestBlackjackViz__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestBlackjackViz__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackSimulation.h"
#include "engine/aiComponent/behaviorComponent/behaviors/blackjack/blackJackVisualizer.h"

namespace Anki {
namespace Vector {

class BehaviorDevTestBlackjackViz : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevTestBlackjackViz();

  void DealToPlayer();
  void DealToDealer();
  void Flop();
  void SpreadPlayerCards();
  void Charlie();
  void ClearCards(const bool shuffle = false);
  void AnnouncePlayerScore();
  void AnnounceDealerScore();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevTestBlackjackViz(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    Json::Value layout;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool firstDealerCard = true;
  };

  BlackJackGame         _game;
  BlackJackVisualizer   _visualizer;

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevTestBlackjackViz__
