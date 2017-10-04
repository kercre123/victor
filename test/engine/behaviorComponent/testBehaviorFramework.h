/**
* File: testBehaviorFramework
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Framework that provides helper classes and functions for
* testing behaviors
*
* Copyright: Anki, Inc. 2017
*
**/


#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

using namespace Anki::Cozmo;

extern Anki::Cozmo::CozmoContext* cozmoContext;

namespace TestBehaviorFramework{
std::unique_ptr<Robot> CreateRobot(int robotID);
std::unique_ptr<BehaviorContainer> CreateBehaviors();
void GenerateCoreBehaviorTestingComponents(std::unique_ptr<Robot>& robot,
                                           std::unique_ptr<BehaviorContainer>& bc,
                                           std::unique_ptr<BehaviorExternalInterface>& bei);
}


//////////
/// Setup a test behavior class that tracks data for testing
//////////
namespace Anki{
namespace Cozmo{
  
  
// An implementation of BSRunnable that has tons of power vested to it
// so that reasonably arbitrary tests can be written easily
class TestSuperPoweredRunnable : public IBSRunnable
{
public:
  TestSuperPoweredRunnable(const BehaviorContainer& bc): IBSRunnable("TestSuperPoweredRunnable"), _bc(bc){};
  
  virtual void GetAllDelegates(std::set<IBSRunnable*>& delegates) const override;
  
protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnEnteredActivatableScopeInternal() override;
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnLeftActivatableScopeInternal() override;
  
private:
  const BehaviorContainer& _bc;
  
};
  

class TestBehavior : public IBehavior
{
public:
  
  TestBehavior(const Json::Value& config)
  : IBehavior(config)
  {
  }
  
  constexpr static const float kNotRunningScore = 0.25f;
  constexpr static const float kRunningScore = 0.5f;
  
  bool _inited = false;
  int _numUpdates = 0;
  bool _stopped = false;
  int _alwaysHandleCalls = 0;
  int _handleWhileRunningCalls = 0;
  int _handleWhileNotRunningCalls = 0;
  int _calledVoidFunc = 0;
  int _calledRobotFunc = 0;
  

  
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;
  }
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  void Foo();
  void Bar(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool CallStartActing(Robot& robot, bool& actionCompleteRef);
  
  void CallIncreaseScoreWhileActing(float extraScore) { IncreaseScoreWhileActing(extraScore); }
  
  bool CallStartActingExternalCallback1(Robot& robot,
                                        bool& actionCompleteRef,
                                        IBehavior::RobotCompletedActionCallback callback);
  
  bool CallStartActingExternalCallback2(Robot& robot,
                                        bool& actionCompleteRef,
                                        IBehavior::ActionResultCallback callback);
  
  bool CallStartActingInternalCallbackVoid(Robot& robot,
                                           bool& actionCompleteRef);
  bool CallStartActingInternalCallbackRobot(Robot& robot,
                                            bool& actionCompleteRef);
  
  bool CallStopActing() { return StopActing(); }
  bool CallStopActing(bool val) { return StopActing(val); }
  
protected:
  virtual float EvaluateRunningScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual float EvaluateScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
};
  
}
}


