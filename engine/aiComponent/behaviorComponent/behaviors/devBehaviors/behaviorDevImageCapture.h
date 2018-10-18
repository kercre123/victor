/**
 * File: behaviorDevImageCapture.h
 *
 * Author: Brad Neuman
 * Created: 2017-12-12
 *
 * Description: Dev behavior to use the touch sensor or backpack button to enable / disable image capture
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__

#include "coretech/vision/engine/imageCache.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/imageTypes.h"

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

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;
  
private:
  struct InstanceConfig {
    InstanceConfig();
    std::string            imageSavePath;
    std::string            imageSavePrefix;
    int8_t                 imageSaveQuality;
    Vision::ImageCacheSize imageSaveSize;
    bool                   useSavePrefix;
    bool                   useCapTouch;
    bool                   saveSensorData;
    bool                   useShutterSound;
    s32                    numImagesPerCapture;
    std::pair<f32,f32>     distanceRange_mm;
    std::pair<f32,f32>     headAngleRange_rad;
    std::pair<f32,f32>     bodyAngleRange_rad;
    
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
    
    s32   imagesSaved;

    Radians startingBodyAngle_rad;
    Radians startingHeadAngle_rad;
    
    std::list<std::string>::const_iterator currentClassIter;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  void BlinkLight();
  std::string GetSavePath() const;
  void SwitchToNextClass();

  void SaveImages(const ImageSendMode sendMode);
  void MoveToNewPose();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevImageCapture_H__
