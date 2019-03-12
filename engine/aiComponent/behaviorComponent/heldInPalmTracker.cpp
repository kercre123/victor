/**
 * File: heldInPalmTracker.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-02-05
 *
 * Description: Behavior component to track various metrics, such as the
 * "trust" level that the robot has when being held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#define LOG_CHANNEL "Behaviors"

#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include "webServerProcess/src/webService.h"
#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

#define CONSOLE_GROUP "HeldInPalm.Tracker"

CONSOLE_VAR(float, kHeldInPalmTracker_updatePeriod_s, CONSOLE_GROUP, 60.0);

namespace {
  static const char* kWebVizModuleNameHeldInPalm = "heldinpalm";
}

HeldInPalmTracker::HeldInPalmTracker()
  : IDependencyManagedComponent(this, BCComponentID::HeldInPalmTracker )
{
}

void HeldInPalmTracker::SetIsHeldInPalm(const bool isHeldInPalm) {
  if (_isHeldInPalm != isHeldInPalm) {
    LOG_INFO("HeldInPalmTracker.SetIsHeldInPalm", "%s", isHeldInPalm ? "true" : "false");
  }
  _isHeldInPalm = isHeldInPalm;
}

void HeldInPalmTracker::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  // Set up event handle for WebViz subscription, and cache a pointer to the FeatureGate interface if it exists
  const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      auto onWebVizSubscribed = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // Send data upon getting subscribed to WebViz
        Json::Value data;
        PopulateWebVizJson(data);
        sendToClient(data);
      };
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameHeldInPalm ).ScopedSubscribe(
                                    onWebVizSubscribed ));
    }
  }
}

void HeldInPalmTracker::UpdateDependent(const BCCompMap& dependentComps)
{
  const float currBSTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currBSTime_s > _nextUpdateTime_s ) {
    _nextUpdateTime_s = currBSTime_s + kHeldInPalmTracker_updatePeriod_s;
    
    const auto* context = dependentComps.GetComponent<BEIRobotInfo>().GetContext();
    if( context ) {
      if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameHeldInPalm, context->GetWebService()) ) {
        PopulateWebVizJson(webSender->Data());
      }
    }
  }
}

}
}
