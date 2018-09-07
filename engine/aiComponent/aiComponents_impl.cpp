/**
* File: aiComponents_impl.cpp
*
* Author: Kevin M. Karol
* Created: 2/12/18
*
* Description: Template specializations mapping classes to enums
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/componentTypeEnumMap.h"

namespace Anki {
namespace Vector {

// Forward declarations
class BehaviorComponent;
class ContinuityComponent;
class FaceSelectionComponent;
class ObjectInteractionInfoCache;
class PuzzleComponent;
class TimerUtility;
class AIWhiteboard;
class SalientPointsComponent;

} // namespace Vector

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorComponent,                 AIComponentID, BehaviorComponent)
LINK_COMPONENT_TYPE_TO_ENUM(ContinuityComponent,               AIComponentID, ContinuityComponent)
LINK_COMPONENT_TYPE_TO_ENUM(FaceSelectionComponent,            AIComponentID, FaceSelection)
LINK_COMPONENT_TYPE_TO_ENUM(ObjectInteractionInfoCache,        AIComponentID, ObjectInteractionInfoCache)
LINK_COMPONENT_TYPE_TO_ENUM(PuzzleComponent,                   AIComponentID, Puzzle)
LINK_COMPONENT_TYPE_TO_ENUM(TimerUtility,                      AIComponentID, TimerUtility)
LINK_COMPONENT_TYPE_TO_ENUM(AIWhiteboard,                      AIComponentID, Whiteboard)
LINK_COMPONENT_TYPE_TO_ENUM(SalientPointsComponent,            AIComponentID, SalientPointsDetectorComponent)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Vector::AIComponentID>(){ return "AIComponents"; }

template<>
std::string GetComponentStringForID<Vector::AIComponentID>(Vector::AIComponentID enumID)
{
  #define CASE(id) case Vector::AIComponentID::id: { return #id; }
  switch (enumID) {
    CASE(BehaviorComponent)
    CASE(ContinuityComponent)
    CASE(FaceSelection)
    CASE(ObjectInteractionInfoCache)
    CASE(Puzzle)
    CASE(SalientPointsDetectorComponent)
    CASE(TimerUtility)
    CASE(Whiteboard)
    CASE(Count)
  }
  #undef CASE
}


} // namespace Anki
