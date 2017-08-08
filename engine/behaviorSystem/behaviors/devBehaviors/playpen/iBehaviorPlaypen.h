/**
 * File: iBehaviorPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__

#include "engine/behaviorSystem/behaviors/iBehavior.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

#define PLAYPEN_SET_RESULT(result)  { \
                                      if(!ShouldIgnoreFailures() || result == FactoryTestResultCode::SUCCESS) { \
                                        SetResult(result); \
                                        return; \
                                      } else { \
                                        PRINT_NAMED_WARNING("IBehaviorPlaypen.IgnoringFailure", "Ignoring %s failure in behavior %s", EnumToString(result), GetIDStr().c_str()); \
                                      } \
                                    }
  
#define PLAYPEN_TRY(expr, result) { \
                                    if(!(expr)) {PLAYPEN_SET_RESULT(result);}\
                                  }
  
class FactoryTestLogger;
namespace ExternalInterface {
  class CameraCalibration;
}
  
class IBehaviorPlaypen : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  IBehaviorPlaypen(Robot& robot, const Json::Value& config);
  
public:

  // TODO: Pass in results struct to populate
  FactoryTestResultCode GetResults() { GetResultsInternal(); return _result; }
  
  void Reset() { Stop(); _timers.clear(); _result = FactoryTestResultCode::UNKNOWN; }

protected:
  
  // Override of IBehavior functions
  virtual bool IsRunnableInternal(const BehaviorPreReqPlaypen& preReq) const override;
  virtual Result ResumeInternal(Robot& robot) override { return RESULT_FAIL; }
  virtual Result InitInternal(Robot& robot) override final;
  virtual BehaviorStatus UpdateInternal(Robot& robot) override final;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override final;
  void SubscribeToTags(std::set<EngineToGameTag>&& tags); // Hide base class function
  bool StartActing(IActionRunner* action, SimpleCallback callback); // Hide base class function
  

  // Virtual functions for subclasses to override
  virtual void GetResultsInternal() { }
  virtual Result InternalInitInternal(Robot& robot) = 0;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) { return IBehavior::UpdateInternal(robot); }
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) { }
  
  FactoryTestLogger& GetLogger() { return *_factoryTestLogger; }
  void WriteToStorage(Robot& robot, NVStorage::NVEntryTag tag,const u8* data, size_t size);
  
  // Use the macro PLAYPEN_SET_RESULT if you can instead of directly calling this function
  void SetResult(FactoryTestResultCode result) { _result = result; _timers.clear(); }
  
  void AddTimer(TimeStamp_t time_ms, std::function<void(void)> callback)
    { _timers.push_back(Timer(time_ms, callback)); }
  
  // Getters for configurable params via console vars
  bool ShouldIgnoreFailures() const;
  
private:

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
  
  std::set<EngineToGameTag> _tagsSubclassSubscribeTo;
  
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPlaypen_H__
