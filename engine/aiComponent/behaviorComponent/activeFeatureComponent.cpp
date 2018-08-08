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
#include "engine/aiComponent/behaviorComponent/statusLogHandler.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/cozmoContext.h"
#include "proto/external_interface/messages.pb.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/string/stringUtils.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

ActiveFeatureComponent::ActiveFeatureComponent()
  : IDependencyManagedComponent( this, BCComponentID::ActiveFeature )
{
}

void ActiveFeatureComponent::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  _context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
  _lastUsedIntentActivationID = 0;

  _statusLogHandler.reset( new StatusLogHandler( _context ) );
}

void ActiveFeatureComponent::UpdateDependent(const BCCompMap& dependentComps)
{
  const auto& behaviorIterator = dependentComps.GetComponent<ActiveBehaviorIterator>();

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
      return true; // iterate the entire stack
    };

    behaviorIterator.IterateActiveCozmoBehaviorsForward(callback);

    if( _activeFeature != newFeature ) {

      const auto& uic = dependentComps.GetComponent<UserIntentComponent>();

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

      OnFeatureChanged( newFeature, _activeFeature, intentSource );

      if( newFeature != ActiveFeature::NoFeature ) {
        dependentComps.GetComponent<RobotStatsTracker>().IncrementActiveFeature(newFeature, intentSource);
      }

      _activeFeature = newFeature;
    }
  }
}

ActiveFeature ActiveFeatureComponent::GetActiveFeature() const
{
  return _activeFeature;
}

void ActiveFeatureComponent::SendActiveFeatureToWebViz(const std::string& intentSource) const
{
  static const std::string kWebVizModuleName = "behaviors";
  if( _activeFeature != ActiveFeature::NoFeature ) {
    if( _context != nullptr ) {
      const auto* webService = _context->GetWebService();
      if( webService != nullptr && webService->IsWebVizClientSubscribed(kWebVizModuleName) ) {
        Json::Value data;
        data["activeFeature"] = std::string(ActiveFeatureToString(_activeFeature)) + " (" + intentSource + ")";
        webService->SendToWebViz(kWebVizModuleName, data);
      }
    }
  }
}

void ActiveFeatureComponent::OnFeatureChanged(const ActiveFeature& newFeature, const ActiveFeature& oldFeature, const std::string& source)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // send das message
  if( oldFeature != ActiveFeature::NoFeature ) {
    const float timeActive = currTime_s - _lastFeatureActivatedTime_s;

    DASMSG(behavior_feature_end, "behavior.feature.end",
           "This feature is no longer active, but there may not be a new feature yet");
    DASMSG_SET(s1, ActiveFeatureToString(_activeFeature), "The feature");
    DASMSG_SET(i1, std::round(timeActive), "Time the feature was active in seconds");
    DASMSG_SEND();
  }

  if( newFeature != ActiveFeature::NoFeature ) {
    const std::string& uuid = Util::GetUUIDString();
    const ActiveFeatureType& featureType = GetActiveFeatureType(newFeature, ActiveFeatureType::System);

    // NOTE: this event is inspected by the DAS Manager to determine the feature id and type columns, so be
    // especially careful if changing it
    DASMSG(behavior_feature_start, DASMSG_FEATURE_START, "A new feature is active");
    DASMSG_SET(s1, ActiveFeatureToString(newFeature), "The feature");
    DASMSG_SET(s2, source, "The source of the intent (possible values are AI, App, Voice, or Unknown)");
    DASMSG_SET(s3, uuid, "A unique ID associated with this feature run");
    DASMSG_SET(s4, ActiveFeatureTypeToString(featureType), "The feature type (category)");
    DASMSG_SEND();

    _lastFeatureActivatedTime_s = currTime_s;
  }

  _statusLogHandler->SetFeature( ActiveFeatureToString(newFeature), source );

  SendActiveFeatureToWebViz(source);
}

}
}
