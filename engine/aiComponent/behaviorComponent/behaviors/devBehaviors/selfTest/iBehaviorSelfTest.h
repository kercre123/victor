/**
 * File: iBehaviorSelfTest.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Base class for all self test related behaviors
 *              All self test behaviors should be written to be able to continue even after
 *              receiving unexpected things (basically conditional branches should only contain code
 *              that calls SET_RESULT)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorSelfTest_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorSelfTest_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/selfTestConfig.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/types/selfTestTypes.h"

namespace Anki {
namespace Vector {

// Macros for setting self test result codes
#define SELFTEST_SET_RESULT(result)  { \
  if(!ShouldIgnoreFailures() || result == SelfTestResultCode::SUCCESS) { \
    SetResult(result); \
    return; \
  } else { \
    PRINT_NAMED_WARNING("IBehaviorSelfTest.IgnoringFailure", \
                        "Ignoring %s failure in behavior %s", \
                        EnumToString(result), \
                        GetDebugLabel().c_str()); \
    AddToResultList(result); \
  } \
}
  
#define SELFTEST_SET_RESULT_WITH_RETURN_VAL(result, retval)  { \
  if(!ShouldIgnoreFailures() || result == SelfTestResultCode::SUCCESS) { \
    SetResult(result); \
    return retval; \
  } else { \
    PRINT_NAMED_WARNING("IBehaviorSelfTest.IgnoringFailure", \
                        "Ignoring %s failure in behavior %s", \
                        EnumToString(result), \
                        GetDebugLabel().c_str()); \
    AddToResultList(result); \
  } \
}
  
#define SELFTEST_TRY(expr, result) { if(!(expr)) {SELFTEST_SET_RESULT(result);} }
  
class IBehaviorSelfTest : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  IBehaviorSelfTest(const Json::Value& config, SelfTestResultCode timeoutCode);
  
public:

  // Returns the current result of a self test behavior
  // Will be UNKNOWN while running and either SUCCESS or something else when complete
  SelfTestResultCode GetResult() { return _result; }
  
  void Reset() { 
    _timers.clear(); 
    _result = SelfTestResultCode::UNKNOWN; 
    _lastStatus = SelfTestStatus::Running;
  }
  
  static const std::map<std::string, std::vector<SelfTestResultCode>>& GetAllSelfTestResults();
  static void ResetAllSelfTestResults();

  static const std::set<ExternalInterface::MessageEngineToGameTag>& GetFailureTags();

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override final {
    modifiers.behaviorAlwaysDelegates = false;
    GetBehaviorOperationModifiersInternal(modifiers);
  }
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const {}


  enum SelfTestStatus
  {
    Failure,
    Running,
    Complete
  };

  virtual void InitBehavior() override {InitBehaviorInternal();};  

  // Override of IBehavior functions
  virtual bool WantsToBeActivatedBehavior() const override;
  
  // Final override of OnBehaviorActivated so we can do things before the subclass inits
  // OnBehaviorActivatedInternal() is provided for the subclass to override
  virtual void OnBehaviorActivated() override final;
  
  // Final override of UpdateInternal so we can do things before the subclass updates
  // SelfTestUpdateInternal() is provided for the subclass to override if they wish
  virtual void BehaviorUpdate() override final;
  
  // Final override of HandleWhileActivated so we can do things before the subclass handles the event
  // HandleWhileActivatedInternal() is provided for the subclass to override
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override final;
  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override final;

  // Hides IBehavior's SubscribeToTags function so we can do some bookkeeping on what the subclass
  // is subscribing to
  void SubscribeToTags(std::set<EngineToGameTag>&& tags); // Hide base class function
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  
  // Hides some of ICozmoBehavior's various DelegateIfInControl functions so we can inject callbacks that
  // automatically print warnings/failures if actions fail
  bool DelegateIfInControl(IActionRunner* action, SimpleCallback callback); // Hide base class function
  bool DelegateIfInControl(IActionRunner* action, ActionResultCallback callback); // Hide base class function
  bool DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback); // Hide base class function
  

  // Virtual functions for subclasses to override
  virtual void InitBehaviorInternal() {}; 
  
  virtual Result OnBehaviorActivatedInternal() = 0;
  
  virtual SelfTestStatus SelfTestUpdateInternal() { return SelfTestStatus::Running; }
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event) { }
  
  virtual void HandleWhileActivatedInternal(const RobotToEngineEvent& event) { }

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override { }
  
  // Use the macro SELFTEST_SET_RESULT if you can instead of directly calling this function
  void SetResult(SelfTestResultCode result);

  // Add the result to the static list of self test behavior results
  void AddToResultList(SelfTestResultCode result);
  
  // Adds a timer that will call the callback when time_ms has passed
  void AddTimer(TimeStamp_t time_ms, std::function<void(void)> callback, const std::string& name = "")
    { _timers.push_back(Timer(time_ms, callback, name)); }

  // Removes the first timer with the given name
  void RemoveTimer(const std::string& name);

  // Removes all times with the given name
  void RemoveTimers(const std::string& name);

  // Adds time_ms more time to the default behavior timeout timer
  void IncreaseTimeoutTimer(TimeStamp_t time_ms);
  
  // Returns whether or not we should ignore behavior failures
  bool ShouldIgnoreFailures() const;

  // Marks us as having received the fft result
  void ReceivedFFTResult();

  // Returns and resets if we have gotten the fft result
  bool DidReceiveFFTResult();

  // Draws each element in the text vector on a separate line to the screen
  // Will make all lines the same size such that the longest can fit on the screen
  // Rotates the image by rotate_deg before displaying
  // Note: An empty string in the text vector has special behavior
  //       It will cause the element immediately before it to be drawn across multiple lines
  //       Ex: {"36", "", "Test Failed"}, Text will be drawn on "3" lines. "36" will be drawn
  //       across the first two and "Test Failed" will be drawn on the third
  static void DrawTextOnScreen(Robot& robot, const std::vector<std::string>& text,
                               ColorRGBA textColor = NamedColors::WHITE,
                               ColorRGBA bg = NamedColors::BLACK,
                               f32 rotate_deg = 0.f);

private:

  // Let WaitToStart be able to clear its timers on init so it can remove
  // the default timeout timer
  friend class BehaviorSelfTestWaitToStart;
  friend class BehaviorSelfTest;

  // Clears all timers except those marked as "DontDelete"
  void ClearTimers();

  // Simple class that will call a callback when some amount of time has passed
  class Timer
  {
  public:
    Timer(TimeStamp_t time_ms, std::function<void(void)> callback, const std::string& name = "")
    : _time_ms(BaseStationTimer::getInstance()->GetCurrentTimeStamp() + time_ms),
      _callback(callback),
      _name(name)
    {
    
    }
    
    void Tick()
    {
      if(BaseStationTimer::getInstance()->GetCurrentTimeStamp() > _time_ms &&
         _callback != nullptr)
      {
        _callback();
        
        // Nullify callback so it will only be called once since timers are still alive/exist even
        // after _time_ms has been reached
        _callback = nullptr;
      }
    }

    const std::string& GetName() const { return _name; }

    void AddTime(TimeStamp_t time_ms) { _time_ms += time_ms; }
    
  private:
    TimeStamp_t _time_ms = 0;
    std::function<void(void)> _callback = nullptr;
    std::string _name = "";
  };

  std::vector<Timer> _timers;

  SelfTestResultCode _result = SelfTestResultCode::UNKNOWN;
  
  // Set of EngineToGameTags that a subclass has subscribed to
  std::set<EngineToGameTag> _tagsSubclassSubscribeTo;

  // The last value UpdateInternal returned
  SelfTestStatus _lastStatus = SelfTestStatus::Running;

  // If the behavior has been running for too long fail with this result
  SelfTestResultCode _timeoutCode = SelfTestResultCode::TEST_TIMED_OUT;
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_IBehaviorSelfTest_H__
