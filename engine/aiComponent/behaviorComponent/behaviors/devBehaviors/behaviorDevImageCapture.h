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
  friend class BehaviorContainer;
  explicit BehaviorDevImageCapture(const Json::Value& config);

public:

  virtual ~BehaviorDevImageCapture();

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override
  {
    return true;
  }

protected:

  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override { return false; }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  // Always stay running, even with no action
  virtual bool ShouldCancelWhenInControl() const override { return false; }
  
private:

  void BlinkLight(BehaviorExternalInterface& bei);
  std::string GetSavePath() const;
  void SwitchToNextClass();

  std::string _imageSavePath;
  int8_t _imageSaveQuality;
  
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
