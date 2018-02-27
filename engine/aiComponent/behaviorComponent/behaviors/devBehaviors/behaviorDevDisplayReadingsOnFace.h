/**
 * File: behaviorDevDisplayReadingsOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2/13/18
 *
 * Description: Dev behavior which makes it easy to write text to Victor's face so that
 * design and engineering can quickly get a sense of realtime values the robot is using for decision making
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevDisplayReadingsOnFace_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevDisplayReadingsOnFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "cannedAnimLib/proceduralFace.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/vision/engine/image_impl.h"


namespace Anki {
namespace Cozmo {

class BehaviorDevDisplayReadingsOnFace : public ICozmoBehavior
{
protected:
  friend class BehaviorFactory;
  explicit BehaviorDevDisplayReadingsOnFace(const Json::Value& config);

public:
  virtual ~BehaviorDevDisplayReadingsOnFace();

  virtual bool WantsToBeActivatedBehavior() const override { return true;}

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

private:
  static constexpr float kTopLeftCornerMagicNumber = 15.f;
  struct DynamicVariables{
    // autolayout and image variables
    Vision::Image image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
    Point2f autoLayoutPoint = Point2f(0, kTopLeftCornerMagicNumber);

    // for tracking peripheral motion 
    bool motionIsValid = false;
    ExternalInterface::RobotObservedMotion motionMessage;
  };

  DynamicVariables _dVars;

  // Utility function which will automatically layout all text passed into it 
  // along the left side of the screen as space allows
  void DrawTextWithAutoLayout(const std::string& text);

  void UpdateDisplayPeripheralMotion();
  void UpdateDisplayProxReading();



};

}
}


#endif
