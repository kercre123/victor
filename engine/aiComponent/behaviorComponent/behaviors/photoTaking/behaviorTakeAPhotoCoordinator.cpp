/**
 * File: BehaviorTakeAPhotoCoordinator.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-05
 *
 * Description: Behavior which handles the behavior flow when the user wants to take a photo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/photoTaking/behaviorTakeAPhotoCoordinator.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/components/photographyManager.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTakeAPhotoCoordinator::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTakeAPhotoCoordinator::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTakeAPhotoCoordinator::BehaviorTakeAPhotoCoordinator(const Json::Value& config)
 : ICozmoBehavior(config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTakeAPhotoCoordinator::~BehaviorTakeAPhotoCoordinator()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTakeAPhotoCoordinator::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool isPhotoPending = uic.IsUserIntentPending(USER_INTENT(take_a_photo));
  return isPhotoPending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.frameFacesBehavior.get());
  delegates.insert(_iConfig.takePhotoAnimationsBehavior.get());
  delegates.insert(_iConfig.storageIsFullBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.frameFacesBehavior          = BC.FindBehaviorByID(BEHAVIOR_ID(FrameFaces));
  _iConfig.takePhotoAnimationsBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(TakingPhotoAnimation));
  _iConfig.storageIsFullBehavior       = BC.FindBehaviorByID(BEHAVIOR_ID(SingletonICantDoThat));

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  UserIntentPtr intentData = SmartActivateUserIntent(USER_INTENT(take_a_photo));
  const bool isStorageFull = GetBEI().GetPhotographyManager().IsPhotoStorageFull();

  if(isStorageFull){
    TransitionToStorageIsFull();
  }else if(intentData != nullptr){
    const bool isASelfie = !(intentData->intent.Get_take_a_photo().empty_or_selfie.empty());
    // If we're taking a selfie we need to center the faces first - otherwise just take a photo
    if(isASelfie){
      TransitionToFrameFaces();
    }else{
      TransitionToTakeAPhotoAnimations();
    }
  }else{
    PRINT_NAMED_ERROR("BehaviorTakeAPhotoCoordinator.OnBehaviorActivated.NullIntentData",
                      "");
  }

  GetBEI().GetPhotographyManager().EnablePhotoMode(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::OnBehaviorDeactivated()
{
  GetBEI().GetPhotographyManager().EnablePhotoMode(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::TransitionToStorageIsFull()
{
  ANKI_VERIFY(_iConfig.storageIsFullBehavior->WantsToBeActivated(), 
              "BehaviorTakeAPhotoCoordinator.TransitionToStorageIsFull.DoesNotWantToBeActivated", 
              "");
  DelegateIfInControl(_iConfig.storageIsFullBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::TransitionToFrameFaces()
{
  ANKI_VERIFY(_iConfig.frameFacesBehavior->WantsToBeActivated(),
              "BehaviorTakeAPhotoCoordinator.TransitionToFrameFaces.DoesNotWantToBeActivated", 
              "");
  DelegateIfInControl(_iConfig.frameFacesBehavior.get(), 
                      &BehaviorTakeAPhotoCoordinator::TransitionToTakeAPhotoAnimations);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::TransitionToTakeAPhotoAnimations()
{
  ANKI_VERIFY(_iConfig.takePhotoAnimationsBehavior->WantsToBeActivated(), 
              "BehaviorTakeAPhotoCoordinator.TransitionToTakeAPhotoAnimations.DoesNotWantToBeActivated", 
              "");
  DelegateIfInControl(_iConfig.takePhotoAnimationsBehavior.get(), 
                      &BehaviorTakeAPhotoCoordinator::CaptureCurrentImage);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTakeAPhotoCoordinator::CaptureCurrentImage()
{
  GetBEI().GetPhotographyManager().TakePhoto();
}

}
}
