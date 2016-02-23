/**
* File: robotImplMessageing
*
* Author: damjan stulic
* Created: 9/9/15
*
* Description:
* robot class methods specific to message handling
*
* Copyright: Anki, inc. 2015
*
*/

#include <opencv2/imgproc.hpp>

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/helpers/printByteArray.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/robotStatusAndActions.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeFstream.h"
#include <functional>

// Whether or not to handle prox obstacle events
#define HANDLE_PROX_OBSTACLES 0

// Prints the IDs of the active blocks that are on but not currently
// talking to a robot. Prints roughly once/sec.
#define PRINT_UNCONNECTED_ACTIVE_OBJECT_IDS 0


namespace Anki {
namespace Cozmo {
  
using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;

void Robot::InitRobotMessageComponent(RobotInterface::MessageHandler* messageHandler, RobotID_t robotId)
{
  // bind to specific handlers in the robot class
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::cameraCalibration,
    std::bind(&Robot::HandleCameraCalibration, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::printText,
    std::bind(&Robot::HandlePrint, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::trace,
    std::bind(&Robot::HandleTrace, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::crashReport,
    std::bind(&Robot::HandleCrashReport, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::blockPickedUp,
    std::bind(&Robot::HandleBlockPickedUp, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::blockPlaced,
    std::bind(&Robot::HandleBlockPlaced, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectDiscovered,
    std::bind(&Robot::HandleActiveObjectDiscovered, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectConnectionState,
    std::bind(&Robot::HandleActiveObjectConnectionState, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectMoved,
    std::bind(&Robot::HandleActiveObjectMoved, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectStopped,
    std::bind(&Robot::HandleActiveObjectStopped, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::activeObjectTapped,
    std::bind(&Robot::HandleActiveObjectTapped, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::goalPose,
    std::bind(&Robot::HandleGoalPose, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::cliffEvent,
    std::bind(&Robot::HandleCliffEvent, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::proxObstacle,
    std::bind(&Robot::HandleProxObstacle, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::chargerEvent,
   std::bind(&Robot::HandleChargerEvent, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::image,
    std::bind(&Robot::HandleImageChunk, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::imuDataChunk,
    std::bind(&Robot::HandleImuData, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::imuRawDataChunk,
    std::bind(&Robot::HandleImuRawData, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::syncTimeAck,
    std::bind(&Robot::HandleSyncTimeAck, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::robotPoked,
    std::bind(&Robot::HandleRobotPoked, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::nvData,
    std::bind(&Robot::HandleNVData, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::nvResult,
    std::bind(&Robot::HandleNVOpResult, this, std::placeholders::_1)));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::robotAvailable,
                                                     std::bind(&Robot::HandleRobotSetID, this, std::placeholders::_1)));
  

  // lambda wrapper to call internal handler
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::state,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotState& payload = message.GetData().Get_state();
      UpdateFullRobotState(payload);
    }));


  // lambda for some simple message handling
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::animState,
     [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
       if (_timeSynced) {
         _numAnimationBytesPlayed = message.GetData().Get_animState().numAnimBytesPlayed;
         _numAnimationAudioFramesPlayed = message.GetData().Get_animState().numAudioFramesPlayed;
         _enabledAnimTracks = message.GetData().Get_animState().enabledAnimTracks;
         _animationTag = message.GetData().Get_animState().tag;
       }
     }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseStarted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a ramp.", GetID());
      SetOnRamp(true);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::rampTraverseCompleted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a ramp.", GetID());
      SetOnRamp(false);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseStarted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it started traversing a bridge.", GetID());
      //SetOnBridge(true);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::bridgeTraverseCompleted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d reported it completed traversing a bridge.", GetID());
      //SetOnBridge(false);
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::chargerMountCompleted,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage", "Robot %d charger mount %s.", GetID(), message.GetData().Get_chargerMountCompleted().didSucceed ? "SUCCEEDED" : "FAILED" );
      if (message.GetData().Get_chargerMountCompleted().didSucceed) {
        SetPoseOnCharger();
      }
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::mainCycleTimeError,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotInterface::MainCycleTimeError& payload = message.GetData().Get_mainCycleTimeError();
      if (payload.numMainTooLongErrors > 0) {
        PRINT_NAMED_WARNING("Robot.MainCycleTooLong", " %d Num errors: %d, Avg time: %d us", GetID(), payload.numMainTooLongErrors, payload.avgMainTooLongTime);
      }
      if (payload.numMainTooLateErrors > 0) {
        PRINT_NAMED_WARNING("Robot.MainCycleTooLate", "%d Num errors: %d, Avg time: %d us", GetID(), payload.numMainTooLateErrors, payload.avgMainTooLateTime);
      }
    }));
  _signalHandles.push_back(messageHandler->Subscribe(robotId, RobotInterface::RobotToEngineTag::dataDump,
    [this](const AnkiEvent<RobotInterface::RobotToEngine>& message){
      const RobotInterface::DataDump& payload = message.GetData().Get_dataDump();
      char buf[payload.data.size() * 2 + 1];
      FormatBytesAsHex((char *)payload.data.data(), (int)payload.data.size(), buf, (int)sizeof(buf));
      PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageDataDump", "ID: %d, size: %zd, data: %s", GetID(), payload.data.size(), buf);
    }));
}


void Robot::HandleCameraCalibration(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::CameraCalibration& payload = message.GetData().Get_cameraCalibration();
  PRINT_NAMED_INFO("RobotMessageHandler.CameraCalibration",
    "Received new %dx%d camera calibration from robot.", payload.ncols, payload.nrows);

  // Convert calibration message into a calibration object to pass to
  // the robot
  Vision::CameraCalibration calib(payload.nrows,
    payload.ncols,
    payload.focalLength_x,
    payload.focalLength_y,
    payload.center_x,
    payload.center_y,
    payload.skew);

  _visionComponent.SetCameraCalibration(*this, calib);  // Set intrisic calibration
  SetCameraRotation(0, 0, 0);                           // Set extrinsic calibration (rotation only, assuming known position)
                                                        // TODO: Set these from rotation calibration info to be sent in CameraCalibration message
                                                        //       and/or when we do on-engine calibration with images of tool code.
  
  SetPhysicalRobot(payload.isPhysicalRobots);  
}
  
void Robot::HandleRobotSetID(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::RobotAvailable& payload = message.GetData().Get_robotAvailable();
  // Set DAS Global on all messages
  char string_id[8];
  snprintf(string_id, sizeof(string_id), "%08x", payload.robotID);
  Anki::Util::sSetGlobal(DPHYS, string_id);
}

void Robot::HandlePrint(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::PrintText& payload = message.GetData().Get_printText();
  printf("ROBOT-PRINT (%d): %s", GetID(), payload.text.c_str());
}

void Robot::HandleTrace(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _traceHandler.HandleTrace(message);
}

void Robot::HandleCrashReport(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _traceHandler.HandleCrashReport(message);
}

void Robot::HandleBlockPickedUp(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const BlockPickedUp& payload = message.GetData().Get_blockPickedUp();
  const char* successStr = (payload.didSucceed ? "succeeded" : "failed");
  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPickedUp",
    "Robot %d reported it %s picking up block. Stopping docking and turning on Look-for-Markers mode.", GetID(), successStr);

  if(payload.didSucceed) {
    SetDockObjectAsAttachedToLift();
    SetLastPickOrPlaceSucceeded(true);
  }
  else {
    SetLastPickOrPlaceSucceeded(false);
  }

  // Note: this returns the vision system to whatever mode it was in before
  // it was docking/tracking
  _visionComponent.EnableMode(VisionMode::Tracking, false);
}

void Robot::HandleBlockPlaced(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const BlockPlaced& payload = message.GetData().Get_blockPlaced();
  const char* successStr = (payload.didSucceed ? "succeeded" : "failed");
  PRINT_NAMED_INFO("RobotMessageHandler.ProcessMessage.MessageBlockPlaced",
    "Robot %d reported it %s placing block. Stopping docking and turning on Look-for-Markers mode.", GetID(), successStr);

  if(payload.didSucceed) {
    SetCarriedObjectAsUnattached();
    SetLastPickOrPlaceSucceeded(true);
  }
  else {
    SetLastPickOrPlaceSucceeded(false);
  }

  _visionComponent.EnableMode(VisionMode::DetectingMarkers, true);
  _visionComponent.EnableMode(VisionMode::Tracking, false);

}
  
void Robot::HandleActiveObjectDiscovered(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
#if(PRINT_UNCONNECTED_ACTIVE_OBJECT_IDS)
  const ObjectDiscovered payload = message.GetData().Get_activeObjectDiscovered();
  if (payload.factory_id < s32_MAX) {  // Ignore chargers which have MSB set
    PRINT_NAMED_INFO("ActiveObjectDiscovered", "%8x", payload.factory_id);
  }
#endif
}

 
void Robot::HandleActiveObjectConnectionState(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
#if(OBJECTS_HEARABLE)
  ObjectConnectionState payload = message.GetData().Get_activeObjectConnectionState();
  
  ObjectID objID;
  if (payload.connected) {
    // Add cube to blockworld if not already there
    objID = GetBlockWorld().AddLightCube(payload.objectID, payload.factoryID);
    if (objID.IsSet()) {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Connected",
                       "Object %d (activeID %d, factoryID 0x%x)",
                       objID.GetValue(), payload.objectID, payload.factoryID);
      
      // Turn off lights upon connection
      std::array<Anki::Cozmo::LightState, 4> lights{}; // Use the default constructed, empty light structure
      SendRobotMessage<CubeLights>(lights, payload.objectID);
    }
  } else {
    // Remove cube from blockworld if it exists
    ObservableObject* obj = GetBlockWorld().GetActiveObjectByActiveID(payload.objectID);
    if (obj) {
      GetBlockWorld().ClearObject(obj);
      objID = obj->GetID();
      PRINT_NAMED_INFO("Robot.HandleActiveObjectConnectionState.Disconnected",
                       "Object %d (activeID %d, factoryID 0x%x)",
                       objID.GetValue(), payload.objectID, payload.factoryID);
    }
  }
  

  if (objID.IsSet()) {
    // Update the objectID to be blockworld ID
    payload.objectID = objID.GetValue();
  
    // Forward on to game
    Broadcast(ExternalInterface::MessageEngineToGame(ObjectConnectionState(payload)));
  }
#endif
}
  

void Robot::HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectMoved payload = message.GetData().Get_activeObjectMoved();
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID
  ObservableObject* object = GetBlockWorld().GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("Robot.HandleActiveObjectMoved.UnknownActiveID",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  // Ignore move messages for objects we are docking to, since we expect to bump them
  else if(object->GetID() != GetDockObject())
  {
    ASSERT_NAMED(object->IsActive(), "Got movement message from non-active object?");
    
    if(object->GetPoseState() == ObservableObject::PoseState::Known)
    {
      PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.ActiveObjectMoved",
                       "Received message that %s %d (Active ID %d) moved. Delocalizing it.",
                       EnumToString(object->GetType()),
                       object->GetID().GetValue(), object->GetActiveID());
      
      // Once an object moves, we can no longer use it for localization because
      // we don't know where it is anymore. Next time we see it, relocalize it
      // relative to robot's pose estimate. Then we can use it for localization
      // again.
      object->SetPoseState(ObservableObject::PoseState::Dirty);
      
      // If this is the object we were localized to, unset our localizedToID.
      // Note we are still "localized" by odometry, however.
      if(GetLocalizedTo() == object->GetID()) {
        ASSERT_NAMED(IsLocalized(), "Robot should think it is localized if GetLocalizedTo is set to something.");
        PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.UnsetLocalzedToID",
                         "Unsetting %s %d, which moved, as robot %d's localization object.",
                         ObjectTypeToString(object->GetType()), object->GetID().GetValue(), GetID());
        SetLocalizedTo(nullptr);
      } else if(!IsLocalized() && !IsPickedUp()) {
        // If we are not localized and there is nothing else left in the world that
        // we could localize to, then go ahead and mark us as localized (via
        // odometry alone)
        if(false == _blockWorld.AnyRemainingLocalizableObjects()) {
          PRINT_NAMED_INFO("Robot.HandleActiveObjectMoved.NoMoreRemainingLocalizableObjects",
                           "Marking previously-unlocalized robot %d as localized to odometry because "
                           "there are no more objects to localize to in the world.", GetID());
          SetLocalizedTo(nullptr);
        }
      }
      
    }
    
    // Don't notify game about moving objects that are being carried
    ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
    assert(actionObject != nullptr);
    if(!actionObject->IsBeingCarried()) {
      // Update the ID to be the blockworld ID before broadcasting
      payload.objectID = object->GetID();
      Broadcast(ExternalInterface::MessageEngineToGame(ObjectMoved(payload)));
    }
  }

  // Set moving state of object
  object->SetIsMoving(true, payload.timestamp);
}

void Robot::HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectStoppedMoving payload = message.GetData().Get_activeObjectStopped();
  
  // The message from the robot has the active object ID in it, so we need
  // to find the object in blockworld (which has its own bookkeeping ID) that
  // has the matching active ID
  ObservableObject* object = GetBlockWorld().GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_WARNING("Robot.HandleActiveObjectStopped.UnknownActiveID",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  // Ignore stopped-moving messages for objects we are docking to, since we expect to bump them
  else if(object->GetID() != GetDockObject())
  {
    ASSERT_NAMED(object->IsActive(), "Got movement message from non-active object?");

    PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.MessageActiveObjectStoppedMoving",
                     "Received message that %s %d (Active ID %d) stopped moving.",
                     EnumToString(object->GetType()),
                     object->GetID().GetValue(), payload.objectID);
    
    if(object->GetPoseState() == ObservableObject::PoseState::Known) {
      // Not sure how an object could have a known pose before it stopped moving,
      // but just to be safe, re-delocalize and force a re-localization now
      // that we've gotten the stopped-moving message.
      object->SetPoseState(ObservableObject::PoseState::Dirty);
      
      // If this is the object we were localized to, unset our localizedToID.
      // Note we are still "localized" by odometry, however.
      if(GetLocalizedTo() == object->GetID()) {
        ASSERT_NAMED(IsLocalized(), "Robot should think it is localized if GetLocalizedTo is set to something.");
        PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.UnsetLocalzedToID",
                         "Unsetting %s %d, which stopped moving, as robot %d's localization object.",
                         ObjectTypeToString(object->GetType()), object->GetID().GetValue(), GetID());
        SetLocalizedTo(nullptr);
      } else if(!IsLocalized() && !IsPickedUp()) {
        // If we are not localized and there is nothing else left in the world that
        // we could localize to, then go ahead and mark us as localized (via
        // odometry alone)
        if(false == _blockWorld.AnyRemainingLocalizableObjects()) {
          PRINT_NAMED_INFO("Robot.HandleActiveObjectStopped.NoMoreRemainingLocalizableObjects",
                           "Marking previously-unlocalized robot %d as localized to odometry because "
                           "there are no more objects to localize to in the world.", GetID());
          SetLocalizedTo(nullptr);
        }
      }
    }
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = object->GetID();
    Broadcast(ExternalInterface::MessageEngineToGame(ObjectStoppedMoving(payload)));
  }

  // Set moving state of object
  object->SetIsMoving(false, payload.timestamp);
}

void Robot::HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectTapped payload = message.GetData().Get_activeObjectTapped();
  ObservableObject* object = GetBlockWorld().GetActiveObjectByActiveID(payload.objectID);
  
  if(nullptr == object)
  {
    PRINT_NAMED_INFO("Robot.HandleActiveObjectTapped.UnknownActiveID",
                     "Could not find match for active object ID %d", payload.objectID);
  } else {
    assert(object->IsActive());
    
    PRINT_NAMED_INFO("Robot.HandleActiveObjectTapped.MessageActiveObjectTapped",
                     "Received message that %s %d (Active ID %d) was tapped %d times.",
                     EnumToString(object->GetType()),
                     object->GetID().GetValue(), payload.objectID, payload.numTaps);
    
    // Update the ID to be the blockworld ID before broadcasting
    payload.objectID = object->GetID();
    Broadcast(ExternalInterface::MessageEngineToGame(ObjectTapped(payload)));
  }
}

void Robot::HandleGoalPose(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const GoalPose& payload = message.GetData().Get_goalPose();
  Anki::Pose3d p(payload.pose.angle, Z_AXIS_3D(),
    Vec3f(payload.pose.x, payload.pose.y, payload.pose.z));
  //PRINT_INFO("Goal pose: x=%f y=%f %f deg (%d)", msg.pose_x, msg.pose_y, RAD_TO_DEG_F32(msg.pose_angle), msg.followingMarkerNormal);
  if (payload.followingMarkerNormal) {
    GetContext()->GetVizManager()->DrawPreDockPose(100, p, ::Anki::NamedColors::RED);
  } else {
    GetContext()->GetVizManager()->DrawPreDockPose(100, p, ::Anki::NamedColors::GREEN);
  }
}

  

void Robot::HandleCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  CliffEvent cliffEvent = message.GetData().Get_cliffEvent();
  if (cliffEvent.detected) {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Detected", "at %f,%f while driving %s",
                     cliffEvent.x_mm, cliffEvent.y_mm, cliffEvent.drivingForward ? "forwards" : "backwards");
    
    // Stop whatever we were doing
    GetActionList().Cancel();
    
    // Add cliff obstacle
    Pose3d cliffPose(cliffEvent.angle_rad, Z_AXIS_3D(), {cliffEvent.x_mm, cliffEvent.y_mm, 0}, GetWorldOrigin());
    _blockWorld.AddCliff(cliffPose);
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleCliffEvent.Undetected", "");
  }

  _blockWorld.SetIsOnCliff(cliffEvent.detected);
  
  // Forward on with EngineToGame event
  CliffEvent payload = message.GetData().Get_cliffEvent();
  Broadcast(ExternalInterface::MessageEngineToGame(CliffEvent(payload)));
  
}
  
void Robot::HandleProxObstacle(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
#if(HANDLE_PROX_OBSTACLES)
  ProxObstacle proxObs = message.GetData().Get_proxObstacle();
  PRINT_NAMED_INFO("RobotImplMessaging.HandleProxObstacle.Detected", "at dist %d mm",
                   proxObs.distance_mm);
  
  // Compute location of obstacle
  // NOTE: This should actually depend on a historical pose, but this is all changing eventually anyway...
  f32 heading = GetPose().GetRotationAngle<'Z'>().ToFloat();
  Vec3f newPt(GetPose().GetTranslation());
  newPt.x() += proxObs.distance_mm * cosf(heading);
  newPt.y() += proxObs.distance_mm * sinf(heading);
  Pose3d obsPose(heading, Z_AXIS_3D(), newPt, GetWorldOrigin());
  
  // Add prox obstacle (hijack cliff function for now)
  _blockWorld.AddCliff(obsPose);
#endif
}
  
  
void Robot::HandleChargerEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ChargerEvent chargerEvent = message.GetData().Get_chargerEvent();
  if (chargerEvent.onCharger) {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleChargerEvent.OnCharger", "");
    
    // Stop whatever we were doing
    //GetActionList().Cancel();
    
  } else {
    PRINT_NAMED_INFO("RobotImplMessaging.HandleChargerEvent.OffCharger", "");
  }
  
  // Forward on with EngineToGame event
  ChargerEvent payload = message.GetData().Get_chargerEvent();
  Broadcast(ExternalInterface::MessageEngineToGame(ChargerEvent(payload)));
}
  

// For processing image chunks arriving from robot.
// Sends complete images to VizManager for visualization (and possible saving).
void Robot::HandleImageChunk(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // Ignore images if robot has not yet acknowledged time sync
  if (!_timeSynced)
    return;
  
  const ImageChunk& payload = message.GetData().Get_image();
  const u16 width  = Vision::CameraResInfo[(int)payload.resolution].width;
  const u16 height = Vision::CameraResInfo[(int)payload.resolution].height;

  const bool isImageReady = _imageDeChunker->
  AppendChunk(payload.imageId, payload.frameTimeStamp,
    height, width,
    payload.imageEncoding,
    payload.imageChunkCount,
    payload.chunkId, payload.data.data(), (uint32_t)payload.data.size() );

  if (_context->GetExternalInterface() != nullptr && GetImageSendMode() != ImageSendMode::Off) {
    ExternalInterface::MessageEngineToGame msgWrapper;
    msgWrapper.Set_ImageChunk(payload);
    _context->GetExternalInterface()->Broadcast(msgWrapper);

    const bool wasLastChunk = payload.chunkId == payload.imageChunkCount-1;

    if(wasLastChunk && GetImageSendMode() == ImageSendMode::SingleShot) {
      // We were just in single-image send mode, and the image got sent, so
      // go back to "off". (If in stream mode, stay in stream mode.)
      SetImageSendMode(ImageSendMode::Off);
    }
  }
  GetContext()->GetVizManager()->SendImageChunk(GetID(), payload);

  if(isImageReady)
  {
    cv::Mat cvImg = _imageDeChunker->GetImage();
    if(cvImg.channels() == 1) {
      cv::cvtColor(cvImg, cvImg, CV_GRAY2RGB);
    }

    Vision::ImageRGB image(height,width,cvImg.data);
    image.SetTimestamp(payload.frameTimeStamp);

#   if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1
    // Create a 50mb ramdisk on OSX at "/Volumes/RamDisk/" by typing: diskutil erasevolume HFS+ 'RamDisk' `hdiutil attach -nomount ram://100000`
    static const char * const g_queueImages_filenamePattern = "/Volumes/RamDisk/robotImage%04d.bmp";
    static const s32 g_queueImages_queueLength = 70; // Must be at least the FPS of the camera. But higher numbers may cause more lag for the consuming process.
    static s32 g_queueImages_queueIndex = 0;
    
    char filename[256];
    snprintf(filename, 256, g_queueImages_filenamePattern, g_queueImages_queueIndex);
    
    cv::imwrite(filename, image.get_CvMat_());
    
    g_queueImages_queueIndex++;
    
    if(g_queueImages_queueIndex >= g_queueImages_queueLength)
      g_queueImages_queueIndex = 0;
#   endif // #if defined(STREAM_IMAGES_VIA_FILESYSTEM) && STREAM_IMAGES_VIA_FILESYSTEM == 1

    ProcessImage(image);

  } // if(isImageReady)

}

// For processing imu data chunks arriving from robot.
// Writes the entire log of 3-axis accelerometer and 3-axis
// gyro readings to a .m file in kP_IMU_LOGS_DIR so they
// can be read in from Matlab. (See robot/util/imuLogsTool.m)
void Robot::HandleImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::IMUDataChunk& payload = message.GetData().Get_imuDataChunk();

  // If seqID has changed, then start a new log file
  if (payload.seqId != _imuSeqID) {
    _imuSeqID = payload.seqId;
    
    // Make sure imu capture folder exists
    std::string imuLogsDir = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.HandleImuData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuLog_" + std::to_string(_imuSeqID) + ".dat";
    PRINT_NAMED_INFO("Robot.HandleImuData.OpeningLogFile",
                     "%s", imuLogFileName.c_str());
    
    _imuLogFileStream.open(imuLogFileName.c_str());
    _imuLogFileStream << "aX aY aZ gX gY gZ\n";
  }
  
  
  for (u32 s = 0; s < (u32)IMUConstants::IMU_CHUNK_SIZE; ++s) {
    _imuLogFileStream << payload.aX.data()[s] << " "
                      << payload.aY.data()[s] << " "
                      << payload.aZ.data()[s] << " "
                      << payload.gX.data()[s] << " "
                      << payload.gY.data()[s] << " "
                      << payload.gZ.data()[s] << "\n";
  }
  
  // Close file when last chunk received
  if (payload.chunkId == payload.totalNumChunks - 1) {
    PRINT_NAMED_INFO("Robot.HandleImuData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}


void Robot::HandleImuRawData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const RobotInterface::IMURawDataChunk& payload = message.GetData().Get_imuRawDataChunk();
  
  if (payload.order == 0) {
    ++_imuSeqID;
    
    // Make sure imu capture folder exists
    std::string imuLogsDir = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMU_LOGS_DIR);
    if (!Util::FileUtils::CreateDirectory(imuLogsDir, false, true)) {
      PRINT_NAMED_ERROR("Robot.HandleImuRawData.CreateDirFailed","%s", imuLogsDir.c_str());
    }
    
    // Open imu log file
    std::string imuLogFileName = std::string(imuLogsDir.c_str()) + "/imuRawLog_" + std::to_string(_imuSeqID) + ".dat";
    PRINT_NAMED_INFO("Robot.HandleImuRawData.OpeningLogFile",
                     "%s", imuLogFileName.c_str());
    
    _imuLogFileStream.open(imuLogFileName.c_str());
    _imuLogFileStream << "timestamp aX aY aZ gX gY gZ\n";
  }
  
  _imuLogFileStream
    << payload.a.data()[0] << " "
    << payload.a.data()[1] << " "
    << payload.a.data()[2] << " "
    << payload.g.data()[0] << " "
    << payload.g.data()[1] << " "
    << payload.g.data()[2] << " "
    << static_cast<int>(payload.timestamp) << "\n";
  
  // Close file when last chunk received
  if (payload.order == 2) {
    PRINT_NAMED_INFO("Robot.HandleImuRawData.ClosingLogFile", "");
    _imuLogFileStream.close();
  }
}

  
void Robot::HandleSyncTimeAck(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _timeSynced = true;
}
  
void Robot::HandleRobotPoked(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // Forward on with EngineToGame event
  PRINT_NAMED_INFO("Robot.HandleRobotPoked","");
  RobotInterface::RobotPoked payload = message.GetData().Get_robotPoked();
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPoked(payload.robotID)));
}

  
  void Robot::HandleNVData(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    NVStorage::NVStorageBlob nvBlob = message.GetData().Get_nvData();

    switch((NVStorage::NVEntryTag)nvBlob.tag)
    {
      case NVStorage::NVEntryTag::NVEntry_CameraCalibration:
      {
        PRINT_NAMED_INFO("Robot.HandleNVData.CameraCalibration","");
        
        RobotInterface::CameraCalibration payload;
        payload.Unpack(nvBlob.blob.data(), nvBlob.blob.size());
        PRINT_NAMED_INFO("RobotMessageHandler.CameraCalibration",
                         "Received new %dx%d camera calibration from robot.", payload.ncols, payload.nrows);
        
        // Convert calibration message into a calibration object to pass to the robot
        Vision::CameraCalibration calib(payload.nrows,
                                        payload.ncols,
                                        payload.focalLength_x,
                                        payload.focalLength_y,
                                        payload.center_x,
                                        payload.center_y,
                                        payload.skew);
        
        _visionComponent.SetCameraCalibration(*this, calib);
        SetPhysicalRobot(payload.isPhysicalRobots);
        break;
      }
      case NVStorage::NVEntryTag::NVEntry_APConfig:
      case NVStorage::NVEntryTag::NVEntry_StaConfig:
      case NVStorage::NVEntryTag::NVEntry_SDKModeUnlocked:
      case NVStorage::NVEntryTag::NVEntry_Invalid:
        PRINT_NAMED_WARNING("Robot.HandleNVData.UnhandledTag", "TODO: Implement handler for tag %d", nvBlob.tag);
        break;
      default:
        PRINT_NAMED_WARNING("Robot.HandleNVData.InvalidTag", "Invalid tag %d", nvBlob.tag);
        break;
    }

  }

