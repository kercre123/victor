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

#include "coretech/vision/engine/imageCache.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <list>

namespace Anki {
namespace Vector {

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
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;
  
private:
  struct InstanceConfig {
    InstanceConfig();
    std::string               imageSavePath;
    int8_t                    imageSaveQuality;
    Vision::ImageCache::Size  imageSaveSize;
    bool                      useCapTouch;
    bool                      saveSensorData;

    std::list<std::string> classNames;
    std::list<VisionMode>  visionModesBesidesSaving;
  };

  struct DynamicVariables {
    DynamicVariables();
    float touchStartedTime_s;
    
    // backpack lights state for debug
    bool  blinkOn;
    float timeToBlink;

    bool  isStreaming;
    bool  wasLiftUp;

    std::list<std::string>::const_iterator currentClassIter;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void BlinkLight();
  std::string GetSavePath() const;
  void SwitchToNextClass();

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__
