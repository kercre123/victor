/**
* File: cladProtoTypeTranslator.h
*
* Author: ross
* Created: jun 18 2018
*
* Description: Guards and helpers make sure translation between clad and proto enum types is safe,
*              in case the underlying values or field numbers change.
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __Engine_ExternalInterface_CladProtoTypeTranslator_H__
#define __Engine_ExternalInterface_CladProtoTypeTranslator_H__


#include "proto/external_interface/messages.pb.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/behaviorComponent/attentionTransferTypes.h"


namespace Anki {
namespace Cozmo {

namespace CladProtoTypeTranslator {

  constexpr external_interface::OnboardingStages ToProtoEnum( OnboardingStages value ){
    return static_cast<external_interface::OnboardingStages>( static_cast<std::underlying_type_t<OnboardingStages>>(value) );
  }

  constexpr external_interface::AttentionTransferReason ToProtoEnum( AttentionTransferReason value ){
    return static_cast<external_interface::AttentionTransferReason>( static_cast<std::underlying_type_t<AttentionTransferReason>>(value) );
  }

  #define CLAD_PROTO_COMPARE_ASSERT(T,V) static_assert(ToProtoEnum(T::V) == external_interface::T::V, "Invalid cast " #T "::" #V )
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, NotStarted);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, FinishedComeHere);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, FinishedMeetVictor);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, Complete);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, DevDoNothing);

  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, Invalid);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, NoCloudConnection);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, PhotoStorageFull);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, CantFindCharger);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, UnmatchedIntent);

}


} // end namespace Cozmo
} // end namespace Anki

#endif //__Engine_ExternalInterface_CladProtoTypeTranslator_H__
