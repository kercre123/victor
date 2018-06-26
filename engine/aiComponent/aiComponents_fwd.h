/**
* File: aiComponents_fwd.h
*
* Author: Kevin M. Karol
* Created: 1/5/18
*
* Description: Enumeration of components within aiComponent.h's entity
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_AI_Components_fwd_H__
#define __Cozmo_Basestation_BehaviorSystem_AI_Components_fwd_H__

#include <set>

namespace Anki {

// forward declarations
template<typename EnumType>
class DependencyManagedEntity;

template<typename EnumType>
class IDependencyManagedComponent;


namespace Cozmo {

// When adding to this enum be sure to also declare a template specialization
// in the _impl.cpp file mapping the enum to the class type it is associated with
enum class AIComponentID{
  // component which manages all aspects of the AI system that relate to behaviors
  BehaviorComponent,
  // component which sits between the behavior system and action list/animation streamer
  // to ensure smooth transitions between actions
  ContinuityComponent,
  // provide a simple interface for selecting the best face to interact with
  FaceSelection,
  // component for tracking freeplay DAS data
  FreeplayDataTracker,
  // module to analyze information for the AI in processes common to more than one behavior, for example
  // border calculation
  InformationAnalyzer,
  // Component which tracks and caches the best objects to use for certain interactions
  ObjectInteractionInfoCache,
  // Component that maintains the puzzles victor can solve
  Puzzle,
  // component for behaviors to access information about salient points being detected
  SalientPointsDetectorComponent,
  // component that maintains persistant information about the timer utility
  TimerUtility,
  // whiteboard for behaviors to share information, or to store information only useful to behaviors
  Whiteboard,

  Count,


};

using AIComp =  IDependencyManagedComponent<AIComponentID>;
using AICompMap = DependencyManagedEntity<AIComponentID>;
using AICompIDSet = std::set<AIComponentID>;


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
