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

namespace {
const float kNotRunningScore = 0.25f;
const float kRunningScore = 0.5f;
}

namespace Anki{
namespace Cozmo{

class TestBehavior : public IBehavior
{
public:
  
  TestBehavior(const Json::Value& config)
  : IBehavior(config)
  {
  }
  
  bool _inited = false;
  int _numUpdates = 0;
  bool _stopped = false;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override{
    auto robotExternalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
    if(robotExternalInterface != nullptr) {
      SubscribeToTags({EngineToGameTag::Ping});
    }
  }
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;
  }
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override {
    _inited = true;
    return RESULT_OK;
  }
  
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override {
    _numUpdates++;
    return Status::Running;
  }
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override {
    _stopped = true;
  }
  
  int _alwaysHandleCalls = 0;
  virtual void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override {
    _alwaysHandleCalls++;
  }
  
  int _handleWhileRunningCalls = 0;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override {
    _handleWhileRunningCalls++;
  }
  
  int _handleWhileNotRunningCalls = 0;
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override {
    _handleWhileNotRunningCalls++;
  }
  
  int _calledVoidFunc = 0;
  void Foo() {
    _calledVoidFunc++;
  }
  
  int _calledRobotFunc = 0;
  void Bar(BehaviorExternalInterface& behaviorExternalInterface) {
    _calledRobotFunc++;
  }
  
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
  
  virtual float EvaluateRunningScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return kRunningScore;
  }
  virtual float EvaluateScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return kNotRunningScore;
  }
  
};
  
}
}


