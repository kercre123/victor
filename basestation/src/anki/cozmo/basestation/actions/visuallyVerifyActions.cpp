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
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
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
          observedMarkerNames += marker->GetCodeName();
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

#pragma mark -
#pragma mark VisuallyVerifyNoObjectAtPoseAction

VisuallyVerifyNoObjectAtPoseAction::VisuallyVerifyNoObjectAtPoseAction(Robot& robot,
                                                                       const Pose3d& pose,
                                                                       const Point3f& thresholds_mm)
: IAction(robot,
          "VisuallyVerifyNoObjectAtPose",
          RobotActionType::VISUALLY_VERIFY_NO_OBJECT_AT_POSE,
          (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::BODY_TRACK)
, _pose(pose)
, _thresholds_mm(thresholds_mm)
{
  std::string name = "VisuallyVerifyNoObjectAtPose(";
  name += std::to_string((int)_pose.GetTranslation().x()) + ",";
  name += std::to_string((int)_pose.GetTranslation().y()) + ",";
  name += std::to_string((int)_pose.GetTranslation().z()) + ")";
  SetName(name);
  
  _filter.SetIgnoreFamilies({ObjectFamily::MarkerlessObject});
  // Augment the default filter (object not in unknown pose state) with one that
  // checks that this object was observed in the last frame
  _filter.AddFilterFcn([this](const ObservableObject* object)
                       {
                         if(object->GetLastObservedTime() >= _robot.GetLastImageTimeStamp())
                         {
                           return true;
                         }
                         return false;
                       });

}

VisuallyVerifyNoObjectAtPoseAction::~VisuallyVerifyNoObjectAtPoseAction()
{
  if(_turnTowardsPoseAction != nullptr)
  {
    _turnTowardsPoseAction->PrepForCompletion();
    Util::SafeDelete(_turnTowardsPoseAction);
  }
  
  if(_waitForImagesAction != nullptr)
  {
    _waitForImagesAction->PrepForCompletion();
    Util::SafeDelete(_waitForImagesAction);
  }
}

ActionResult VisuallyVerifyNoObjectAtPoseAction::Init()
{
  // Turn towards the pose and move the lift out of the way while we turn
  // then wait for a number of images
  _turnTowardsPoseAction = new CompoundActionParallel(_robot, {new TurnTowardsPoseAction(_robot, _pose, DEG_TO_RAD(180)),
                                                               new MoveLiftToHeightAction(_robot, MoveLiftToHeightAction::Preset::OUT_OF_FOV)});
  _waitForImagesAction = new WaitForImagesAction(_robot, _numImagesToWaitFor, VisionMode::DetectingMarkers);
  
  _turnTowardsPoseAction->ShouldEmitCompletionSignal(false);
  _turnTowardsPoseAction->ShouldSuppressTrackLocking(true);
  
  return ActionResult::SUCCESS;
}

void VisuallyVerifyNoObjectAtPoseAction::AddIgnoreID(const ObjectID& objID)
{
  if (HasStarted()) {
    // You're too late! Set objects to ignore before you start the action!
    PRINT_NAMED_WARNING("VisuallyVerifyNoObjectAtPoseAciton.AddIgnoreID.ActionAlreadyStarted", "");
  } else {
    _filter.AddIgnoreID(objID);
  }
}

ActionResult VisuallyVerifyNoObjectAtPoseAction::CheckIfDone()
{
  // Tick the turnTowardsPoseAction first until it completes and then delete it
  if(_turnTowardsPoseAction != nullptr)
  {
    ActionResult res = _turnTowardsPoseAction->Update();
    if(res != ActionResult::SUCCESS)
    {
      return res;
    }
    
    _turnTowardsPoseAction->PrepForCompletion();
    Util::SafeDelete(_turnTowardsPoseAction);
    
    return ActionResult::RUNNING;
  }
  // If the turnTowardsPoseAction is null then it must have completed so tick the waitForImagesAction
  // if it succeeds then that means we went _numImagesToWaitFor without seeing an object close to _pose so
  // this action will succeed
  else if(_waitForImagesAction != nullptr)
  {
    ActionResult res = _waitForImagesAction->Update();
    
    // If there is an object at the given pose within the threshold then fail
    // Only do this check once we have turned towards the pose and have started waiting for images in case
    // there isn't actually an object at the pose but blockworld thinks there is
    if(_robot.GetBlockWorld().FindObjectClosestTo(_pose, _thresholds_mm, _filter) != nullptr)
    {
      PRINT_CH_DEBUG("Actions", "VisuallyVerifyNoObjectAtPose.FoundObject",
                     "Seeing object near pose (%f %f %f)",
                     _pose.GetTranslation().x(),
                     _pose.GetTranslation().y(),
                     _pose.GetTranslation().z());
      return ActionResult::FAILURE_ABORT;
    }
    
    return res;
  }
  
  PRINT_NAMED_WARNING("VisuallyVerifyNoObjectAtPoseAction.NullSubActions",
                      "Both subActions are null returning failure");
  
  return ActionResult::FAILURE_ABORT;
}


  
} // namespace Cozmo
} // namesace Anki
