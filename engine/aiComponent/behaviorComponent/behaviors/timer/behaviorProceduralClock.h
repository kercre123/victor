/**
 * File: behaviorProceduralClock.h
 *
 * Author: Kevin M. Karol
 * Created: 1/31/18
 *
 * Description: Behavior which displays a procedural clock on the robot's face
 *
 * Copyright: Anki, Inc. 208
 *
 **/

#ifndef __Engine_Behaviors_BehaviorProceduralClock_H__
#define __Engine_Behaviors_BehaviorProceduralClock_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorProceduralClock : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorProceduralClock(const Json::Value& config);
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override { return true;}

  void TransitionToGetIn();
  void TransitionToShowClock();
  void TransitionToGetOut();

private:

  // digits of display - numbered left to right
  enum class DigitID{
    DigitOne,
    DigitTwo,
    DigitThree,
    DigitFour,
    Count
  };

  enum class BehaviorState{
    GetIn,
    ShowClock,
    GetOut,
  };


  struct InstanceParams{
    Json::Value templateJSON;
    std::map<std::string, std::string> staticElements;
    std::map<DigitID, std::string> digitToTemplateMap;
    std::map<int, std::string> intsToImages;
    AnimationTrigger getInAnim;
    AnimationTrigger getOutAnim;
    int totalTimeDisplayClock;
  };

  struct LifetimeParams{
    int timeShowClockStarted = 0;
    int nextUpdateTime_s = 0;
    BehaviorState currentState = BehaviorState::GetIn;
  };

  
  InstanceParams _instanceParams;
  LifetimeParams _lifetimeParams;

  DigitID StringToDigitID(const std::string& str);
  // Function which builds and displays the timer - adds the 4 core digits on top
  // of any quadrant images passed into the function
  void BuildAndDisplayTimer(std::map<std::string, std::string>& quadrantMap);




};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorProceduralClock_H__
