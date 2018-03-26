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
namespace Cozmo {

// Forward declarations
class BehaviorComponent;
class ContinuityComponent;
class FaceSelectionComponent;
class FreeplayDataTracker;
class AIInformationAnalyzer;
class ObjectInteractionInfoCache;
class PuzzleComponent;
class CompositeImageCache;
class TimerUtility;
class AIWhiteboard;

} // namespace Cozmo

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorComponent,          AIComponentID, BehaviorComponent)
LINK_COMPONENT_TYPE_TO_ENUM(ContinuityComponent,        AIComponentID, ContinuityComponent)
LINK_COMPONENT_TYPE_TO_ENUM(FaceSelectionComponent,     AIComponentID, FaceSelection)
LINK_COMPONENT_TYPE_TO_ENUM(FreeplayDataTracker,        AIComponentID, FreeplayDataTracker)
LINK_COMPONENT_TYPE_TO_ENUM(AIInformationAnalyzer,      AIComponentID, InformationAnalyzer)
LINK_COMPONENT_TYPE_TO_ENUM(ObjectInteractionInfoCache, AIComponentID, ObjectInteractionInfoCache)
LINK_COMPONENT_TYPE_TO_ENUM(PuzzleComponent,            AIComponentID, Puzzle)
LINK_COMPONENT_TYPE_TO_ENUM(CompositeImageCache,        AIComponentID, CompositeImageCache)
LINK_COMPONENT_TYPE_TO_ENUM(TimerUtility,               AIComponentID, TimerUtility)
LINK_COMPONENT_TYPE_TO_ENUM(AIWhiteboard,               AIComponentID, Whiteboard)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Cozmo::AIComponentID>(){ return "AIComponents"; }

template<>
std::string GetComponentStringForID<Cozmo::AIComponentID>(Cozmo::AIComponentID enumID)
{
  switch(enumID){
    case Cozmo::AIComponentID::BehaviorComponent:          { return "BehaviorComponent";}
    case Cozmo::AIComponentID::ContinuityComponent:        { return "ContinuityComponent";}
    case Cozmo::AIComponentID::FaceSelection:              { return "FaceSelection";}
    case Cozmo::AIComponentID::FreeplayDataTracker:        { return "FreeplayDataTracker";}
    case Cozmo::AIComponentID::InformationAnalyzer:        { return "InformationAnalyzer";}
    case Cozmo::AIComponentID::ObjectInteractionInfoCache: { return "ObjectInteractionInfoCache";}
    case Cozmo::AIComponentID::Puzzle:                     { return "Puzzle";}
    case Cozmo::AIComponentID::CompositeImageCache:        { return "CompositeImageCache";}
    case Cozmo::AIComponentID::TimerUtility:               { return "TimerUtility";}
    case Cozmo::AIComponentID::Whiteboard:                 { return "Whiteboard";}
    case Cozmo::AIComponentID::Count:                      { return "Count";}
  }
}


} // namespace Anki
