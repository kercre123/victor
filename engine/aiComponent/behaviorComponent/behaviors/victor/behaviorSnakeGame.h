/**
 * File: BehaviorSnakeGame.h
 *
 * Author: ross
 * Created: 2018-02-27
 *
 * Description: victor plays the game snake
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSnakeGame__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSnakeGame__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {

namespace Vision {
class Image;
}
  
namespace Vector {
  
class SnakeGame;
class SnakeGameSolver;

class BehaviorSnakeGame : public ICozmoBehavior
{
public: 
  virtual ~BehaviorSnakeGame();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorSnakeGame(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override ;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void UpdateGame();
  
  void DrawGame( Vision::Image& image ) const;
  
  void AnimateDirection( uint8_t direction );

  struct InstanceConfig {
  };

  struct DynamicVariables {
    std::unique_ptr<Vision::Image> image;
    std::unique_ptr<SnakeGame> game;
    std::unique_ptr<SnakeGameSolver> solver;
    unsigned int gameTicks;
    bool lost = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

}
}

#endif
