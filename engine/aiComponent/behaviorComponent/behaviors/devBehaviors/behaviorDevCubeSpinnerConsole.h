/**
 * File: BehaviorDevCubeSpinnerConsole.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-21
 *
 * Description: A behavior for testing cube spinner via the web console
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevCubeSpinnerConsole__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevCubeSpinnerConsole__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviors/cubeSpinner/cubeSpinnerGame.h"

namespace Anki {
namespace Cozmo {

// forward declaration
class CubeSpinnerGame;

class BehaviorDevCubeSpinnerConsole : public ICozmoBehavior
{
public: 
  virtual ~BehaviorDevCubeSpinnerConsole();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDevCubeSpinnerConsole(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;

private:

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<CubeSpinnerGame> cubeSpinnerGame;
    Json::Value gameConfig;
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID objID;
    CubeSpinnerGame::LockResult lastLockResult = CubeSpinnerGame::LockResult::Count;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDevCubeSpinnerConsole__
