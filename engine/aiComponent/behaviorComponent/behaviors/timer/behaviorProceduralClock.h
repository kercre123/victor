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
namespace Vector {

class BehaviorProceduralClock : public ICozmoBehavior
{
public:
  // List of digit display boxes for external systems to specify
  // the time to put in each box
  static const std::vector<Vision::SpriteBoxName> DigitDisplayList;

  // Determine the digit to put in a given sprite box
  // offset specifies a time offset so that values can be pre-computed 
  using GetDigitsFunction = std::function<std::map<Vision::SpriteBoxName, int>(const int offset)>;
  
  // Specify how each digit on the face should be calculated - must set all 4 values in map
  void SetGetDigitFunction(GetDigitsFunction&& function);
  // Specify a callback which should be called when the clock is shown on the robot's face
  void SetShowClockCallback(std::function<void()>&& callback)
  {
    _instanceParams.showClockCallback = callback;
  }

  int GetTimeDisplayClock_sec() const { return _instanceParams.totalTimeDisplayClock_sec;}


protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorProceduralClock(const Json::Value& config);
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override final {
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    GetBehaviorOperationModifiersProceduralClock(modifiers);
  }

  virtual void GetBehaviorOperationModifiersProceduralClock(BehaviorOperationModifiers& modifiers) const {}

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override final;
  virtual void GetBehaviorJsonKeysInternal(std::set<const char*>& expectedKeys) const {};

  virtual void OnBehaviorActivated() override final;
  virtual void InitBehavior() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override { return true;}

  virtual bool ShouldDimLeadingZeros() const { return true; }

  void TransitionToTurnToFace();
  void TransitionToGetIn();
  void TransitionToShowClock();
  void TransitionToGetOut();

  // Override to display clock differently
  // Default behavior is to update the clock once per second for the length of display clock
  virtual void TransitionToShowClockInternal();

  void SetTimeDisplayClock_sec(int displayTime_sec) { _instanceParams.totalTimeDisplayClock_sec = displayTime_sec;}

  // Function which builds and displays the proceduralClock - adds the 4 core digits on top
  // of any quadrant images passed into the function
  void BuildAndDisplayProceduralClock(const int clockOffset_s = 0, const int displayOffset_ms = 0);

private:
  enum class BehaviorState{
    TurnToFace,
    GetIn,
    ShowClock,
    GetOut,
  };


  struct InstanceParams{
    std::unique_ptr<Vision::CompositeImage> compImg;
    
    // User facing properties
    AnimationTrigger getInAnim;
    AnimationTrigger getOutAnim;
    bool shouldTurnToFace = false;
    Json::Value layout;
    int totalTimeDisplayClock_sec;
    bool shouldPlayAudioOnClockUpdates = true;

    // Asset properties
    Vision::CompositeImageLayer::ImageMap staticImageMap;
    std::map<Vision::SpriteBoxName, Vision::SpriteName> staticElements;
    std::map<int, Vision::SpriteName> intsToImages;

    // Playback properties
    GetDigitsFunction getDigitFunction;
    std::function<void()> showClockCallback; 
  };

  struct LifetimeParams{
    int timeShowClockStarted = 0;
    BehaviorState currentState = BehaviorState::TurnToFace;
    SmartFaceID targetFaceID;
    bool hasBaseImageBeenSent = false;
  };

  
  InstanceParams _instanceParams;
  LifetimeParams _lifetimeParams;


  // Updates the target face within lifetime params - returns the member face for checking success inline
  SmartFaceID UpdateTargetFace();




};

} // namespace Vector
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorProceduralClock_H__
