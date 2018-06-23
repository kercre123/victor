/**
* File: externalMessageRouter.cpp
*
* Author: ross
* Created: may 31 2018
*
* Description: Templates to automatically wrap messages included in the MessageEngineToGame union
*              and the GatewayWrapper protobuf oneof (union) based on external requirements and
*              event organization (the hierarchy in MessageEngineToGame clad).
*
* Copyright: Anki, Inc. 2018
*/

#include "engine/externalInterface/externalMessageRouter.h"
#include "proto/external_interface/messages.pb.h"

namespace Anki {
namespace Cozmo {


// -------------------------------------------------------------------------------------------------
// Outbound Proto Messages
// -------------------------------------------------------------------------------------------------
  
// Currently this has to be done for each message type because of protobuf limitations compared
// to CLAD. TODO (VIC-3880): extend generated code to allow for templated versions, like how we do it for
// clad events in the header.
// For now, use macros to make this easier.
  
#define WRAP_RESPONSE_MESSAGE(T,t) \
template<> \
external_interface::GatewayWrapper ExternalMessageRouter::WrapResponse<T>( T* msg ) \
{ \
  external_interface::GatewayWrapper wrapper; \
  wrapper.set_allocated_##t( msg ); \
  return wrapper; \
}
  
#define WRAP_EVENT_MESSAGE_ONEOF(S,T,t)               \
template<> \
external_interface::GatewayWrapper ExternalMessageRouter::Wrap<T>( T* msg ) \
{ \
  external_interface::GatewayWrapper wrapper; \
  wrapper.mutable_event()->mutable_##S()->set_allocated_##t( msg ); \
  return wrapper; \
}

#define WRAP_EVENT_MESSAGE(T,t)               \
template<> \
external_interface::GatewayWrapper ExternalMessageRouter::Wrap<T>( T* msg ) \
{ \
  external_interface::GatewayWrapper wrapper; \
  wrapper.mutable_event()->set_allocated_##t( msg ); \
  return wrapper; \
}

WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingState,           onboarding_state );
WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingOnCharger,       onboarding_on_charger );
WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingLowBattery,      onboarding_battery );
WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingPickedUp,        onboarding_picked_up );
WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingSeesCube,        onboarding_sees_cube );
WRAP_EVENT_MESSAGE_ONEOF( onboarding, external_interface::OnboardingCantFindCube,    onboarding_cant_find_cube );
  
WRAP_EVENT_MESSAGE_ONEOF( wake_word, external_interface::WakeWordBegin,    wake_word_begin );
WRAP_EVENT_MESSAGE_ONEOF( wake_word, external_interface::WakeWordEnd,      wake_word_end );

WRAP_EVENT_MESSAGE( external_interface::AttentionTransfer, attention_transfer);

WRAP_RESPONSE_MESSAGE( external_interface::OnboardingState,             onboarding_state );
WRAP_RESPONSE_MESSAGE( external_interface::OnboardingContinueResponse,  onboarding_continue_response );

WRAP_RESPONSE_MESSAGE( external_interface::LatestAttentionTransfer, latest_attention_transfer );


} // end namespace Cozmo
} // end namespace Anki
