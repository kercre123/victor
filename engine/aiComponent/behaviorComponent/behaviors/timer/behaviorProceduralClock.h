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
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class BehaviorProceduralClock : public ICozmoBehavior
{
public:
  // digits of display - numbered left to right
  enum class DigitID{
    DigitOne = 0,
    DigitTwo,
    DigitThree,
    DigitFour,
    Count
  };


  // Specify how each digit on the face should be calculated - must set all 4 values in map
  void SetDigitFunctions(std::map<DigitID, std::function<int()>>&& functions);
  // Specify a callback which should be called when the clock is shown on the robot's face
  void SetShowClockCallback(std::function<void()>&& callback)
  {
    _instanceParams.showClockCallback = callback;
  }

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorProceduralClock(const Json::Value& config);
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void InitBehavior() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override { return true;}

  void TransitionToTurnToFace();
  void TransitionToGetIn();
  void TransitionToShowClock();
  void TransitionToGetOut();

private:
  enum class BehaviorState{
    TurnToFace,
    GetIn,
    ShowClock,
    GetOut,
  };


  struct InstanceParams{
    Json::Value templateJSON;
    std::map<std::string, std::string> staticElements;
    // TODO: transition these maps to fullyEnumeratedArrays
    std::map<DigitID, std::string> digitToTemplateMap;
    std::map<DigitID, std::function<int()>> getDigitFunctions; 
    std::map<int, std::string> intsToImages;
    AnimationTrigger getInAnim;
    AnimationTrigger getOutAnim;
    int totalTimeDisplayClock;
    bool shouldTurnToFace = false;
    std::function<void()> showClockCallback; 
  };

  struct LifetimeParams{
    int timeShowClockStarted = 0;
    int nextUpdateTime_s = 0;
    BehaviorState currentState = BehaviorState::TurnToFace;
    SmartFaceID targetFaceID;
  };

  
  InstanceParams _instanceParams;
  LifetimeParams _lifetimeParams;

  DigitID StringToDigitID(const std::string& str);
  // Function which builds and displays the timer - adds the 4 core digits on top
  // of any quadrant images passed into the function
  void BuildAndDisplayTimer(std::map<std::string, std::string>& quadrantMap);

  // Updates the target face within lifetime params - returns the member face for checking success inline
  SmartFaceID UpdateTargetFace();




};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorProceduralClock_H__
