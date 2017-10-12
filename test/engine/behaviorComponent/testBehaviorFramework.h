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


#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;

namespace Anki{
namespace Cozmo{
class StateChangeComponent;
}
}

namespace TestBehaviorFramework{
std::unique_ptr<Anki::Cozmo::Robot> CreateRobot(int robotID);
std::unique_ptr<Anki::Cozmo::BehaviorContainer> CreateBehaviors();
void GenerateCoreBehaviorTestingComponents(std::unique_ptr<Anki::Cozmo::Robot>& robot,
                                           std::unique_ptr<Anki::Cozmo::BehaviorComponent>& behaviorComp,
                                           std::unique_ptr<Anki::Cozmo::BehaviorContainer>& bc,
                                           std::unique_ptr<Anki::Cozmo::StateChangeComponent>& stateChangeComp,
                                           std::unique_ptr<Anki::Cozmo::BehaviorExternalInterface>& bei);
}


//////////
/// Setup a test behavior class that tracks data for testing
//////////
namespace Anki{
namespace Cozmo{
  
  
// An implementation of BSRunnable that has tons of power vested to it
// so that reasonably arbitrary tests can be written easily
class TestSuperPoweredRunnable : public IBehavior
{
public:
  TestSuperPoweredRunnable(const BehaviorContainer& bc): IBehavior("TestSuperPoweredRunnable"), _bc(bc){};
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
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
  

class TestBehavior : public ICozmoBehavior
{
public:
  TestBehavior(const Json::Value& config)
  : ICozmoBehavior(config)
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
  
  bool CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef);
  
  void CallIncreaseScoreWhileActing(float extraScore) { IncreaseScoreWhileActing(extraScore); }
  
  bool CallStartActingExternalCallback1(Robot& robot,
                                        bool& actionCompleteRef,
                                        ICozmoBehavior::RobotCompletedActionCallback callback);
  
  bool CallStartActingExternalCallback2(Robot& robot,
                                        bool& actionCompleteRef,
                                        ICozmoBehavior::ActionResultCallback callback);
  
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


