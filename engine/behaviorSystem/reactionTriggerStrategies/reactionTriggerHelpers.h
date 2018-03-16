/**
 * File: reactionTriggerHelpers.h
 *
 * Author: Kevin M. Karol
 * Created: 2/28/17
 *
 * Description: Helpers for translating between ReactionTrigger Structs and 
 * enumerated arrays.  Also contains convenience array constants
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategies_ReactionTriggerHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategies_ReactionTriggerHelpers_H__

#define ALL_TRIGGERS_CONSIDERED_TO_FULL_ARRAY(triggers) { \
  {ReactionTrigger::CliffDetected,                triggers.cliffDetected}, \
  {ReactionTrigger::CubeMoved,                    triggers.cubeMoved}, \
  {ReactionTrigger::FacePositionUpdated,          triggers.facePositionUpdated}, \
  {ReactionTrigger::FistBump,                     triggers.fistBump}, \
  {ReactionTrigger::Frustration,                  triggers.frustration}, \
  {ReactionTrigger::Hiccup,                       triggers.hiccup}, \
  {ReactionTrigger::MotorCalibration,             triggers.motorCalibration}, \
  {ReactionTrigger::NoPreDockPoses,               triggers.noPreDockPoses}, \
  {ReactionTrigger::ObjectPositionUpdated,        triggers.objectPositionUpdated}, \
  {ReactionTrigger::PlacedOnCharger,              triggers.placedOnCharger}, \
  {ReactionTrigger::PetInitialDetection,          triggers.petInitialDetection}, \
  {ReactionTrigger::RobotFalling,                 triggers.robotFalling},  \
  {ReactionTrigger::RobotPickedUp,                triggers.robotPickedUp},  \
  {ReactionTrigger::RobotPlacedOnSlope,           triggers.robotPlacedOnSlope},  \
  {ReactionTrigger::ReturnedToTreads,             triggers.returnedToTreads},  \
  {ReactionTrigger::RobotOnBack,                  triggers.robotOnBack},  \
  {ReactionTrigger::RobotOnFace,                  triggers.robotOnFace},  \
  {ReactionTrigger::RobotOnSide,                  triggers.robotOnSide},  \
  {ReactionTrigger::RobotShaken,                  triggers.robotShaken},  \
  {ReactionTrigger::Sparked,                      triggers.sparked},  \
  {ReactionTrigger::UnexpectedMovement,           triggers.unexpectedMovement},  \
}


#include "clad/types/behaviorSystem/reactionTriggers.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"


namespace Anki {
namespace Cozmo {
namespace ReactionTriggerHelpers {
  
using FullReactionArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<ReactionTrigger, bool, ReactionTrigger::Count>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline bool IsTriggerAffected(ReactionTrigger reactionTrigger,
                                     const FullReactionArray& reactions)
{
  return reactions[Util::EnumToUnderlying(reactionTrigger)].Value();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AllTriggersConsidered ConvertReactionArrayToAllTriggersConsidered(const FullReactionArray& reactions);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Convenience function for parts of engine that want to use the AllTriggersConsidered struct so values can be assignable
// since the FullEnumArray is non-assignable


const ReactionTriggerHelpers::FullReactionArray& GetAffectAllArray();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const AllTriggersConsidered& GetAffectAllReactions();
  
} // namespace ReactionTriggerHelpers
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategies_ReactionTriggerHelpers_H__
