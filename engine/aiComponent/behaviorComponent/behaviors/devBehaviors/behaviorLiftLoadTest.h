/**
 * File: behaviorLiftLoadTest.h
 *
 * Author: Kevin Yoon
 * Date:   11/11/2016
 *
 * Description: Tests the lift load detection system
 *              Saves attempt information to log webotsCtrlGameEngine/temp
 *
 *              Init conditions:
 *                - Cozmo's lift is down. A cube is optionally placed against it.
 *
 *              Behavior:
 *                - Raise and lower lift. Optionally with a cube attached
 *                - Records the result of the lift load check
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorLiftLoadTest_H__
#define __Cozmo_Basestation_Behaviors_BehaviorLiftLoadTest_H__

#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#include <fstream>

namespace Anki {
namespace Vector {

class BehaviorLiftLoadTest : public ICozmoBehavior
{
protected:
  friend class BehaviorFactory;
  BehaviorLiftLoadTest(const Json::Value& config);

public:

  virtual ~BehaviorLiftLoadTest() { }

  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  void InitBehavior() override;
private:
  enum class State {
    Init,           // Main test loop
    WaitForPutdown, // Robot was picked up, now waiting for putdown
    TestComplete    // Test complete
  };


  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<Util::RollingFileLogger> logger;
  };

  struct DynamicVariables {
    DynamicVariables();
    State currentState;
    bool  abortTest;
    bool  canRun;

    // Stats for test/attempts
    int  numLiftRaises;
    int  numHadLoad;
    bool loadStatusReceived;
    bool hasLoad;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


  void Write(const std::string& s);

  virtual void OnBehaviorActivated() override;

  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override;

  virtual void AlwaysHandleInScope(const GameToEngineEvent& event) override;

  void PrintStats();

  void SetCurrState(State s);
  const char* StateToString(const State m);
  void UpdateStateName();

};
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDockingTest_H__
