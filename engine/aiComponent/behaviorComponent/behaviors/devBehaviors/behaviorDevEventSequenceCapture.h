/**
 * File: behaviorDevEventSequenceCapture.h
 *
 * Author: Humphrey Hu
 * Created: 2018-05-14
 *
 * Description: Dev behavior to trigger a user event and capture data before and after it
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevEventSequenceCapture_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevEventSequenceCapture_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDevEventSequenceCapture : public ICozmoBehavior
{
  friend class BehaviorFactory;
  explicit BehaviorDevEventSequenceCapture(const Json::Value& config);

public:

  virtual ~BehaviorDevEventSequenceCapture();

  virtual bool WantsToBeActivatedBehavior() const override
  {
    return true;
  }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.visionModesForActiveScope->insert({VisionMode::SavingImages, EVisionUpdateFrequency::High});
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void BehaviorUpdate() override;

private:
  struct InstanceConfig {
    InstanceConfig() = default;
    std::string imageSavePath;
    int8_t      imageSaveQuality;
    bool        useCapTouch;
    
    float       sequenceSetupTime;
    float       preEventCaptureTime;
    float       postEventCaptureTime;

    std::list<std::string> classNames;
  };

  enum class SequenceState
  {
    Waiting = 0,
    Setup,
    PreEventCapture,
    PostEventCapture
  };

  struct DynamicVariables {
    DynamicVariables();

    SequenceState seqState;
    float         waitStartTime_s;
    TimeStamp_t   seqStartTimeStamp;
    TimeStamp_t   seqEventTimeStamp;
    TimeStamp_t   seqEndTimeStamp;
    int32_t       currentSeqNumber;
    bool          wasTouched;
    bool          wasLiftUp;

    std::list<std::string>::const_iterator currentClassIter;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  int32_t GetNumCurrentSequences() const;

  std::string GetRelClassSavePath() const;
  std::string GetRelSequenceSavePath() const;
  std::string GetAbsInfoSavePath() const;
  std::string GetAbsBaseSavePath() const;
  void SwitchToNextClass();

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_DevBehaviors_BehaviorDevEventSequenceCapture_H__