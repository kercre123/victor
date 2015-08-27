#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Cozmo {
  
  FaceWorld::FaceWorld(Robot& robot)
  : _robot(robot)
  {
    
  }
  
  Result FaceWorld::UpdateFaceTracking(const Vision::TrackedFace& face)
  {
    const Vec3f& robotTrans = _robot.GetPose().GetTranslation();
    
    Pose3d headPose;
    if(false == face.GetHeadPose().GetWithRespectTo(*_robot.GetWorldOrigin(), headPose)) {
      PRINT_NAMED_ERROR("BlockWorld.UpdateTrackToObject",
                        "Could not get pose of observed marker w.r.t. world for head tracking.\n");
      return RESULT_FAIL;
    }
    
    const f32 xDist = headPose.GetTranslation().x() - robotTrans.x();
    const f32 yDist = headPose.GetTranslation().y() - robotTrans.y();
    
    const f32 minDist = std::sqrt(xDist*xDist + yDist*yDist);
    
    // Keep track of best zDist too, so we don't have to redo the GetWithRespectTo call outside this loop
    // NOTE: This isn't perfectly accurate since it doesn't take into account the
    // the head angle and is simply using the neck joint (which should also
    // probably be queried from the robot instead of using the constant here)
    const f32 zDist = headPose.GetTranslation().z() - (robotTrans.z() + NECK_JOINT_POSITION[2]);
    
    const f32 headAngle = std::atan(zDist/(minDist + 1e-6f));
    
    MessagePanAndTiltHead msg;
    msg.headTiltAngle_rad = headAngle;
    msg.bodyPanAngle_rad = 0.f;
    
    if(false == _robot.IsTrackingWithHeadOnly()) {
      // Also rotate ("pan") body:
      const Radians panAngle = std::atan2(yDist, xDist);// - _robot->GetPose().GetRotationAngle<'Z'>();
      msg.bodyPanAngle_rad = panAngle.ToFloat();
    }
    
    _robot.SendMessage(msg);
    
    return RESULT_OK;
  } // UpdateFaceTracking()
  
  Result FaceWorld::UpdateFace(Vision::TrackedFace& face)
  {
    auto insertResult = _knownFaces.insert({face.GetID(), face});
    
    if(insertResult.second) {
      PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace", "Added new face with ID=%lld.", face.GetID());
    } else {
      // Update the existing face:
      insertResult.first->second = face;
    }
    
    if((_robot.GetTrackToFace() != Vision::TrackedFace::UnknownFace) &&
       (_robot.GetTrackToFace() == face.GetID()))
    {
      // TODO: Re-enable this
      //UpdateFaceTracking(face);
    }
    
    // Send out an event about this face
    _robot.GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotObservedFace(face.GetID(), _robot.GetID())));

    return RESULT_OK;
  }
    
  const Vision::TrackedFace* FaceWorld::GetFace(Vision::TrackedFace::ID_t faceID) const
  {
    auto faceIter = _knownFaces.find(faceID);
    if(faceIter == _knownFaces.end()) {
      return nullptr;
    } else {
      return &faceIter->second;
    }
  }

  
} // namespace Cozmo
} // namespace Anki

