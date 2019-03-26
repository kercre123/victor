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

#include "clad/types/selfTestTypes.h"

namespace Anki {
namespace Vector {

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
    modifiers.visionModesForActiveScope->insert({VisionMode::Markers, EVisionUpdateFrequency::High});
  }

  virtual void InitBehavior() override;

  // Override of IBehavior functions
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void HandleWhileActivated(const GameToEngineEvent& event) override;

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override { }

private:
  
  // Handle the playpen result and finish up the test
  void HandleResult(SelfTestResultCode result);

  // Display appropriate stuff on the face and backpack lights depending on the result
  void DisplayResult(SelfTestResultCode result);

  // Reset all playpen behaviors and state
  void Reset();

  // Begin the cube connection check
  void StartCubeConnectionCheck();

  // Perform any final checks
  SelfTestResultCode DoFinalChecks();
  
  using SelfTestBehavior = std::shared_ptr<IBehaviorSelfTest>;

  // The current running self test behavior
  SelfTestBehavior _currentBehavior = nullptr;

  // List of all self test behaviors
  std::vector<SelfTestBehavior>::iterator _currentSelfTestBehaviorIter;
  std::vector<SelfTestBehavior> _selfTestBehaviors;
  
  std::vector<::Signal::SmartHandle> _signalHandles;

  // Whether or not we are waiting on button press to finish the self test
  // after the result has been displayed
  bool _waitForButtonToEndTest = false;
  bool _buttonPressed = false;

  enum class RadioScanState
  {
    None,
    WaitingForWifiResult,
    WaitingForCubeResult,
    Failed,
    Passed
  };

  RadioScanState _radioScanState = RadioScanState::None;
  
};

} // namespace Vector
} // namespace Anki

#endif
