/**
 * File: BehaviorSinging.h
 *
 * Author: Al Chaussee
 * Created: 4/25/17
 *
 * Description: Behavior for Cozmo Sings
 *              Plays GetIn, Song, and GetOut animation sequence
 *              While running, cubes can be shaken to modify audio parameters for the song
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSinging_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSinging_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/cubes/cubeAccelListeners/shakeListener.h"
#include "engine/engineTimeStamp.h"

#include "clad/audio/audioSwitchTypes.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorSinging : public ICozmoBehavior
{
protected:
  friend class BehaviorFactory;
  BehaviorSinging(const Json::Value& config);
  
public:
  virtual ~BehaviorSinging();
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
    
private:
  using AudioSwitchGroup = AudioMetaData::SwitchState::SwitchGroupType;
  AudioSwitchGroup _audioSwitchGroup;
  AudioMetaData::SwitchState::GenericSwitch _audioSwitch;
  AnimationTrigger _songAnimTrigger = AnimationTrigger::Count;
  
  // Vector of cubeAccelListeners that are running each light cube's accel data
  std::vector<std::pair<ObjectID, std::shared_ptr<CubeAccelListeners::ICubeAccelListener>>> _cubeAccelListeners;
  
  // Simple class for keep track of rolling averages
  class RollingAverage
  {
  public:
    RollingAverage() { Reset(); }
  
    inline void Update(const float amount) { avg += (amount - avg)/(count*1.f); ++count; }
    
    inline void Reset() { avg = 0.f; count = 1; }
    
    inline float Avg() const { return avg; }
    
  private:
    float avg = 0.f;
    u32 count = 1;
  };
  
  // Map to keep track of each object's average shake amount each tick
  std::map<ObjectID, RollingAverage> _objectShakeAverages;
  
  // Output of low pass filter on vibrato scale
  float _vibratoScaleFilt = 0;
  
  // Keep track of when cubes start being shaken
  EngineTimeStamp_t _cubeShakingStartTime_ms = 0;
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSinging_H__
