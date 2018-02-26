/**
 * File: BehaviorNameObject.h
 *
 * Author: ross
 * Created: 2018-02-25
 *
 * Description: the robot will name the object
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorNameObject__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorNameObject__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <thread>

namespace Anki {
  
namespace Vision {
class ImageRGB;
class ObjectDetector;
class ImageCache;
}
  
namespace Cozmo {
  
class EncodedImage;

class BehaviorNameObject : public ICozmoBehavior
{
public: 
  virtual ~BehaviorNameObject();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorNameObject(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override ;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
  virtual void InitBehavior() override;

private:
  
  
  
  void ClassifierThread();
  
  // Helper to deal with the fact the detector runs asynchronously.
  // This just waits for it to finish.
  Result DetectionHelper(Vision::ObjectDetector& detector,
                         Vision::ImageCache& imageCache,
                         std::vector<std::string>& objectNames) const;

  struct InstanceConfig {
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  std::unique_ptr<EncodedImage> _encodedImage;
  
  std::mutex _mutex;
  std::condition_variable _condition;
  std::unique_ptr<std::thread> _thread;
  bool _runThread=true;
  bool _imageAvailable=false;
  std::unique_ptr<Vision::ImageRGB> _image;
  std::string _detectionResult;
  
  bool _madeAttempt = false;
  
};

}
}

#endif
