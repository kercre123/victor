/**
 * File: BehaviorCoordinateInHabitat.cpp
 *
 * Author: Arjun Menon
 * Created: 2018-07-30
 *
 * Description: A coordinating behavior that ensures certain behaviors
 * and voice commands are suppressed or allowed when the robot knows
 * it is inside the Habitat tray
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateInHabitat.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/habitatDetectorComponent.h"
#include <iterator>

namespace Anki {
namespace Cozmo {
  
namespace
{
  // from PassThroughDispatcher base class
  const char* kBehaviorIDConfigKey = "delegateID";
  
  const char* kSuppressedBehaviorsKey = "suppressedBehaviors";
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateInHabitat::InstanceConfig::InstanceConfig()
{
  suppressedBehaviorNames.clear();
  behaviorsToSuppressInHabitat.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateInHabitat::BehaviorCoordinateInHabitat(const Json::Value& config)
: BehaviorDispatcherPassThrough(config)
{
  JsonTools::GetVectorOptional(config, kSuppressedBehaviorsKey, _iConfig.suppressedBehaviorNames);
  if(_iConfig.suppressedBehaviorNames.size()==0) {
    PRINT_NAMED_WARNING("BehaviorCoordinateInHabitat.EmptySuppressedBehaviorList","");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateInHabitat::~BehaviorCoordinateInHabitat()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateInHabitat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kBehaviorIDConfigKey);
  expectedKeys.insert(kSuppressedBehaviorsKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateInHabitat::InitPassThrough()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  for( const auto& name : _iConfig.suppressedBehaviorNames) {
    BehaviorID id = BehaviorTypesWrapper::BehaviorIDFromString(name);
    _iConfig.behaviorsToSuppressInHabitat.push_back( BC.FindBehaviorByID(id) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateInHabitat::OnPassThroughActivated()
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateInHabitat::PassThroughUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
  if(habitatDetector.GetHabitatBeliefState() == HabitatBeliefState::InHabitat) {
    for( const auto& beh : _iConfig.behaviorsToSuppressInHabitat ) {
      beh->SetDontActivateThisTick(GetDebugLabel());
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateInHabitat::OnPassThroughDeactivated()
{

}

}
}
