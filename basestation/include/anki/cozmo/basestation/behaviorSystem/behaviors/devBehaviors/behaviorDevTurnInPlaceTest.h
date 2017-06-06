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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
// forward decl:
class CompoundActionSequential;
  
class BehaviorDevTurnInPlaceTest : public IBehavior
{
public:
  
  virtual ~BehaviorDevTurnInPlaceTest() { }
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}

protected:
  
  friend class BehaviorFactory;
  BehaviorDevTurnInPlaceTest(Robot& robot, const Json::Value& config);

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
  
  virtual Result InitInternal(Robot& robot) override;

  virtual void StopInternal(Robot& robot) override;
  
  void Reset();

  CompoundActionSequential* GenerateTestAction(Robot& robot, const int testInd) const;
  
  void ActionCallback(Robot&);

  // member variables:
  
  // Holds all of the tests to run
  std::vector<sTest> _tests;
  
  // Current test index
  int _testInd = 0;
  
  // Optional gap between tests
  float _gapBetweenTests_s = 0.f;
  
  // How many times to run each test?
  uint8_t _nRunsPerTest = 1;
  
  bool _loopForever = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevTurnInPlaceTest_H__
