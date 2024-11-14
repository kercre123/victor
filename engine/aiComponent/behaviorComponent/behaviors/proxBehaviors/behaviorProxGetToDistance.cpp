/**
 * File: behaviorProxGetToDistance.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/11/17
 *
 * Description: Behavior that uses a graph based driving profile in order to drive
 * the robot to the requested prox sensor reading distance
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/proxBehaviors/behaviorProxGetToDistance.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/basicActions.h"
#include "engine/components/sensors/proxSensorComponent.h"

namespace Anki {
namespace Vector {
  
namespace{
  const char* kDistToSpeedKey  = "distMM_speedMM_Graph";
  const char* kGoalDistanceKey = "goalDistance_mm";
  const char* kGoalTolerenceKey = "tolerence_mm";
  const char* kEndWhenGoalReachedKey = "endWhenGoalReached";
  const float kThresholdSensedMoved_mm = 25;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProxGetToDistance::BehaviorProxGetToDistance(const Json::Value& config)
: ICozmoBehavior(config)
{
  _params.goalDistance_mm = JsonTools::ParseFloat(config,
                                                  kGoalDistanceKey,
                                                  "BehaviorProxGetToDistance.BehaviorProxGetToDistance.GoalDistance");
  _params.tolarance_mm = JsonTools::ParseFloat(config,
                                               kGoalTolerenceKey,
                                               "BehaviorProxGetToDistance.BehaviorProxGetToDistance.GoalDistance");
  _params.shouldEndWhenGoalReached = JsonTools::ParseBool(config,
                                                          kEndWhenGoalReachedKey,
                                                          "BehaviorProxGetToDistance.BehaviorProxGetToDistance.EndWhenGoalReached");
  const Json::Value& distToSpeedJSON = config[kDistToSpeedKey];
  if (!distToSpeedJSON.isNull())
  {
    _params.distMMToSpeedMMGraph.ReadFromJson(distToSpeedJSON);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProxGetToDistance::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kGoalDistanceKey,
    kGoalTolerenceKey,
    kEndWhenGoalReachedKey,
    kDistToSpeedKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorProxGetToDistance::WantsToBeActivatedBehavior() const
{
  return !IsWithinGoalTolerence();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProxGetToDistance::OnBehaviorActivated()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProxGetToDistance::OnBehaviorDeactivated()
{  

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorProxGetToDistance::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  if( !proxData.foundObject ){
    CancelSelf();
    return;
  }

  if(_params.shouldEndWhenGoalReached && 
     IsWithinGoalTolerence()){
    CancelSelf();
    return;
  }

  if(IsControlDelegated() &&
    ShouldRecalculateDrive()){
    CancelDelegates(false);
  }

  if(!IsControlDelegated()){
    const float speed_mm_s = _params.distMMToSpeedMMGraph.EvaluateY(proxData.distance_mm);
    DelegateIfInControl(new DriveStraightAction(CalculateDistanceToDrive(), speed_mm_s));
    const bool isSensorReadingValid = proxSensor.GetLatestProxData().foundObject;
    DEV_ASSERT(isSensorReadingValid, "BehaviorProxGetToDistance.BehaviorUpdate.SensorReadingInvalid");
  }

  return;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorProxGetToDistance::CalculateDistanceToDrive() const
{
  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  
  if (!proxData.foundObject) {
    PRINT_NAMED_WARNING("BehaviorProxGetToDistance.CalculateDistanceToDrive.NoObjectFound",
                        "No object found to drive to!");
    return 0.f;
  }
  
  float distanceToDrive = proxData.distance_mm - _params.goalDistance_mm;
  
  // See if there's a distance at which we want to change speed that occurs before we
  // make it all the way to our goal distance
  {
    const bool drivingForward = (distanceToDrive >= 0);
    // make sure we get something that we can actually drive to
    const float distOutsideDriveTolerence = drivingForward
        ? (proxData.distance_mm -  _params.tolarance_mm)
        : (proxData.distance_mm +  _params.tolarance_mm);
    const Util::GraphEvaluator2d::Node* closestNode = GetNodeClosestToDistance(distOutsideDriveTolerence, drivingForward);

    const bool isNodeCloser = drivingForward
        ? (closestNode != nullptr) && (closestNode->_x > _params.goalDistance_mm)
        : (closestNode != nullptr) && (closestNode->_x < _params.goalDistance_mm);

    if(isNodeCloser){
      distanceToDrive = proxData.distance_mm - closestNode->_x;
    }
  }
  
  return distanceToDrive;
  return 0.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Util::GraphEvaluator2d::Node* BehaviorProxGetToDistance::GetNodeClosestToDistance(const u16 distance, bool floor) const
{
  // Node selection
  const size_t numNodes = _params.distMMToSpeedMMGraph.GetNumNodes();
  int i= 0;
  for(; i < numNodes; i++){
    const Util::GraphEvaluator2d::Node& node = _params.distMMToSpeedMMGraph.GetNode(i);
    if(node._x >  distance){
      break;
    }
  }
  // If driving forward than we need the node before the one we selected
  if(floor){
    i--;
  }
  return &_params.distMMToSpeedMMGraph.GetNode(i);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorProxGetToDistance::ShouldRecalculateDrive()
{
  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  f32 distSqrSensedChanged_mm = 0.f;
  const bool recalculate = proxData.foundObject &&
      ComputeDistanceSQBetween(_previousProxObjectPose, proxData.objectPose, distSqrSensedChanged_mm) &&
      distSqrSensedChanged_mm > (kThresholdSensedMoved_mm * kThresholdSensedMoved_mm);
  if(recalculate){
   _previousProxObjectPose = proxData.objectPose;
  }
  return recalculate;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorProxGetToDistance::IsWithinGoalTolerence() const
{
  const auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const auto& proxData = proxSensor.GetLatestProxData();
  const bool isReadingValid = proxData.foundObject;
  if(isReadingValid){
    return Util::IsNear(proxData.distance_mm, _params.goalDistance_mm, _params.tolarance_mm);
  }
  return false;
}


} // namespace Vector
} // namespace Anki
