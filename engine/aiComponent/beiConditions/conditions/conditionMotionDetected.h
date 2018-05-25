/**
* File: conditionMotionDetected.h
*
* Author: Kevin M. Karol
* Created: 1/23/18
*
* Description: Condition which is true when motion is detected
*
* Copyright: Anki, Inc. 2018
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionMotionDetected_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionMotionDetected_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Cozmo {

class BEIConditionMessageHelper;

class ConditionMotionDetected : public IBEICondition,  private IBEIConditionEventHandler
{
public:
  // constructor
  explicit ConditionMotionDetected(const Json::Value& config);
  ~ConditionMotionDetected();
  
  float GetLastObservedMotionLevel() const { return _lifetimeParams.detectedMotionLevel; }

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override
  {
    requiredVisionModes.insert( {VisionMode::DetectingMotion, EVisionUpdateFrequency::High} );
  }

  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
private:
  enum class MotionArea{
    Left,
    Right,
    Top,
    Ground,
    Any
  };

  // We currently don't have uniform thresholding/calibration on
  // motion area/messages/decay across the system, but pretend like we do
  // so that there aren't behaviors specifying arbitrary floats all over the place
  enum class MotionLevel{
    High,
    Low,
    Any
  };

  struct {
    std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
    MotionArea motionArea = MotionArea::Ground;
    MotionLevel motionLevel = MotionLevel::Low;
  } _instanceParams;

  struct {
    size_t tickCountMotionObserved = 0;
    float detectedMotionLevel = 0.0f;
  } _lifetimeParams;


  MotionArea  AreaStringToEnum(const std::string& areaStr);
  MotionLevel LevelStringToEnum(const std::string& levelStr);

  // helper functions to evaluate the appropriate parameters
  using RobotObservedMotion = ExternalInterface::RobotObservedMotion;
  bool EvaluateLeft(const RobotObservedMotion& motionObserved);
  bool EvaluateRight(const RobotObservedMotion& motionObserved);
  bool EvaluateTop(const RobotObservedMotion& motionObserved);
  bool EvaluateGround(const RobotObservedMotion& motionObserved);
};
  
} // namespace
} // namespace

#endif // endif __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionMotionDetected_H__
