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
  
  constexpr external_interface::FaceEnrollmentResult ToProtoEnum( FaceEnrollmentResult value ){
    return static_cast<external_interface::FaceEnrollmentResult>( static_cast<std::underlying_type_t<FaceEnrollmentResult>>(value) );
  }

  #define CLAD_PROTO_COMPARE_ASSERT(T,V) static_assert(ToProtoEnum(T::V) == external_interface::T::V, "Invalid cast " #T "::" #V )
  #define CLAD_PROTO_COMPARE_ASSERT2(T,V,U) static_assert(ToProtoEnum(T::V) == external_interface::T::U, "Invalid cast " #T "::" #V " to " #T "::" #U )
  
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, NotStarted);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, FinishedComeHere);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, FinishedMeetVictor);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, Complete);
  CLAD_PROTO_COMPARE_ASSERT(OnboardingStages, DevDoNothing);

  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, Invalid);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, NoCloudConnection);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, NoWifi);
  CLAD_PROTO_COMPARE_ASSERT(AttentionTransferReason, UnmatchedIntent);
  
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, Success, SUCCESS);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, SawWrongFace, SAW_WRONG_FACE);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, SawMultipleFaces, SAW_MULTIPLE_FACES);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, TimedOut, TIMED_OUT);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, SaveFailed, SAVE_FAILED);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, Incomplete, INCOMPLETE);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, Cancelled, CANCELLED);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, NameInUse, NAME_IN_USE);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, NamedStorageFull, NAMED_STORAGE_FULL);
  CLAD_PROTO_COMPARE_ASSERT2(FaceEnrollmentResult, UnknownFailure, UNKNOWN_FAILURE);
}


} // end namespace Cozmo
} // end namespace Anki

#endif //__Engine_ExternalInterface_CladProtoTypeTranslator_H__
