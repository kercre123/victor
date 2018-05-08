/**
 * File: activeFeature.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-05-07
 *
 * Description: Component to track the current active feature
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

ActiveFeatureComponent::ActiveFeatureComponent()
  : IDependencyManagedComponent( this, BCComponentID::ActiveFeature )
{
}

void ActiveFeatureComponent::UpdateDependent(const BCCompMap& dependentComponents)
{
  const auto& behaviorIterator = dependentComponents.GetValue<ActiveBehaviorIterator>();

  // Only bother to do an update if the behavior stack changed
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  
  if( behaviorIterator.GetLastTickBehaviorStackChanged() == currTick ) {
    
    // get the feature at the end of the stack
    ActiveFeature lastFeature = ActiveFeature::NoFeature;
    auto callback = [&lastFeature](const ICozmoBehavior& behavior) {
      ActiveFeature feature = ActiveFeature::NoFeature;
      if( behavior.GetAssociatedActiveFeature(feature) &&
          feature != ActiveFeature::NoFeature ) {
        lastFeature = feature;
      }
    };

    behaviorIterator.IterateActiveCozmoBehaviorsForward(callback);

    SetActiveFeature(lastFeature);
  }
}

ActiveFeature ActiveFeatureComponent::GetActiveFeature() const
{
  return _activeFeature;
}

void ActiveFeatureComponent::SetActiveFeature(ActiveFeature newFeature)
{
  if( _activeFeature != newFeature ) {
    
    if( _activeFeature != ActiveFeature::NoFeature ) {
      PRINT_CH_INFO("BehaviorSystem", "active_feature.end",
                    "%s",
                    ActiveFeatureToString(_activeFeature));
    }

    if( newFeature != ActiveFeature::NoFeature ) {
      PRINT_CH_INFO("BehaviorSystem", "active_feature.start",
                    "%s",
                    ActiveFeatureToString(newFeature));      
    }

    _activeFeature = newFeature;
  }
}

}
}
