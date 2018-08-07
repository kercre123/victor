/**
 * File: stackVizMonitor.h
 *
 * Author: ross
 * Created: 2018-06 26
 *
 * Description: Sends stack info to webviz and webots
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/stackMonitors/stackVizMonitor.h"

#include "clad/vizInterface/messageViz.h"
#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/viz/vizManager.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackVizMonitor::NotifyOfChange( BehaviorExternalInterface& bei,
                                      const std::vector<IBehavior*>& stack,
                                      const BehaviorStack* stackComponent )
{
  SendToWebViz( bei, stack, stackComponent );
  SendToWebots( bei, stack, stackComponent );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackVizMonitor::SendToWebViz( BehaviorExternalInterface& bei,
                                    const std::vector<IBehavior*>& stack,
                                    const BehaviorStack* stackComponent ) const
{
  const auto* context = bei.GetRobotInfo().GetContext();
  if( context != nullptr ) {
    const auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      const bool behaviorsSub = webService->IsWebVizClientSubscribed( "behaviors" );
      const bool behaviorCondsSub = webService->IsWebVizClientSubscribed( "behaviorconds" );
      Json::Value data;
      if( behaviorsSub || behaviorCondsSub ) {
        data = stackComponent->BuildDebugBehaviorTree( bei );
      }
      if( behaviorsSub ) {
        webService->SendToWebViz( "behaviors", data );
      }
      if( behaviorCondsSub ) {
        webService->SendToWebViz( "behaviorconds", data );
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackVizMonitor::SendToWebots( BehaviorExternalInterface& bei,
                                    const std::vector<IBehavior*>& stack,
                                    const BehaviorStack* stackComponent ) const
{
  VizInterface::BehaviorStackDebug data;

  for( const auto& behavior : stack ) {
    data.debugStrings.push_back( behavior->GetDebugLabel() );
  }

  auto context = bei.GetRobotInfo().GetContext();
  if( context != nullptr ){
    auto vizManager = context->GetVizManager();
    if( vizManager != nullptr ) {
      vizManager->SendBehaviorStackDebug( std::move(data) );
    }
  }
}

}
}

