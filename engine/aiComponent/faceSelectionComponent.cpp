/**
 * File: faceSelectionComponent.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2017-12-04
 *
 * Description: Component that assists in selecting the best face to
 * use given a set of criteria
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/faceSelectionComponent.h"

#include "engine/actions/basicActions.h"
#include "engine/faceWorld.h"
#include "engine/micDirectionHistory.h"
#include "engine/robot.h"
#include "engine/smartFaceId.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceSelectionComponent::~FaceSelectionComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID FaceSelectionComponent::GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap, const std::set<SmartFaceID>& possibleFaces) const
{
  if( possibleFaces.empty() ) {
    SmartFaceID invalidID;
    return invalidID;
  }

  if( possibleFaces.size() == 1 ) {
    return *possibleFaces.begin();
  }
  
  // Set up the scoring callback functions
  typedef std::function<int(const Vision::TrackedFace*)> calcScoreFunction;
  using FullCalcScoreArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<FaceSelectionPenaltyMultiplier, calcScoreFunction, FaceSelectionPenaltyMultiplier::Count>;
  static const FullCalcScoreArray kFullScoreCalcMap = {
    {FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians,    std::bind(&FaceSelectionComponent::CalculateBodyAngleCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians,    std::bind(&FaceSelectionComponent::CalculateHeadAngleCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::RelativeMicDirectionRadians, std::bind(&FaceSelectionComponent::CalculateMicDirectionCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::UnnamedFace,                 std::bind(&FaceSelectionComponent::CalculateUnnamedCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::TimeSinceSeen_s,             std::bind(&FaceSelectionComponent::CalculateTimeSinceSeenCost, this, std::placeholders::_1)}
  };
  DEV_ASSERT(IsSequentialArray(kFullScoreCalcMap), 
             "FaceSelectionComponent.GetBestFaceToUse.ScoreFuncsNotSequenttial");


  // add up score for all criteria
  float bestScore = std::numeric_limits<float>::max();
  SmartFaceID bestID;
  for(auto& currentID: possibleFaces){
    float currentScore = 0;
    const Vision::TrackedFace* currentFace = _faceWorld.GetFace(currentID);
    if( nullptr == currentFace ) {
      continue;
    }

    for(const auto& criteriaPair: criteriaMap){
      const auto& scoringFunc = kFullScoreCalcMap[Util::EnumToUnderlying(criteriaPair.first)].Value();
      const float criteriaScore = scoringFunc(currentFace);
      const float criteriaFactor = criteriaPair.second;
      currentScore += (criteriaScore * criteriaFactor);
    }
    
    if(currentScore < bestScore){
      bestScore = currentScore;
      bestID = currentID;
    }
  } // end possible faces

  return bestID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceSelectionComponent::FaceSelectionPenaltyMultiplier FaceSelectionComponent::FaceSelectionPenaltyFromString(const std::string& str)
{
  using ID = FaceSelectionComponent::FaceSelectionPenaltyMultiplier;
  if(str =="RelativeBodyAngleRadians")   { return ID::RelativeBodyAngleRadians;}
  if(str =="RelativeHeadAngleRadians")   { return ID::RelativeHeadAngleRadians;}
  if(str =="RelativeMicDirectionRadians"){ return ID::RelativeMicDirectionRadians;}
  if(str =="UnnamedFace")                { return ID::UnnamedFace;}
  if(str =="TimeSinceSeen_s")            { return ID::TimeSinceSeen_s;}

  DEV_ASSERT(false, "FaceSelectionComponent.FaceSelectionPenaltyFromString.NoMatch");
  return ID::Count;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateHeadAngleCost(const Vision::TrackedFace* currentFace) const
{
  Pose3d poseWrtRobot;    
  if( ! currentFace->GetHeadPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot) ) {
    // no transform, probably a different origin
    return 0.0f;
  }

  Radians absHeadTurnAngle = TurnTowardsPoseAction::GetAbsoluteHeadAngleToLookAtPose(poseWrtRobot.GetTranslation());
  Radians relHeadTurnAngle = absHeadTurnAngle - _robot.GetComponent<FullRobotPose>().GetHeadAngle();
  return relHeadTurnAngle.getAbsoluteVal().ToFloat();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateBodyAngleCost(const Vision::TrackedFace* currentFace) const
{
  Pose3d poseWrtRobot;    
  if( ! currentFace->GetHeadPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot) ) {
    // no transform, probably a different origin
    return 0.0f;
  }

  Radians relBodyTurnAngle = TurnTowardsPoseAction::GetRelativeBodyAngleToLookAtPose(poseWrtRobot.GetTranslation());
  return relBodyTurnAngle.getAbsoluteVal().ToFloat();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateMicDirectionCost(const Vision::TrackedFace* currentFace) const
{
  // Recent direction is provided in terms of an index 0 - 11 coresponding to the face of a clock
  // these calculations assume that we've done the work to normalize recent direction to already
  // be relative to robot
  MicDirectionIndex dirIdx = _micDirectionHistory.GetRecentDirection();
  // Only care about relative turn in either direction
  dirIdx = dirIdx < 6 ? dirIdx : dirIdx - 6;
  const float radiansPerIdx = (2.0f/12.0f);
  return radiansPerIdx  * dirIdx;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateUnnamedCost(const Vision::TrackedFace* currentFace) const
{
  return currentFace->HasName() ? 0.0f : 1;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateTimeSinceSeenCost(const Vision::TrackedFace* currentFace) const
{
  return currentFace->GetTimeStamp()/1000.0f;
}

} // namespace Cozmo
} // namespace Anki
