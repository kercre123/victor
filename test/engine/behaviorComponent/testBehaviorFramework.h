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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/robotDataLoader.h"
#include "clad/types/behaviorComponent/userIntent.h"


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
void DoBehaviorInterfaceTicks(Robot& robot, ICozmoBehavior& behavior, int num=1);
void DoBehaviorComponentTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorComponent& behaviorComponent, int num);

void InjectBehaviorIntoStack(ICozmoBehavior& behavior, TestBehaviorFramework& testFramework);

// return true if the stacks a and b "match" with arbitrary prefix. In other words, check from the back of the
// stacks so that a/b/c/d would match c/d
bool CheckStackSuffixMatch(const std::vector<IBehavior*>& a, const std::vector<IBehavior*>& b);

void IncrementBaseStationTimerTicks();
void InjectValidDelegateIntoBSM(TestBehaviorFramework& testFramework,
                                IBehavior* delegator,
                                IBehavior* delegated,
                                bool shouldMarkAsEnteredScope = true);

void InjectAndDelegate(TestBehaviorFramework& testFramework,
                       IBehavior* delegator,
                       IBehavior* delegated);

using BEIComponentMap = std::map<BEIComponentID, void*>;
void InitBEIPartial( const BEIComponentMap& map, BehaviorExternalInterface& bei );

class TestBehaviorFramework{
public:
  // Create the test behavior framework with an appropriate robot
  TestBehaviorFramework(int robotID = 1,
                        CozmoContext* context = cozmoContext);
  ~TestBehaviorFramework();
  Robot& GetRobot(){ assert(_robot); return *_robot;}


  void InitializeStandardBehaviorComponent(IBehavior* baseBehavior = nullptr,
                                           std::function<void(const BehaviorComponent::ComponentPtr&)> initializeBehavior = {},
                                           bool shouldCallInitOnBase = true);

  // Call in order to set up and initialize a standard behavior component
  void InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                           std::function<void(const BehaviorComponent::ComponentPtr&)> initializeBehavior,
                                           bool shouldCallInitOnBase,
                                           BehaviorContainer*& customContainer);

  // After calling the initializer above, the following accessors will work appropriately
  AIComponent& GetAIComponent(){ assert(_aiComponent); return *_aiComponent;}
  BehaviorComponent& GetBehaviorComponent(){ assert(_behaviorComponent); return *_behaviorComponent;}
  BehaviorExternalInterface& GetBehaviorExternalInterface(){ assert(_behaviorExternalInterface); return *_behaviorExternalInterface;}
  BehaviorSystemManager& GetBehaviorSystemManager(){ assert(_behaviorSystemManager); return *_behaviorSystemManager; }
  BehaviorContainer& GetBehaviorContainer(){ assert(_behaviorContainer); return *_behaviorContainer;}

 // Return a named behavior stack as defined in namedBehaviorStacks.json
  std::vector<IBehavior*> GetNamedBehaviorStack(const std::string& behaviorStackName);

  ///////
  // Functions which alter the behavior stack
  ///////
  
  // Grabs the data defined base behavior used in the victor experience
  void SetDefaultBaseBehavior();
  // returns the current behavior stack
  std::vector<IBehavior*> GetCurrentBehaviorStack();
  // Destroy the current behavior stack and replaces it with the new stack
  void ReplaceBehaviorStack(std::vector<IBehavior*> newStack);
  // Add a valid delegate to the stack
  void AddDelegateToStack(IBehavior* delegate);
   // Set the active behavior stack using a namedBehviorStack from namedBehaviorStacks.json
  void SetBehaviorStackByName(const std::string& behaviorStackName);

  // Delegate through all valid tree states TreeCallback is called after each delegation (and optionally takes
  // a bool which is true if the node is a leaf, false otherwise)
  void FullTreeWalk(std::map<IBehavior*,std::set<IBehavior*>>& delegateMap,
                    std::function<void(void)> evaluateTreeCallback);

  void FullTreeWalk(std::map<IBehavior*,std::set<IBehavior*>>& delegateMap,
                    std::function<void(bool)> evaluateTreeCallback = nullptr);

  // Walks the full freeplay tree to see whether the stack can occur
  static bool CanStackOccurDuringFreeplay(const std::vector<IBehavior*>& stackToBuild);

private:

  // Called once during init to load the namedBehaviorStack map from Json
  void LoadNamedBehaviorStacks();

  std::unique_ptr<BehaviorContainer> _behaviorContainer;
  std::unique_ptr<Robot> _robot;
  
  std::unordered_map<std::string, std::vector<IBehavior*>> _namedBehaviorStacks;

  // Not guaranteed to be initialized
  AIComponent*               _aiComponent;
  BehaviorComponent*         _behaviorComponent;
  BehaviorExternalInterface* _behaviorExternalInterface;
  BehaviorSystemManager*     _behaviorSystemManager;

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
  virtual void InitInternal() override;
  virtual void OnEnteredActivatableScopeInternal() override;
  virtual void UpdateInternal() override;
  virtual bool WantsToBeActivatedInternal() const override;
  virtual void OnActivatedInternal() override;
  virtual void OnDeactivatedInternal() override;
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


  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  void InitBehavior() override;

  virtual bool WantsToBeActivatedBehavior() const override {
    return true;
  }

  virtual void OnBehaviorActivated() override;

  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;

  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event) override;

  void Foo();
  void Bar();

  bool CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef);

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

};

}
}
