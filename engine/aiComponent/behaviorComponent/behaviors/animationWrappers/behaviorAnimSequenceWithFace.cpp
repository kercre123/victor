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
#include "engine/aiComponent/faceSelectionComponent.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kFaceSelectionPenaltiesKey = "faceSelectionPenalties";
}

struct BehaviorAnimSequenceWithFace::InstanceConfig
{
  FaceSelectionComponent::FaceSelectionFactorMap faceSelectionCriteria = FaceSelectionComponent::kDefaultSelectionCriteria;
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequenceWithFace::BehaviorAnimSequenceWithFace(const Json::Value& config)
  : BaseClass(config)
  , _iConfig(new InstanceConfig)
{
  if( !config[kFaceSelectionPenaltiesKey].isNull() ) {
    const bool parsedOK = FaceSelectionComponent::ParseFaceSelectionFactorMap(config[kFaceSelectionPenaltiesKey],
                                                                             _iConfig->faceSelectionCriteria);
    ANKI_VERIFY(parsedOK, "BehaviorAnimSequenceWithFace.InvalidFaceSelectionConfig",
                "behavior '%s' has invalid config",
                GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAnimSequenceWithFace::~BehaviorAnimSequenceWithFace()
{
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
  expectedKeys.insert(kFaceSelectionPenaltiesKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAnimSequenceWithFace::OnBehaviorActivated()
{
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  SmartFaceID bestFace = faceSelection.GetBestFaceToUse( _iConfig->faceSelectionCriteria );
  if( bestFace.IsValid() ) {
    // attempt to turn towards last face, and even if fails, move on to the animations
    DelegateIfInControl(new TurnTowardsFaceAction(bestFace), [this]() {
        BaseClass::StartPlayingAnimations();
      });
  }
  else {
    // otherwise skip straight to the animation
    BaseClass::StartPlayingAnimations();

    // NOTE: we could fall back to the "last face pose" here in the case when all faces have times out of face
    // world. This may be a useful feature in the future and was used on Cozmo, but my hope is that the
    // increased field of view will increase the chances of there being a face around
  }  
}

} // namespace Cozmo
} // namespace Anki
