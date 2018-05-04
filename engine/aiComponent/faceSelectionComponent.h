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

#include "json/json-forwards.h"

#include "clad/types/faceSelectionTypes.h"

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
  using FaceSelectionFactorMap = std::unordered_map<FaceSelectionPenaltyMultiplier, float>;

  static const FaceSelectionFactorMap kDefaultSelectionCriteria;
  
  explicit FaceSelectionComponent(const Robot& robot,
                                  const FaceWorld& faceWorld,
                                  const MicDirectionHistory& micDirectionHistory)
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::FaceSelection)
  , _robot(robot)
  , _faceWorld(faceWorld)
  , _micDirectionHistory(micDirectionHistory){}
  ~FaceSelectionComponent();

  // try to read a list of factor penalties from the given json config. Return true and modify the passed in
  // criteria on success, false otherwise
  static bool ParseFaceSelectionFactorMap(const Json::Value& config, FaceSelectionFactorMap& output);

  SmartFaceID GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap,
                               const std::vector<SmartFaceID>& possibleFaces) const;

  // defaults to using any known face present in face world
  SmartFaceID GetBestFaceToUse(const FaceSelectionFactorMap& criteriaMap) const;

private:
  const Robot& _robot;
  const FaceWorld& _faceWorld;
  const MicDirectionHistory& _micDirectionHistory;

  float CalculateBodyAngleCost(const Vision::TrackedFace* currentFace) const;
  float CalculateHeadAngleCost(const Vision::TrackedFace* currentFace) const;
  float CalculateMicDirectionCost(const Vision::TrackedFace* currentFace) const;
  float CalculateUnnamedCost(const Vision::TrackedFace* currentFace) const;
  float CalculateTimeSinceSeenCost(const Vision::TrackedFace* currentFace) const;
  float CalculateNotMakingEyeContact(const Vision::TrackedFace* currentFace) const;
  float CalculateDistanceCost(const Vision::TrackedFace* currentFace) const;

};

}
}

#endif
