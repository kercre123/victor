#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Cozmo {
  
  FaceWorld::KnownFace::KnownFace(Vision::TrackedFace& faceIn)
  : face(faceIn)
  , vizHandle(VizManager::INVALID_HANDLE)
  {
  
  }

  
  FaceWorld::FaceWorld(Robot& robot)
  : _robot(robot)
  , _deletionTimeout_ms(3000)
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
  
  Result FaceWorld::AddOrUpdateFace(Vision::TrackedFace& face)
  {
    // The incoming TrackedFace is w.r.t. the arbitrary historical camera
    // that observed it. Make w.r.t. robot world origin now:
    if(face.GetHeadPose().GetParent() == nullptr || face.GetHeadPose().GetParent()->IsOrigin())
    {
      PRINT_NAMED_ERROR("FaceWorld.AddOrUpdateFace.BadPoseParent",
                        "TrackedFace's head pose parent should not be null or an origin.");
      return RESULT_FAIL;
    }
    Pose3d headPose = face.GetHeadPose().GetWithRespectToOrigin(); // w.r.t. *historical* origin!
    headPose.SetParent(_robot.GetWorldOrigin()); // transfer to robot world origin

    face.SetHeadPose(headPose);

    auto insertResult = _knownFaces.insert({face.GetID(), face});
    
    if(insertResult.second) {
      PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace", "Added new face with ID=%lld.", face.GetID());
    } else {
      // Update the existing face:
      insertResult.first->second = face;
    }

    // Draw 3D face
    static const Point3f humanHeadSize{148.f, 225.f, 195.f};
    KnownFace& knownFace = insertResult.first->second;
    knownFace.vizHandle = VizManager::getInstance()->DrawHumanHead(static_cast<u32>(knownFace.face.GetID()),
                                                                   humanHeadSize,
                                                                   knownFace.face.GetHeadPose(),
                                                                   NamedColors::GREEN);
    
    if((_robot.GetTrackToFace() != Vision::TrackedFace::UnknownFace) &&
       (_robot.GetTrackToFace() == face.GetID()))
    {
      // TODO: Re-enable this
      //UpdateFaceTracking(face);
    }
    
    // Send out an event about this face being observed
    if(_robot.HasExternalInterface())
    {
      using namespace ExternalInterface;
      const Vec3f& trans = face.GetHeadPose().GetTranslation();
      const UnitQuaternion<f32>& q = face.GetHeadPose().GetRotation().GetQuaternion();
      _robot.GetExternalInterface()->Broadcast(MessageEngineToGame(RobotObservedFace(face.GetID(),
                                                                                     _robot.GetID(),
                                                                                     trans.x(),
                                                                                     trans.y(),
                                                                                     trans.z(),
                                                                                     q.w(),
                                                                                     q.x(),
                                                                                     q.y(),
                                                                                     q.z())));
    }

    return RESULT_OK;
  }
  
  Result FaceWorld::Update()
  {
    // Delete any faces we haven't seen in awhile
    for(auto faceIter = _knownFaces.begin(); faceIter != _knownFaces.end(); )
    {
      if(_robot.GetLastImageTimeStamp() - faceIter->second.face.GetTimeStamp() > _deletionTimeout_ms) {
        
        PRINT_NAMED_INFO("FaceWorld.Update.DeletingFace",
                         "Removing face %llu at t=%d, because it hasn't been seen since t=%d.",
                         faceIter->first, _robot.GetLastImageTimeStamp(),
                         faceIter->second.face.GetTimeStamp());
        
        VizManager::getInstance()->EraseVizObject(faceIter->second.vizHandle);
        faceIter = _knownFaces.erase(faceIter);
      } else {
        ++faceIter;
      }
    }
    
    return RESULT_OK;
  } // Update()
    
  const Vision::TrackedFace* FaceWorld::GetFace(Vision::TrackedFace::ID_t faceID) const
  {
    auto faceIter = _knownFaces.find(faceID);
    if(faceIter == _knownFaces.end()) {
      return nullptr;
    } else {
      return &faceIter->second.face;
    }
  }

  
} // namespace Cozmo
} // namespace Anki

