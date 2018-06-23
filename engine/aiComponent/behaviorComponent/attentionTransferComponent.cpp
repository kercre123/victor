/**
 * File: attentionTransferComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-06-15
 *
 * Description: Component to manage state related to attention transfer between the robot and app
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/attentionTransferComponent.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "util/entityComponent/dependencyManagedEntity.h"

namespace Anki {
namespace Cozmo {

AttentionTransferComponent::AttentionTransferComponent()
  : IDependencyManagedComponent( this, BCComponentID::AttentionTransferComponent )
{
}


void AttentionTransferComponent::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  _gi = dependentComps.GetComponent<BEIRobotInfo>().GetGatewayInterface();

  _eventHandles.push_back(_gi->Subscribe(AppToEngineTag::kLatestAttentionTransferRequest,
                                         [this](const AppToEngineEvent& event) {
                                           HandleAppRequest(event);
                                         }));
}

void AttentionTransferComponent::PossibleAttentionTransferNeeded(AttentionTransferReason reason)
{
  auto& tracker = GetTracker(reason);
  tracker.AddOccurrence();
}

RecentOccurrenceTracker::Handle AttentionTransferComponent::GetRecentOccurrenceHandle(AttentionTransferReason reason,
                                                                                      int numberOfTimes,
                                                                                      float amountOfSeconds)
{
  auto& tracker = GetTracker(reason);
  return tracker.GetHandle(numberOfTimes, amountOfSeconds);
}

void AttentionTransferComponent::OnAttentionTransferred(AttentionTransferReason reason)
{

  PRINT_CH_INFO("Behaviors", "AttentionTransferComponent.OnAttentionTransferred",
                "Send attention transfer message for '%s'",
                AttentionTransferReasonToString(reason));


  ResetAttentionTransfer(reason);

  if( _gi != nullptr ) {
    auto* attentionTransfer = new external_interface::AttentionTransfer;
    attentionTransfer->set_reason( CladProtoTypeTranslator::ToProtoEnum(reason) );
    const float now = 0.0f;
    attentionTransfer->set_seconds_ago(now);

    _gi->Broadcast( ExternalMessageRouter::Wrap( attentionTransfer ) );
  }

  _lastAttentionTransferReason = reason;
  _lastAttentionTransferTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void AttentionTransferComponent::ResetAttentionTransfer(AttentionTransferReason reason)
{
  auto it = _transferTrackers.find(reason);
  if( it != _transferTrackers.end() ) {
    // Reset the tracker since the attention was transferred
    it->second.Reset();
  }
}

void AttentionTransferComponent::HandleAppRequest(const AppToEngineEvent& event)
{
  DEV_ASSERT( event.GetData().oneof_message_type_case() ==
              external_interface::GatewayWrapper::OneofMessageTypeCase::kLatestAttentionTransferRequest,
              "AttentionTransferComponent.HandleAppRequest.InvalidEvent" );

  if( _gi != nullptr ) {
    auto* response = new external_interface::LatestAttentionTransfer;
    if( _lastAttentionTransferReason != AttentionTransferReason::Invalid ) {
      auto* attentionTransfer = new external_interface::AttentionTransfer;
      attentionTransfer->set_reason( CladProtoTypeTranslator::ToProtoEnum(_lastAttentionTransferReason) );

      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const float dt = currTime_s - _lastAttentionTransferTime_s;
      attentionTransfer->set_seconds_ago(dt);

      response->set_allocated_attention_transfer(attentionTransfer);
    }
    _gi->Broadcast( ExternalMessageRouter::WrapResponse( response ) );
  }
}

RecentOccurrenceTracker& AttentionTransferComponent::GetTracker(AttentionTransferReason reason)
{
  auto it = _transferTrackers.find(reason);
  if( it == _transferTrackers.end() ) {
    std::string debugName = "AttentionTransferReason::" + std::string(AttentionTransferReasonToString(reason));
    it = _transferTrackers.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(reason),
                                   std::forward_as_tuple(debugName)).first;
  }

  return it->second;
}


}
}