  void Robot::HandleNVOpResult(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    NVStorage::NVOpResult payload = message.GetData().Get_nvResult();
    PRINT_NAMED_INFO("Robot.HandleNVOpResult","Tag: %d, Result: %d, write: %d", payload.tag, (int)payload.result, payload.write);
  }

void Robot::SetupMiscHandlers(IExternalInterface& externalInterface)
{
  auto helper = MakeAnkiEventUtil(externalInterface, *this, _signalHandles);
  
  using namespace ExternalInterface;
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBehaviorSystemEnabled>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::CancelAction>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::DrawPoseMarker>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::IMURequest>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableRobotPickupParalysis>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetIdleAnimation>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ReplayLastAnimation>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ExecuteTestPlan>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveImages>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SaveRobotState>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetRobotCarryingObject>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortPath>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::AbortAll>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetActiveObjectLEDs>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetAllActiveObjectLEDs>();
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetBehaviorSystemEnabled& msg)
{
  _isBehaviorMgrEnabled = msg.enabled;
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::CancelAction& msg)
{
  GetActionList().Cancel((RobotActionType)msg.actionType);
}

template<>
void Robot::HandleMessage(const ExternalInterface::DrawPoseMarker& msg)
{
  if(IsCarryingObject()) {
    Pose3d targetPose(msg.rad, Z_AXIS_3D(), Vec3f(msg.x_mm, msg.y_mm, 0));
    Quad2f objectFootprint = GetBlockWorld().GetObjectByID(GetCarryingObject())->GetBoundingQuadXY(targetPose);
    GetContext()->GetVizManager()->DrawPoseMarker(0, objectFootprint, ::Anki::NamedColors::GREEN);
  }
}

template<>
void Robot::HandleMessage(const ExternalInterface::IMURequest& msg)
{
  RequestIMU(msg.length_ms);
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::EnableRobotPickupParalysis& msg)
{
  SendEnablePickupParalysis(msg.enable);
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  SetBackpackLights(msg.onColor, msg.offColor,
                    msg.onPeriod_ms, msg.offPeriod_ms,
                    msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms);
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetIdleAnimation& msg)
{
  _animationStreamer.SetIdleAnimation(msg.animationName);
}

template<>
void Robot::HandleMessage(const ExternalInterface::ReplayLastAnimation& msg)
{
  _animationStreamer.SetStreamingAnimation(*this, _lastPlayedAnimationId, msg.numLoops);
}

template<>
void Robot::HandleMessage(const ExternalInterface::ExecuteTestPlan& msg)
{
  Planning::Path p;

  PathMotionProfile motionProfile(msg.motionProf);

  _longPathPlanner->GetTestPath(GetPose(), p, &motionProfile);
  ExecutePath(p);
}

template<>
void Robot::HandleMessage(const ExternalInterface::SaveImages& msg)
{
  _imageSaveMode = (SaveMode_t)msg.mode;
}

template<>
void Robot::HandleMessage(const ExternalInterface::SaveRobotState& msg)
{
  _stateSaveMode = (SaveMode_t)msg.mode;
}
  
template<>
void Robot::HandleMessage(const ExternalInterface::SetRobotCarryingObject& msg)
{
  if(msg.objectID < 0) {
    UnSetCarryingObjects();
  } else {
    SetCarryingObject(msg.objectID);
  }
}

template<>
void Robot::HandleMessage(const ExternalInterface::AbortPath& msg)
{
  AbortDrivingToPose();
}

template<>
void Robot::HandleMessage(const ExternalInterface::AbortAll& msg)
{
  AbortAll();
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetActiveObjectLEDs& msg)
{
  assert(msg.objectID <= s32_MAX);
  SetObjectLights(msg.objectID,
                  msg.whichLEDs,
                  msg.onColor, msg.offColor,
                  msg.onPeriod_ms, msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                  msg.turnOffUnspecifiedLEDs,
                  msg.makeRelative,
                  Point2f(msg.relativeToX, msg.relativeToY));
}

template<>
void Robot::HandleMessage(const ExternalInterface::SetAllActiveObjectLEDs& msg)
{
  assert(msg.objectID <= s32_MAX);
  SetObjectLights(msg.objectID,
                  msg.onColor, msg.offColor,
                  msg.onPeriod_ms, msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms, msg.transitionOffPeriod_ms,
                  msg.makeRelative, Point2f(msg.relativeToX, msg.relativeToY));
}
  
void Robot::SetupVisionHandlers(IExternalInterface& externalInterface)
{
  // TODO: Just move these handlers to VisionComponent and have it handle them directly
  
  // StartFaceTracking
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::EnableVisionMode,
    [this] (const GameToEngineEvent& event)
    {
      auto const& payload = event.GetData().Get_EnableVisionMode();
      _visionComponent.EnableMode(payload.mode, payload.enable);
    }));
  
  // VisionWhileMoving
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::VisionWhileMoving,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::VisionWhileMoving& msg = event.GetData().Get_VisionWhileMoving();
      _visionComponent.EnableVisionWhileMoving(msg.enable);
    }));
}

void Robot::SetupGainsHandlers(IExternalInterface& externalInterface)
{
  // SetControllerGains
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::ControllerGains,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::ControllerGains& msg = event.GetData().Get_ControllerGains();
      
      SendRobotMessage<RobotInterface::ControllerGains>(msg.kp, msg.ki, msg.kd, msg.maxIntegralError, msg.controller);
    }));

  // SetMotionModelParams
  _signalHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetMotionModelParams,
    [this] (const GameToEngineEvent& event)
    {
      const ExternalInterface::SetMotionModelParams& msg = event.GetData().Get_SetMotionModelParams();
                                                         
      SendRobotMessage<RobotInterface::SetMotionModelParams>(msg.slipFactor);
    }));
  
}
  
} // end namespace Cozmo
} // end namespace Anki
