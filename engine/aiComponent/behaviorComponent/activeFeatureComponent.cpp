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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/cozmoContext.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Cozmo {

ActiveFeatureComponent::ActiveFeatureComponent()
  : IDependencyManagedComponent( this, BCComponentID::ActiveFeature )
{
}

void ActiveFeatureComponent::InitDependent( Robot* robot, const BCCompMap& dependentComponents )
{
  _context = dependentComponents.GetValue<BEIRobotInfo>().GetContext();
  _lastUsedIntentActivationID = 0;
}

void ActiveFeatureComponent::UpdateDependent(const BCCompMap& dependentComponents)
{
  const auto& behaviorIterator = dependentComponents.GetValue<ActiveBehaviorIterator>();

  // Only bother to do an update if the behavior stack changed
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  
  if( behaviorIterator.GetLastTickBehaviorStackChanged() == currTick ) {
    
    // get the feature at the end of the stack
    ActiveFeature newFeature = ActiveFeature::NoFeature;
    auto callback = [&newFeature](const ICozmoBehavior& behavior) {
      ActiveFeature feature = ActiveFeature::NoFeature;
      if( behavior.GetAssociatedActiveFeature(feature) &&
          feature != ActiveFeature::NoFeature ) {
        newFeature = feature;
      }
    };

    behaviorIterator.IterateActiveCozmoBehaviorsForward(callback);

    if( _activeFeature != newFeature ) {

      const auto& uic = dependentComponents.GetValue<UserIntentComponent>();

      UserIntentPtr activeIntent = uic.GetActiveUserIntent();

      // TODO:(bn) delete this soon. This shouldn't happen but is possible theoretically. If this is a real
      // use case, I need to know about it
      if( activeIntent == nullptr &&
          uic.IsAnyUserIntentPending() &&
          newFeature != ActiveFeature::NoFeature ) {
        PRINT_NAMED_ERROR("ActiveFeatureComponent.PossibleBug",
                          "TELL BRAD: Feature '%s' is activating (old feature is %s). No intent active, but one is pending.",
                          ActiveFeatureToString(newFeature),
                          ActiveFeatureToString(_activeFeature));
      }

      // the default is that the robot activated the intent on it's own
      std::string intentSource = "AI";
      
      if( activeIntent != nullptr ) {
        // only consider a single source to be activated by an intent
        if( newFeature != ActiveFeature::NoFeature &&
            activeIntent->activationID != _lastUsedIntentActivationID ) {
          // set intent source from this feature, and mark it's ID so we don't use it again
          intentSource = UserIntentSourceToString(activeIntent->source);
          _lastUsedIntentActivationID = activeIntent->activationID;
        }
      }
      
      if( _activeFeature != ActiveFeature::NoFeature ) {
        DASMSG(behavior_featureEnd, "behavior.feature.end", "This feature is no longer active");
        DASMSG_SET(s1, ActiveFeatureToString(_activeFeature), "The feature");
        DASMSG_SEND();
      }

      if( newFeature != ActiveFeature::NoFeature ) {
        DASMSG(behavior_featureEnd, "behavior.feature.start", "A new feature is active");
        DASMSG_SET(s1, ActiveFeatureToString(newFeature), "The feature");
        DASMSG_SET(s2, intentSource, "The source of the intent (possible values are AI, App, Voice, or Unknown)");
        DASMSG_SEND();
      }

      _activeFeature = newFeature;
      SendActiveFeatureToWebViz(intentSource);
    }
  }
}

ActiveFeature ActiveFeatureComponent::GetActiveFeature() const
{
  return _activeFeature;
}

void ActiveFeatureComponent::SendActiveFeatureToWebViz(const std::string& intentSource) const
{
  if( _activeFeature != ActiveFeature::NoFeature ) {
    if( _context != nullptr ) {
      const auto* webService = _context->GetWebService();
      if( webService != nullptr ){
        Json::Value data;
        data["activeFeature"] = std::string(ActiveFeatureToString(_activeFeature)) + " (" + intentSource + ")";
        webService->SendToWebViz("behaviors", data);
      }
    }
  }
}

}
}
