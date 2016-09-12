/**
 * File: visuallyVerifyActions.cpp
 *
 * Author: Andrew Stein
 * Date:   6/16/2016
 *
 * Description: Actions for visually verifying the existance of objects or face.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
#pragma mark - 
#pragma mark IVisuallyVerifyAction
  
  IVisuallyVerifyAction::IVisuallyVerifyAction(Robot& robot,
                                               const std::string name,
                                               const RobotActionType type,
                                               VisionMode imageTypeToWaitFor,
                                               LiftPreset liftPosition)
  : IAction(robot,
            name,
            type,
            (u8)AnimTrackFlag::HEAD_TRACK)
  , _imageTypeToWaitFor(imageTypeToWaitFor)
  , _liftPreset(liftPosition)
  {
    
  }
  
  IVisuallyVerifyAction::~IVisuallyVerifyAction()
  {
    if(nullptr != _compoundAction) {
      _compoundAction->PrepForCompletion();
      Util::SafeDelete(_compoundAction);
    }
  }
  
  ActionResult IVisuallyVerifyAction::Init()
  {
    _compoundAction = new CompoundActionParallel(_robot, {
      new MoveLiftToHeightAction(_robot, _liftPreset),
      new WaitForImagesAction(_robot, GetNumImagesToWaitFor(), _imageTypeToWaitFor),
    });
    
    _compoundAction->ShouldSuppressTrackLocking(true);
    _compoundAction->ShouldEmitCompletionSignal(false);
    
    return InitInternal();
  }
  
  void IVisuallyVerifyAction::SetupEventHandler(EngineToGameTag tag, EventCallback callback)
  {
    _observationHandle = _robot.GetExternalInterface()->Subscribe(tag, callback);
  }
  
  ActionResult IVisuallyVerifyAction::CheckIfDone()
  {
    if(HaveSeenObject()) {
      // Saw what we're looking for!
      return ActionResult::SUCCESS;
    }
    
    // Keep waiting for lift to get out of the way and number of images to come in
    const ActionResult compoundResult = _compoundAction->Update();
    if(ActionResult::RUNNING != compoundResult)
    {
      PRINT_NAMED_INFO("IVisuallyVerifyAction.CheckIfDone.TimedOut",
                       "%s: Did not see object before processing %d images",
                       GetName().c_str(), GetNumImagesToWaitFor());
      
      return ActionResult::FAILURE_ABORT;
    }
    
    return ActionResult::RUNNING;
  }
  
#pragma mark -
#pragma mark VisuallyVerifyObjectAction
  
VisuallyVerifyObjectAction::VisuallyVerifyObjectAction(Robot& robot,
                                                       ObjectID objectID,
                                                       Vision::Marker::Code whichCode)
  : IVisuallyVerifyAction(robot,
                          "VisuallyVerifyObject" + std::to_string(objectID.GetValue()),
                          RobotActionType::VISUALLY_VERIFY_OBJECT,
                          VisionMode::DetectingMarkers,
                          LiftPreset::OUT_OF_FOV)
, _objectID(objectID)
, _whichCode(whichCode)
{
  
}

VisuallyVerifyObjectAction::~VisuallyVerifyObjectAction()
{

}

ActionResult VisuallyVerifyObjectAction::InitInternal()
{
  using namespace ExternalInterface;
  
  _objectSeen = false;
  
  auto obsObjLambda = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    const auto& objectObservation = event.GetData().Get_RobotObservedObject();
    // ID has to match and we have to actually have seen a marker (not just
    // saying part of the object is in FOV due to assumed projection)
    if(!_objectSeen && objectObservation.objectID == _objectID)
    {
      _objectSeen = true;
    }
  };
  
  SetupEventHandler(MessageEngineToGameTag::RobotObservedObject, obsObjLambda);
  
  if(_whichCode == Vision::Marker::ANY_CODE) {
    _markerSeen = true;
  } else {
    _markerSeen = false;
  }
  
  return ActionResult::SUCCESS;
}

bool VisuallyVerifyObjectAction::HaveSeenObject()
{
  if(_objectSeen)
  {
    if(!_markerSeen)
    {
      // We've seen the object, check if we've seen the correct marker if one was
      // specified and we haven't seen it yet
      ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_objectID);
      if(object == nullptr) {
        PRINT_NAMED_WARNING("VisuallyVerifyObjectAction.HaveSeenObject.ObjectNotFound",
                            "[%d] Object with ID=%d no longer exists in the world.",
                            GetTag(),
                            _objectID.GetValue());
        return false;
      }
      
      // Look for which markers were seen since (and including) last observation time
      std::vector<const Vision::KnownMarker*> observedMarkers;
      object->GetObservedMarkers(observedMarkers, object->GetLastObservedTime());
      
      for(auto marker : observedMarkers) {
        if(marker->GetCode() == _whichCode) {
          _markerSeen = true;
          break;
        }
      }
      
      if(!_markerSeen) {
        // Seeing wrong marker(s). Log this for help in debugging
        std::string observedMarkerNames;
        for(auto marker : observedMarkers) {
          observedMarkerNames += Vision::MarkerTypeStrings[marker->GetCode()];
          observedMarkerNames += " ";
        }
        
        PRINT_NAMED_INFO("VisuallyVerifyObjectAction.HaveSeenObject.WrongMarker",
                         "[%d] Have seen object %d, but not marker code %d. Have seen: %s",
                         GetTag(), _objectID.GetValue(), _whichCode, observedMarkerNames.c_str());
      }
    } // if(!_markerSeen)
    
    if(_markerSeen) {
      // We've seen the object and the correct marker: we're good to go!
      return true;
    }
    
  }
  
  return false;
  
} // VisuallyVerifyObjectAction::HaveSeenObject()

#pragma mark -
#pragma mark VisuallyVerifyFaceAction

VisuallyVerifyFaceAction::VisuallyVerifyFaceAction(Robot& robot, Vision::FaceID_t faceID)
: IVisuallyVerifyAction(robot,
                        "VisuallyVerifyFace" + std::to_string(faceID),
                        RobotActionType::VISUALLY_VERIFY_FACE,
                        VisionMode::DetectingFaces,
                        LiftPreset::LOW_DOCK)
, _faceID(faceID)
{
  
}

VisuallyVerifyFaceAction::~VisuallyVerifyFaceAction()
{
  
}

ActionResult VisuallyVerifyFaceAction::InitInternal()
{
  using namespace ExternalInterface;
  
  _faceSeen = false;
  
  auto obsFaceLambda = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    if(!_faceSeen)
    {
      if(_faceID == Vision::UnknownFaceID)
      {
        // Happy to see _any_ face
        _faceSeen = true;
      }
      else if(event.GetData().Get_RobotObservedFace().faceID == _faceID)
      {
        _faceSeen = true;
      }
    }
  };
  
  SetupEventHandler(MessageEngineToGameTag::RobotObservedFace, obsFaceLambda);
  
  return ActionResult::SUCCESS;
}

bool VisuallyVerifyFaceAction::HaveSeenObject()
{
  return _faceSeen;
}
  
} // namespace Cozmo
} // namesace Anki
