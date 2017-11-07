/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description: Base class for all playpen related behaviors
 *              All Playpen behaviors should be written to be able to continue even after
 *              receiving unexpected things (basically conditional branches should only contain code
 *              that calls SET_RESULT) E.g. Even if camera calibration is outside our threshold we should
 *              still be able to continue running through the rest of playpen.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/playpenConfig.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// Macros for setting playpen result codes
#define PLAYPEN_SET_RESULT(result)  { \
  if(!ShouldIgnoreFailures() || result == FactoryTestResultCode::SUCCESS) { \
    SetResult(result); \
    return; \
  } else { \
    PRINT_NAMED_WARNING("IBehaviorPlaypen.IgnoringFailure", \
                        "Ignoring %s failure in behavior %s", \
                        EnumToString(result), \
                        GetIDStr().c_str()); \
    AddToResultList(result); \
  } \
}
  
#define PLAYPEN_SET_RESULT_WITH_RETURN_VAL(result, retval)  { \
  if(!ShouldIgnoreFailures() || result == FactoryTestResultCode::SUCCESS) { \
    SetResult(result); \
    return retval; \
  } else { \
    PRINT_NAMED_WARNING("IBehaviorPlaypen.IgnoringFailure", \
                        "Ignoring %s failure in behavior %s", \
                        EnumToString(result), \
                        GetIDStr().c_str()); \
    AddToResultList(result); \
  } \
}
  
#define PLAYPEN_TRY(expr, result) { if(!(expr)) {PLAYPEN_SET_RESULT(result);} }
  
class FactoryTestLogger;

class IBehaviorPlaypen : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  IBehaviorPlaypen(const Json::Value& config);
  
public:

  // Returns the current result of a playpen behavior
  // Will be UNKNOWN while running and either SUCCESS or something else when complete
  FactoryTestResultCode GetResult() { return _result; }
  
  void Reset(BehaviorExternalInterface& behaviorExternalInterface) { 
    _timers.clear(); 
    _result = FactoryTestResultCode::UNKNOWN; 
  }
  
  static const std::map<std::string, std::vector<FactoryTestResultCode>>& GetAllPlaypenResults();
  static void ResetAllPlaypenResults();

protected:
  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override {InitBehaviorInternal(behaviorExternalInterface);};  

  // Override of IBehavior functions
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  // Playpen behaviors can not be resumed
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override { return RESULT_FAIL; }
  
  // Final override of OnBehaviorActivated so we can do things before the subclass inits
  // OnBehaviorActivatedInternal() is provided for the subclass to override
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override final;
  
  // Final override of UpdateInternal so we can do things before the subclass updates
  // PlaypenUpdateInternal() is provided for the subclass to override if they wish
  virtual BehaviorStatus UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override final;
  
  // No playpen behavior can start while carrying an object
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
  // Final override of HandleWhileActivated so we can do things before the subclass handles the event
  // HandleWhileActivatedInternal() is provided for the subclass to override
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override final;
  
  // Hides IBehavior's SubscribeToTags function so we can do some bookkeeping on what the subclass
  // is subscribing to
  void SubscribeToTags(std::set<EngineToGameTag>&& tags); // Hide base class function
  
  // Hides some of ICozmoBehavior's various DelegateIfInControl functions so we can inject callbacks that
  // automatically print warnings/failures if actions fail
  bool DelegateIfInControl(IActionRunner* action, SimpleCallback callback); // Hide base class function
  bool DelegateIfInControl(IActionRunner* action, ActionResultCallback callback); // Hide base class function
  bool DelegateIfInControl(IActionRunner* action, RobotCompletedActionCallback callback); // Hide base class function
  

  // Virtual functions for subclasses to override
  virtual void InitBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface) {}; 
  
  virtual Result OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  
  virtual BehaviorStatus PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) { return BehaviorStatus::Running; }
  
  virtual void HandleWhileActivatedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) { }
  
  FactoryTestLogger& GetLogger() { return *_factoryTestLogger; }
  
  // Attempts to write to robot storage, if enabled, and will fail with failureCode if writing fails
  void WriteToStorage(Robot& robot, 
                      NVStorage::NVEntryTag tag,
                      const u8* data, 
                      size_t size,
                      FactoryTestResultCode failureCode);
  
  // Use the macro PLAYPEN_SET_RESULT if you can instead of directly calling this function
  void SetResult(FactoryTestResultCode result);

  // Add the result to the static list of playpen behavior results
  void AddToResultList(FactoryTestResultCode result);
  
  // Adds a timer that will call the callback when time_ms has passed
  void AddTimer(TimeStamp_t time_ms, std::function<void(void)> callback)
    { _timers.push_back(Timer(time_ms, callback)); }
  
  // Returns whether or not we should ignore behavior failures
  bool ShouldIgnoreFailures() const;
  
private:

  // Let WaitToStart be able to clear its timers on init so it can remove
  // the default timeout timer
  friend class BehaviorPlaypenWaitToStart;
  void ClearTimers() { _timers.clear(); }

  // Simple class that will call a callback when some amount of time has passed
  class Timer
  {
  public:
    Timer(TimeStamp_t time_ms, std::function<void(void)> callback)
    : _time_ms(BaseStationTimer::getInstance()->GetCurrentTimeStamp() + time_ms),
      _callback(callback)
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
    
  private:
    TimeStamp_t _time_ms = 0;
    std::function<void(void)> _callback = nullptr;
  };

  std::vector<Timer> _timers;

  mutable FactoryTestLogger* _factoryTestLogger;
  
  FactoryTestResultCode _result = FactoryTestResultCode::UNKNOWN;
  
  // Set of EngineToGameTags that a subclass has subscribed to
  std::set<EngineToGameTag> _tagsSubclassSubscribeTo;
  
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
