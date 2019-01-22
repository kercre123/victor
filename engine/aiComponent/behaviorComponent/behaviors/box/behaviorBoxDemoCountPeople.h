/**
 * File: BehaviorBoxDemoCountPeople.h
 *
 * Author: Andrew Stein
 * Created: 2019-01-14
 *
 * Description: Uses local neural net person classifier to trigger cloud object/person detection
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
#pragma once

#include "coretech/vision/engine/image.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoCountPeople : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoCountPeople();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoCountPeople(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
private:

  struct InstanceConfig {
    InstanceConfig();
    float visionRequestTimeout_sec = 10.f;
    float waitTimeBetweenImages_sec = 1.f;
    int   saveQuality = 90; // set to zero to disable
  };

  struct DynamicVariables {
    DynamicVariables();
    RobotTimeStamp_t startImageTime_ms = 0;
    bool isWaitingForMotion = false;
    bool motionObserved = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  Vision::ImageRGB565 _dispImg;
  
  void TransitionToWaitingForMotion();
  void TransitionToWaitingForPeople();
  void TransitionToWaitingForCloud();
  
  void DrawText3Lines(const std::array<std::string,3>& strings, const ColorRGBA& color);
  bool DrawSalientPoints(const std::list<Vision::SalientPoint>& salientPoints);
  void DrawSalientPointsOnSavedImage(const std::list<Vision::SalientPoint>& salientPoints) const;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
