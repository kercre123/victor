/**
* File: conditionFaceKnown.cpp
*
* Author:  ross
* Created: May 15 2018
*
* Description: Condition that is true if any (optionally named) face is known within a recent timeframe
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionFaceKnown.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Vector {
  
namespace{
  const char* const kMaxFaceDistKey    = "maxFaceDist_mm";
  const char* const kMaxFaceAgeKey     = "maxFaceAge_s";
  const char* const kMustBeNamed       = "mustBeNamed";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionFaceKnown::ConditionFaceKnown(const Json::Value& config)
: IBEICondition(config)
{
  _maxFaceDist_mm = config.get( kMaxFaceDistKey, -1.0f ).asFloat();
  _maxFaceAge_s = config.get( kMaxFaceAgeKey, -1 ).asInt();
  _mustBeNamed = config.get( kMustBeNamed, false ).asBool();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionFaceKnown::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  const auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
  
  std::set<Vision::FaceID_t> faces;
  if( _maxFaceAge_s >= 0 ) {
    const RobotTimeStamp_t latestImageTimestamp = behaviorExternalInterface.GetRobotInfo().GetLastImageTimeStamp();
    const auto maxFaceAge_ms = 1000*_maxFaceAge_s;
    RobotTimeStamp_t minAge = (latestImageTimestamp > maxFaceAge_ms) ? (latestImageTimestamp - maxFaceAge_ms) : 0;
    faces = faceWorld.GetFaceIDs( minAge );
  } else {
    faces = faceWorld.GetFaceIDs();
  }
  
  bool ret = false;
  for( const auto& faceID : faces ) {
    const auto* face = faceWorld.GetFace(faceID);
    if( (face != nullptr)
        && (!_mustBeNamed || face->HasName()) )
    {
      if( _maxFaceDist_mm < 0.0f ) {
        ret = true;
        break;
      } else {
        const Pose3d facePose = face->GetHeadPose();
        float distanceToFace = 0.0f;
        if( ComputeDistanceSQBetween( behaviorExternalInterface.GetRobotInfo().GetPose(),
                                      facePose,
                                      distanceToFace ) &&
            (distanceToFace < _maxFaceDist_mm*_maxFaceDist_mm) )
        {
          ret = true;
          break;
        }
      }
    }
  }
  return ret;
}

} // namespace Vector
} // namespace Anki
