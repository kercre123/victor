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

#include "clad/types/compositeImageTypes.h"
#include "clad/types/spriteNames.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"


namespace Anki {
namespace Cozmo {

class BehaviorProceduralClock : public ICozmoBehavior
{
public:
  // Specify how each digit on the face should be calculated - must set all 4 values in map
  void SetDigitFunctions(std::map<Vision::SpriteBoxName, std::function<int()>>&& functions);
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
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

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
    Vision::CompositeImage compImg;
    std::map<Vision::SpriteBoxName, Vision::SpriteName> staticElements;
    // TODO: transition these maps to fullyEnumeratedArrays
    std::map<Vision::SpriteBoxName, std::function<int()>> getDigitFunctions; 
    std::map<int, Vision::SpriteName> intsToImages;
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

  // Function which builds and displays the proceduralClock - adds the 4 core digits on top
  // of any quadrant images passed into the function
  void BuildAndDisplayProceduralClock(Vision::CompositeImageLayer::ImageMap&& imageMap);

  // Updates the target face within lifetime params - returns the member face for checking success inline
  SmartFaceID UpdateTargetFace();




};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorProceduralClock_H__
