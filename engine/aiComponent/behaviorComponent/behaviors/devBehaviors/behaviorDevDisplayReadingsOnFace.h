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

#include "cannedAnimLib/proceduralFace/proceduralFace.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/vision/engine/image_impl.h"


namespace Anki {
namespace Vector {

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
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

private:
  struct InstanceConfig {
    InstanceConfig();
  };


  struct DynamicVariables{
    DynamicVariables();
    // autolayout and image variables
    Vision::Image image;
    Point2f       autoLayoutPoint;
    // for tracking peripheral motion 
    bool          motionIsValid;
    
    ExternalInterface::RobotObservedMotion motionMessage;
  };

  InstanceConfig   _iConfig;
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
