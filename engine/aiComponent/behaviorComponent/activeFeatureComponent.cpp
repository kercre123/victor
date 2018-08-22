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

#include "clad/types/simpleMoodTypes.h"
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
#include "engine/moodSystem/moodManager.h"
#include "proto/external_interface/messages.pb.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include "webServerProcess/src/webVizSender.h"


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

    _isTutorial = false;
    
    // get the feature at the end of the stack
    ActiveFeature newFeature = ActiveFeature::NoFeature;
    auto callback = [&newFeature, this](const ICozmoBehavior& behavior) {
      ActiveFeature feature = ActiveFeature::NoFeature;
      if( behavior.GetAssociatedActiveFeature(feature) &&
          feature != ActiveFeature::NoFeature ) {
        newFeature = feature;
        if( newFeature == ActiveFeature::Onboarding ) {
          _isTutorial = true;
        }
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

      const SimpleMoodType simpleMood = dependentComps.GetComponent<MoodManager>().GetSimpleMood();

      OnFeatureChanged( newFeature, _activeFeature, intentSource, simpleMood );

      if( _activeFeature == ActiveFeature::Petting &&
          newFeature != ActiveFeature::Petting ) {
        const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        const float timeActive = currTime_s - _lastFeatureActivatedTime_s;
        dependentComps.GetComponent<RobotStatsTracker>().IncrementPettingDuration( timeActive );
      }

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
  if( _activeFeature != ActiveFeature::NoFeature ) {
    if( _context != nullptr ) {
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender("behaviors",
                                                                        _context->GetWebService()) ) {
        webSender->Data()["activeFeature"] = std::string(ActiveFeatureToString(_activeFeature)) +
                                             " (" + intentSource + ")";
      }
    }
  }
}

void ActiveFeatureComponent::OnFeatureChanged(const ActiveFeature& newFeature,
                                              const ActiveFeature& oldFeature,
                                              const std::string& source,
                                              const SimpleMoodType& simpleMood)
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
    // override the active feature's ActiveFeatureType if the feature is part of onboarding
    const ActiveFeatureType& featureType = _isTutorial
                                           ? ActiveFeatureType::Onboarding
                                           : GetActiveFeatureType(newFeature, ActiveFeatureType::System);

    // before sending the official feature start event, send a "pre_start" event which will still be tracked
    // under the previous feature run ID
    DASMSG(behavior_feature_pre_start, "behavior.feature.pre_start", "A new feature will activate (next message)");
    DASMSG_SET(s1, ActiveFeatureToString(newFeature), "The feature that will start");
    DASMSG_SET(s2, SimpleMoodTypeToString(simpleMood), "The current simple mood");
    DASMSG_SET(s3, uuid, "The UUID the new feature will use for its feature run ID");
    DASMSG_SET(s4, ActiveFeatureTypeToString(featureType), "The type of that new feature");
    DASMSG_SEND();

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
