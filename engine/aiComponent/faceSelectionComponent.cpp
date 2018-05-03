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
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/smartFaceId.h"

#include "util/console/consoleInterface.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kPenaltyFactorKey = "factor";
static const char* kPenaltyMultiplierKey = "multiplier";

CONSOLE_VAR(bool, kFaceSelectionDebugging, "FaceSelection", false);
}

const FaceSelectionComponent::FaceSelectionFactorMap FaceSelectionComponent::kDefaultSelectionCriteria = {
  {FaceSelectionPenaltyMultiplier::Distance_m, 4.0f},
  {FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 3.0f},
  {FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, 0.1f},
  {FaceSelectionPenaltyMultiplier::TimeSinceSeen_s, 20.0f},
  {FaceSelectionPenaltyMultiplier::NotMakingEyeContact, 1.0f} };


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceSelectionComponent::~FaceSelectionComponent()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceSelectionComponent::ParseFaceSelectionFactorMap(const Json::Value& config, FaceSelectionFactorMap& output)
{
  // use a local map to avoid corrupting the output if we fail to parse later on
  FaceSelectionFactorMap map;

  for( const auto& entry : config ) {
    if( !entry[kPenaltyFactorKey].isString() || !entry[kPenaltyMultiplierKey].isNumeric() ) {
      return false;
    }

    map.emplace( FaceSelectionPenaltyMultiplierFromString( entry[kPenaltyFactorKey].asString() ),
                 entry[kPenaltyMultiplierKey].asFloat() );
  }

  // all parsed ok if we get here
  output.swap(map);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID FaceSelectionComponent::GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap) const
{
  const auto& faces = _faceWorld.GetFaceIDs();
  std::vector<SmartFaceID> smartFaces;

  for(auto faceID : faces) {
    smartFaces.emplace_back( _faceWorld.GetSmartFaceID( faceID ) );
  }
  
  return GetBestFaceToUse(criteriaMap, smartFaces);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID FaceSelectionComponent::GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap,
                                                     const std::vector<SmartFaceID>& possibleFaces) const
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
  using FullCalcScoreArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<
    FaceSelectionPenaltyMultiplier,
    calcScoreFunction,
    FaceSelectionPenaltyMultiplier::Count>;  
  
  static const FullCalcScoreArray kFullScoreCalcMap = {
    {FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians,
     std::bind(&FaceSelectionComponent::CalculateBodyAngleCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians,
     std::bind(&FaceSelectionComponent::CalculateHeadAngleCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::RelativeMicDirectionRadians,
     std::bind(&FaceSelectionComponent::CalculateMicDirectionCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::UnnamedFace,
     std::bind(&FaceSelectionComponent::CalculateUnnamedCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::TimeSinceSeen_s,
     std::bind(&FaceSelectionComponent::CalculateTimeSinceSeenCost, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::NotMakingEyeContact,
     std::bind(&FaceSelectionComponent::CalculateNotMakingEyeContact, this, std::placeholders::_1)},
    {FaceSelectionPenaltyMultiplier::Distance_m,
     std::bind(&FaceSelectionComponent::CalculateDistanceCost, this, std::placeholders::_1)}

  };
  DEV_ASSERT(IsSequentialArray(kFullScoreCalcMap), 
             "FaceSelectionComponent.GetBestFaceToUse.ScoreFuncsNotSequenttial");


  // add up score for all criteria

  // NOTE: it is possible that multiple entries in this vector are actually the same face (even if they
  // weren't when inserted). This is OK
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

      if( kFaceSelectionDebugging ) {
        PRINT_CH_DEBUG("Behaviors", "FaceSelectionComponent.Scoring",
                       "%s: %s = %f * factor %f",
                       currentID.GetDebugStr().c_str(),
                       FaceSelectionPenaltyMultiplierToString(criteriaPair.first),
                       criteriaScore,
                       criteriaFactor);
      }                       
    }

    if( kFaceSelectionDebugging ) {
      PRINT_CH_DEBUG("Behaviors", "FaceSelectionComponent.Scoring",
                     "%s: total = %f",
                     currentID.GetDebugStr().c_str(),
                     currentScore);
    }                       

    
    if(currentScore < bestScore){
      bestScore = currentScore;
      bestID = currentID;
    }
  } // end possible faces

  return bestID;
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateNotMakingEyeContact(const Vision::TrackedFace* currentFace) const
{
  // apply a penalty if not making eye contact
  return currentFace->IsMakingEyeContact() ? 0.0f : 1.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float FaceSelectionComponent::CalculateDistanceCost(const Vision::TrackedFace* currentFace) const
{

  float distanceBetween_mm = 0.0f;
  
  if( ! ComputeDistanceBetween(currentFace->GetHeadPose(), _robot.GetPose(), distanceBetween_mm) ) {
    // no transform, probably a different origin
    return 0.0f;
  }

  return MM_TO_M(distanceBetween_mm);
}

} // namespace Cozmo
} // namespace Anki
