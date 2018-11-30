/**
 * File: behaviorSelfTest.h
 *
 * Author: Al Chaussee
 * Created: 11/15/2018
 *
 * Description: Runs individual steps (behaviors) of the self test and manages switching between them and dealing
 *              with failures
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_DevBehaviors_SelfTest_BehaviorSelfTest_H__
#define __Cozmo_Basestation_BehaviorSystem_DevBehaviors_SelfTest_BehaviorSelfTest_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class IBehaviorSelfTest;
class Robot;

class BehaviorSelfTest : public ICozmoBehavior
{
public:
  
  BehaviorSelfTest(const Json::Value& config);
  virtual ~BehaviorSelfTest() {};
  
  virtual void BehaviorUpdate() override;
  
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.visionModesForActiveScope->insert({VisionMode::DetectingMarkers, EVisionUpdateFrequency::High});
  }

  virtual void InitBehavior() override;

  // Override of IBehavior functions
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  //virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;
  
private:
  
  // Handle the playpen result and finish up the test
  void HandleResult(FactoryTestResultCode result);

  // Display appropriate stuff on the face and backpack lights depending on the result
  void DisplayResult(FactoryTestResultCode result);

  // Reset all playpen behaviors and state
  void Reset();
  
  using SelfTestBehavior = std::shared_ptr<IBehaviorSelfTest>;
  
  SelfTestBehavior _currentBehavior = nullptr;
  
  std::vector<SelfTestBehavior>::iterator _currentSelfTestBehaviorIter;
  std::vector<SelfTestBehavior> _selfTestBehaviors;
  
  //std::vector<u32> _behaviorStartTimes;
  
  //IMUTempDuration _imuTemp;

  //bool _startTest = false;
    
  std::vector<::Signal::SmartHandle> _signalHandles;

  bool _waitForButtonToEndTest = false;
  bool _restartOnButtonPress = false;
  bool _buttonPressed = false;
};

} // namespace Cozmo
} // namespace Anki

#endif
