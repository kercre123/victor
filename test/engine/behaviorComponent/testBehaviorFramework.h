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


#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/robotDataLoader.h"


extern Anki::Cozmo::CozmoContext* cozmoContext;

#define DoTicks(testFramework, r, b, n) do { SCOPED_TRACE(__LINE__); DoTicks_(testFramework, r, b, n); } while(0)
#define DoTicksToComplete(testFramework, r, b, n) do { SCOPED_TRACE(__LINE__); DoTicks_(testFramework,r, b, n, true); } while(0)

//////////
/// Setup a test behavior class that tracks data for testing
//////////
namespace Anki{
namespace Cozmo{
class AIComponent;
class BehaviorExternalInterface;
class BehaviorSystemManager;
class CozmoContext;
class Robot;
class BehaviorEventComponent;
class TestBehaviorWithHelpers;
class TestBehaviorFramework;

// Function to do ticks on a behavior
void DoTicks_(TestBehaviorFramework& testFramework, Robot& robot, TestBehaviorWithHelpers& behavior, int num, bool expectComplete = false);
void DoBehaviorInterfaceTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorExternalInterface& behaviorExternalInterface, int num=1);
void DoBehaviorComponentTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorComponent& behaviorComponent, int num);

void InjectBehaviorIntoStack(ICozmoBehavior& behavior, TestBehaviorFramework& testFramework);

void IncrementBaseStationTimerTicks(int numTicks = 1);
void InjectValidDelegateIntoBSM(BehaviorSystemManager& bsm,
                                IBehavior* delegator,
                                IBehavior* delegated,
                                bool shouldMarkAsEnterdScope = true);
  
void InjectAndDelegate(BehaviorSystemManager& bsm,
                       IBehavior* delegator,
                       IBehavior* delegated);
  
class TestBehaviorFramework{
public:
  // Create the test behavior framework with an appropriate robot
  TestBehaviorFramework(int robotID = 1,
                        CozmoContext* context = nullptr);
  Robot& GetRobot(){ assert(_robot); return *_robot;}

  
  void InitializeStandardBehaviorComponent(IBehavior* baseBehavior = nullptr,
                                           std::function<void(const BehaviorComponent::ComponentsPtr&)> initializeBehavior = {},
                                           bool shouldCallInitOnBase = true);
  
  // Call in order to set up and initialize a standard behavior component
  void InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                           std::function<void(const BehaviorComponent::ComponentsPtr&)> initializeBehavior,
                                           bool shouldCallInitOnBase,
                                           BehaviorContainer*& customContainer);
  
  // After calling the initializer above, the following accessors will work appropriately
  AIComponent& GetAIComponent(){ assert(_aiComponent); return *_aiComponent;}
  BehaviorComponent& GetBehaviorComponent(){ assert(_behaviorComponent); return *_behaviorComponent;}
  BehaviorExternalInterface& GetBehaviorExternalInterface(){ assert(_behaviorExternalInterface); return *_behaviorExternalInterface;}
  BehaviorSystemManager& GetBehaviorSystemManager(){ assert(_behaviorSystemManager); return *_behaviorSystemManager; }
  BehaviorContainer& GetBehaviorContainer(){ assert(_behaviorContainer); return *_behaviorContainer;}
  
private:
  std::unique_ptr<Robot> _robot;
  std::unique_ptr<BehaviorContainer> _behaviorContainer;
  
  // Not gaurenteed to be initialized
  BehaviorComponent*         _behaviorComponent;
  BehaviorExternalInterface* _behaviorExternalInterface;
  BehaviorSystemManager*     _behaviorSystemManager;
  AIComponent*               _aiComponent;
  
};

// An implementation of BSBehavior that has tons of power vested to it
// so that reasonably arbitrary tests can be written easily
class TestSuperPoweredBehavior : public IBehavior
{
public:
  TestSuperPoweredBehavior(): IBehavior("TestSuperPoweredBehavior"){};
  
  void SetBehaviorContainer(BehaviorContainer& bc){ _bc = &bc;}

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
  BehaviorContainer* _bc;
  
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
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  void Foo();
  void Bar(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef);
  
