/**
 * File: behaviorDevImageCapture.h
 *
 * Author: Brad Neuman
 * Created: 2017-12-12
 *
 * Description: Dev behavior to use the touch sensor to enable / disable image capture
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <list>

namespace Anki {
namespace Cozmo {

class BehaviorDevImageCapture : public ICozmoBehavior
{
  friend class BehaviorFactory;
  explicit BehaviorDevImageCapture(const Json::Value& config);

public:

  virtual ~BehaviorDevImageCapture();

  virtual bool WantsToBeActivatedBehavior() const override
  {
    return true;
  }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;
  
private:

  void BlinkLight();
  std::string GetSavePath() const;
  void SwitchToNextClass();

  std::string _imageSavePath;
  int8_t _imageSaveQuality;
  bool _saveSensorData = false;
  
  bool  _useCapTouch = false;
  float _touchStartedTime_s = -1.0f;
    
  bool _isStreaming = false;

  // backpack lights state for debug
  bool _blinkOn = false;
  float _timeToBlink = -1.0f;

  bool _wasLiftUp = false;
  std::list<std::string> _classNames;
  std::list<std::string>::const_iterator _currentClassIter;
};

}
}


#endif
