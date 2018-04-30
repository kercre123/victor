/**
 * File: faceSelectionComponent.h
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

#ifndef __Cozmo_Basestation_AIComponent_FaceSelectionComponent_H__
#define __Cozmo_Basestation_AIComponent_FaceSelectionComponent_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <set>
#include <unordered_map>


namespace Anki {

// Forward Decleration
namespace Vision{
class TrackedFace;
}

namespace Cozmo {

class Robot;
class FaceWorld;
class MicDirectionHistory;
class SmartFaceID;


  
class FaceSelectionComponent : public IDependencyManagedComponent<AIComponentID>,  
                               private Util::noncopyable
{
public:
  // Enum that specifies the way that score penalties are applied when selecting face
  // enums that are unspecified apply a penalty of 0
  enum class FaceSelectionPenaltyMultiplier{
    RelativeBodyAngleRadians,
    RelativeHeadAngleRadians,
    RelativeMicDirectionRadians,
    UnnamedFace,
    TimeSinceSeen_s,

    Count
  };
  using FaceSelectionFactorMap = std::unordered_map<FaceSelectionPenaltyMultiplier, float>;
  
  explicit FaceSelectionComponent(const Robot& robot, const FaceWorld& faceWorld, const MicDirectionHistory& micDirectionHistory)
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::FaceSelection)
  , _robot(robot)
  , _faceWorld(faceWorld)
  , _micDirectionHistory(micDirectionHistory){}
  ~FaceSelectionComponent();

  SmartFaceID GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap, const std::vector<SmartFaceID>& possibleFaces) const;


  // Convert string to enum
  static FaceSelectionPenaltyMultiplier FaceSelectionPenaltyFromString(const std::string& str);

private:
  const Robot& _robot;
  const FaceWorld& _faceWorld;
  const MicDirectionHistory& _micDirectionHistory;

  float CalculateBodyAngleCost(const Vision::TrackedFace* currentFace) const;
  float CalculateHeadAngleCost(const Vision::TrackedFace* currentFace) const;
  float CalculateMicDirectionCost(const Vision::TrackedFace* currentFace) const;
  float CalculateUnnamedCost(const Vision::TrackedFace* currentFace) const;
  float CalculateTimeSinceSeenCost(const Vision::TrackedFace* currentFace) const;

};

}
}

#endif