  void CallIncreaseScoreWhileControlDelegated(float extraScore) { IncreaseScoreWhileControlDelegated(extraScore); }
  
  bool CallDelegateIfInControlExternalCallback1(Robot& robot,
                                        bool& actionCompleteRef,
                                        ICozmoBehavior::RobotCompletedActionCallback callback);
  
  bool CallDelegateIfInControlExternalCallback2(Robot& robot,
                                        bool& actionCompleteRef,
                                        ICozmoBehavior::ActionResultCallback callback);
  
  bool CallDelegateIfInControlInternalCallbackVoid(Robot& robot,
                                           bool& actionCompleteRef);
  bool CallDelegateIfInControlInternalCallbackRobot(Robot& robot,
                                            bool& actionCompleteRef);
  
  bool CallCancelDelegates() { return CancelDelegates(); }
  bool CallCancelDelegates(bool val) { return CancelDelegates(val); }
  
protected:
  virtual float EvaluateActivatedScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual float EvaluateScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
};
  

class TestBehaviorWithHelpers : public ICozmoBehavior
{
public:
  
  TestBehaviorWithHelpers(const Json::Value& config)
  : ICozmoBehavior(config)
  {
  }
  
  enum class UpdateResult {
    UseBaseClass,
    Running,
    Complete
  };
  
  // for debugging, controls what UpdateInternal will return
  void SetUpdateResult(UpdateResult res);
  
  void DelegateToHelperOnNextUpdate(HelperHandle handleToRun,
                                    SimpleCallbackWithRobot successCallback,
                                    SimpleCallbackWithRobot failureCallback);
  void StopHelperOnNextUpdate();
  
  void SetActionToRunOnNextUpdate(IActionRunner* action);
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  virtual bool CarryingObjectHandledInternally() const override;
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void  OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  bool _lastDelegateSuccess = false;
  bool _lastDelegateIfInControlResult = false;
  int _updateCount = 0;
  
private:
  UpdateResult _updateResult = UpdateResult::UseBaseClass;
  IActionRunner* _nextActionToRun = nullptr;
  bool _stopHelper = false;
  HelperHandle _helperHandleToDelegate;
  SimpleCallbackWithRobot _successCallbackToDelegate;
  SimpleCallbackWithRobot _failureCallbackToDelegate;
};

////////////////////////////////////////////////////////////////////////////////
// Test Helper which runs actions and delegates to other helpers
////////////////////////////////////////////////////////////////////////////////

static int _TestHelper_g_num = 0;

class TestHelper : public IHelper
{
public:
  
  TestHelper(BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior, const std::string& name = "");
  
  void SetActionToRunOnNextUpdate(IActionRunner* action);
  
  void StartAutoAction(BehaviorExternalInterface& behaviorExternalInterface);
  
  void StopAutoAction();
  
  virtual void OnActivatedHelper(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void StopInternal(bool isActive) override;
  virtual bool ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual ICozmoBehavior::Status InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual ICozmoBehavior::Status UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  void CheckActions();
  
  WeakHelperHandle GetSubHelper();
  TestHelper* GetSubHelperRaw();
  
  ICozmoBehavior::Status _updateResult = ICozmoBehavior::Status::Running;
  mutable bool _cancelDelegates = false;
  bool _delegateAfterAction = false;
  bool _thisSucceedsOnActionSuccess = false;
  bool _immediateCompleteOnSubSuccess = false;
  
  int _initOnStackCount = 0;
  int _stopCount = 0;
  mutable int _shouldCancelCount = 0;
  int _initCount = 0;
  int _updateCount = 0;
  int _actionCompleteCount = 0;
  
  std::string _name;
  
private:
  IActionRunner* _nextActionToRun = nullptr;
  
  bool _selfActionDone = false;
  
  ICozmoBehavior& _behavior;
  
  WeakHelperHandle _subHelper;
  TestHelper* _subHelperRaw = nullptr;
};


  
}
}


