/**
 * File: BehaviorDevTurnInPlaceTest.h
 *
 * Author: Matt Michini
 * Date:   05/22/2017
 *
 * Description: Simple test for TurnInPlace action at various speeds/accels, etc.
 *              Test configuration defined in the behavior's json file.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevTurnInPlaceTest_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevTurnInPlaceTest_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
// forward decl:
class CompoundActionSequential;
  
class BehaviorDevTurnInPlaceTest : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevTurnInPlaceTest() { }
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  friend class BehaviorFactory;
  BehaviorDevTurnInPlaceTest(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

private:

  struct sTest {
    sTest(float angle_deg, float speed_deg_per_sec, float accel_deg_per_sec2, float tol_deg) :
    angle_deg(angle_deg) ,
    speed_deg_per_sec(speed_deg_per_sec) ,
    accel_deg_per_sec2(accel_deg_per_sec2) ,
    tol_deg(tol_deg)
    {}
    
    float angle_deg = 0.f;
    float speed_deg_per_sec = 0.f;
    float accel_deg_per_sec2 = 0.f;
    float tol_deg = 0.f;
    
    // total times this test has run in the current behavior
    uint8_t nTimesRun = 0;
    
    // Reset the test but keep its config
    void Reset()
    {
      nTimesRun = 0;
    }
  };

  struct InstanceConfig {
    InstanceConfig();
    // Holds all of the tests to run
    std::vector<sTest> tests;

    // Optional gap between tests
    float   gapBetweenTests_s;
    // How many times to run each test?
    uint8_t nRunsPerTest;
    bool    loopForever;
  };

  struct DynamicVariables {
    DynamicVariables();
    // Current test index
    int testInd = 0;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  virtual void OnBehaviorActivated() override;

  virtual void OnBehaviorDeactivated() override;
  
  void Reset();

  CompoundActionSequential* GenerateTestAction(const Pose3d& robotPose, const int testInd) const;
  
  void ActionCallback();
  
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevTurnInPlaceTest_H__
