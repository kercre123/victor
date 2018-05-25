/**
 * File: BehaviorTrackFace.cpp
 *
 * Author: Brad
 * Created: 2018-05-21
 *
 * Description: Test behavior for tracking faces
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorTrackFace.h"

#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kFaceSelectionPenaltiesKey = "faceSelectionPenalties";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BehaviorTrackFace::InstanceConfig
{
  FaceSelectionComponent::FaceSelectionFactorMap faceSelectionCriteria =
    FaceSelectionComponent::kDefaultSelectionCriteria;  
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BehaviorTrackFace::DynamicVariables
{
  SmartFaceID faceToTrack;
  bool actionStartedOnCharger = false;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackFace::BehaviorTrackFace(const Json::Value& config)
 : ICozmoBehavior(config)
 , _iConfig(new InstanceConfig)
 , _dVars(new DynamicVariables)
{
  if( !config[kFaceSelectionPenaltiesKey].isNull() ) {
    const bool parsedOK = FaceSelectionComponent::ParseFaceSelectionFactorMap(config[kFaceSelectionPenaltiesKey],
                                                                              _iConfig->faceSelectionCriteria);
    ANKI_VERIFY(parsedOK, "BehaviorTrackFace.InvalidFaceSelectionConfig",
                "behavior '%s' has invalid config",
                GetDebugLabel().c_str());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackFace::~BehaviorTrackFace()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTrackFace::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActivatableScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::Low} );
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );

  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kFaceSelectionPenaltiesKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::OnBehaviorActivated() 
{
  // don't reset variables here (keep tracked face persistent)
  if( _dVars->faceToTrack.IsValid() ) {

    PRINT_CH_INFO("Behaviors", "BehaviorTrackFace.Start",
                  "%s: starting by tracking face %s",
                  GetDebugLabel().c_str(),
                  _dVars->faceToTrack.GetDebugStr().c_str());

    BeginTracking();
  }
  // otherwise just wait for a face
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::BeginTracking()
{
  TrackFaceAction* trackAction = new TrackFaceAction(_dVars->faceToTrack);

  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
  if( onCharger ) {
    // TODO:(bn) something is "fighting" the eye move and re-centering the eyes each time
    trackAction->SetMoveEyes( true );
    trackAction->SetMode( ITrackAction::Mode::HeadOnly );
    _dVars->actionStartedOnCharger = true;
  }
  else {
    _dVars->actionStartedOnCharger = false;
  }
  
  DelegateIfInControl(trackAction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackFace::BehaviorUpdate() 
{
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  SmartFaceID bestFace = faceSelection.GetBestFaceToUse( _iConfig->faceSelectionCriteria );
  
  if( IsActivated() ) {

    const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
    const bool chargerChange = ( onCharger != _dVars->actionStartedOnCharger );
    const bool newFace = bestFace.IsValid() && ( bestFace != _dVars->faceToTrack );

    if( chargerChange || newFace ) {

      if( newFace ) {
        PRINT_CH_INFO("Behaviors", "BehaviorTrackFace.SwitchFaces",
                      "%s: switch from face %s to face %s",
                      GetDebugLabel().c_str(),
                      _dVars->faceToTrack.GetDebugStr().c_str(),
                      bestFace.GetDebugStr().c_str());
      }
      
      const bool allowCallback = false;
      CancelDelegates(allowCallback);
      _dVars->faceToTrack = bestFace;
      BeginTracking();
    }
  }
  else {
    // not activated, store best face
    _dVars->faceToTrack = bestFace;
  }
}

}
}
