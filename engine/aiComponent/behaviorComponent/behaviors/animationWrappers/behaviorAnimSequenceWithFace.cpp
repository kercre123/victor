/**
 * File: behaviorAnimSequenceWithFace.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-05-23
 *
 * Description:  a sequence of animations after turning to a face (if possible)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequenceWithFace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/faceSelectionComponent.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kFaceSelectionPenaltiesKey = "faceSelectionPenalties";
static const char* kRequireFaceForActivationKey = "requireFaceForActivation";
static const char* kRequireFaceConfirmationKey = "requireFaceConfirmation";
static const char* kReturnToOriginalPoseKey = "returnToOriginalPose";
}

struct BehaviorAnimSequenceWithFace::InstanceConfig
{
  FaceSelectionComponent::FaceSelectionFactorMap faceSelectionCriteria = FaceSelectionComponent::kDefaultSelectionCriteria;
  bool requireFaceForActivation;
  bool requireFaceConfirmation;
  bool returnToOriginalPose;
};
  
struct BehaviorAnimSequenceWithFace::DynamicVariables
{
  Radians startAbsAngle_rad;
};
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequenceWithFace::BehaviorAnimSequenceWithFace(const Json::Value& config)
  : BaseClass(config)
  , _iConfig(new InstanceConfig)
  , _dVars(new DynamicVariables)
{
  if( !config[kFaceSelectionPenaltiesKey].isNull() ) {
    const bool parsedOK = FaceSelectionComponent::ParseFaceSelectionFactorMap(config[kFaceSelectionPenaltiesKey],
                                                                             _iConfig->faceSelectionCriteria);
    ANKI_VERIFY(parsedOK, "BehaviorAnimSequenceWithFace.InvalidFaceSelectionConfig",
                "behavior '%s' has invalid config",
                GetDebugLabel().c_str());
  }
  
  _iConfig->requireFaceForActivation = config.get( kRequireFaceForActivationKey, false ).asBool();
  _iConfig->requireFaceConfirmation = config.get( kRequireFaceConfirmationKey, false ).asBool();
  _iConfig->returnToOriginalPose = config.get( kReturnToOriginalPoseKey, false ).asBool();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequenceWithFace::~BehaviorAnimSequenceWithFace()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAnimSequenceWithFace::WantsToBeActivatedAnimSeqInternal() const
{
  if( _iConfig->requireFaceForActivation ) {
    SmartFaceID bestFace = GetBestFace();
    const bool hasFace = bestFace.IsValid();
    return hasFace;
  } else {
    return true;
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });

  BaseClass::GetBehaviorOperationModifiers(modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  BaseClass::GetBehaviorJsonKeys(expectedKeys);
  const char* list[] = {
    kFaceSelectionPenaltiesKey,
    kRequireFaceForActivationKey,
    kRequireFaceConfirmationKey,
    kReturnToOriginalPoseKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::OnBehaviorActivated()
{
  _dVars->startAbsAngle_rad = GetBEI().GetRobotInfo().GetPose().GetRotationAngle<'Z'>();
  SmartFaceID bestFace = GetBestFace();
  if( bestFace.IsValid() ) {
    // attempt to turn towards last face, and even if fails, move on to the animations
    auto* action = new TurnTowardsFaceAction( bestFace );
    action->SetRequireFaceConfirmation( _iConfig->requireFaceConfirmation );
    DelegateIfInControl(action, [this]() {
        BaseClass::StartPlayingAnimations();
      });
  }
  else {
    ANKI_VERIFY( !_iConfig->requireFaceForActivation,
                 "BehaviorAnimSequenceWithFace.OnBehaviorActivated.ActivatedWithoutFace",
                 "This behavior activated without a known face, but config requires a face" );
    // otherwise skip straight to the animation
    BaseClass::StartPlayingAnimations();

    // NOTE: we could fall back to the "last face pose" here in the case when all faces have times out of face
    // world. This may be a useful feature in the future and was used on Cozmo, but my hope is that the
    // increased field of view will increase the chances of there being a face around
  }  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SmartFaceID BehaviorAnimSequenceWithFace::GetBestFace() const
{
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  SmartFaceID bestFace = faceSelection.GetBestFaceToUse( _iConfig->faceSelectionCriteria );
  return bestFace;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::OnAnimationsComplete()
{
  if( _iConfig->returnToOriginalPose ) {
    const bool isAbsolute = true;
    auto* action = new TurnInPlaceAction{ _dVars->startAbsAngle_rad.ToFloat(), isAbsolute };
    DelegateIfInControl( action );
  }
  // otherwise do nothing and the behavior should exit
}

} // namespace Cozmo
} // namespace Anki
