#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/common/basestation/math/point_impl.h"
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
  
    // NOTE: This isn't perfectly accurate since it doesn't take into account the
    // the head angle and is simply using the neck joint (which should also
    // probably be queried from the robot instead of using the constant here)
    const f32 zDist = headPose.GetTranslation().z() - (robotTrans.z() + NECK_JOINT_POSITION[2]);
    
    Radians headAngle = std::atan(zDist/(minDist + 1e-6f));
    
    static const Radians minHeadAngle(DEG_TO_RAD(3.f));
    static const Radians minBodyAngle(DEG_TO_RAD(7.5f));
    
    RobotInterface::PanAndTilt msg;
    if((headAngle - _robot.GetHeadAngle()).getAbsoluteVal() > minHeadAngle) {
      msg.headTiltAngle_rad = headAngle.ToFloat();
    } else {
      msg.headTiltAngle_rad = _robot.GetHeadAngle();
    }
    
    const Radians currentBodyAngle = _robot.GetPose().GetRotationAngle<'Z'>();
    msg.bodyPanAngle_rad = currentBodyAngle.ToFloat();
    if(false == _robot.IsTrackingWithHeadOnly()) {
      // Also rotate ("pan") body, if the angle is large enough:
      const Radians panAngle = std::atan2(yDist, xDist);
      if((panAngle - currentBodyAngle).getAbsoluteVal() > minBodyAngle) {
        msg.bodyPanAngle_rad = panAngle.ToFloat();
      }
    }
    
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
    
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

    static const Point3f humanHeadSize{148.f, 225.f, 195.f};
    static const bool usePoseToMatchIDs = true; // if true, ignores IDs from face tracker
    
    KnownFace* knownFace = nullptr;
    
    if(usePoseToMatchIDs) {
      
      // Look through known faces and compare pose:
      bool foundMatch = false;
      for(auto knownFaceIter = _knownFaces.begin(); knownFaceIter != _knownFaces.end(); ++knownFaceIter)
      {
        // Note we're using really loose thresholds for checking pose sameness
        // since our ability to accurately localize face's 3D pose is limited.
        if(knownFaceIter->second.face.GetHeadPose().IsSameAs(face.GetHeadPose(),
                                                             humanHeadSize,
                                                             DEG_TO_RAD(90)))
        {
          knownFace = &knownFaceIter->second;
          
          const Vision::TrackedFace::ID_t matchedID = knownFace->face.GetID();
          //PRINT_NAMED_INFO("FaceWorld.UpdateFace.ExistingFace",
          //                 "Updating existing face with ID=%lld.", matchedID);
          knownFace->face = face;
          knownFace->face.SetID(matchedID);
          foundMatch = true;
          break;
        }
      }
      
      // Didn't find a match based on pose, so add a new face with a new ID:
      if(!foundMatch) {
        PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace", "Added new face with ID=%lld.", _idCtr);
        face.SetID(_idCtr); // Use our own ID here for the new face
        auto insertResult = _knownFaces.insert({_idCtr, face});
        if(insertResult.second == false) {
          PRINT_NAMED_ERROR("FaceWorld.UpdateFace.ExistingID",
                            "Did not find a match by pose, but ID %lld already in use.",
                            _idCtr);
          return RESULT_FAIL;
        }
        knownFace = &insertResult.first->second;
        ++_idCtr;
      }
      
    } else { // Use ID coming from face tracker / recognizer
      auto insertResult = _knownFaces.insert({face.GetID(), face});
      
      if(insertResult.second) {
        PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace", "Added new face with ID=%lld.", face.GetID());
      } else {
        // Update the existing face:
        insertResult.first->second = face;
      }
      
      knownFace = &insertResult.first->second;
    }
    
    // By now, we should have either created a new face or be pointing at an
    // existing one!
    assert(knownFace != nullptr);
    
    // Draw 3D face
    knownFace->vizHandle = VizManager::getInstance()->DrawHumanHead(1+static_cast<u32>(knownFace->face.GetID()),
                                                                   humanHeadSize,
                                                                   knownFace->face.GetHeadPose(),
                                                                   ::Anki::NamedColors::GREEN);
    
    if((_robot.GetTrackToFace() != Vision::TrackedFace::UnknownFace) &&
       (_robot.GetTrackToFace() == knownFace->face.GetID()))
    {
      UpdateFaceTracking(knownFace->face);
    }
    
    // Send out an event about this face being observed
    if(_robot.HasExternalInterface())
    {
      using namespace ExternalInterface;
      const Vec3f& trans = knownFace->face.GetHeadPose().GetTranslation();
      const UnitQuaternion<f32>& q = knownFace->face.GetHeadPose().GetRotation().GetQuaternion();
      _robot.GetExternalInterface()->Broadcast(MessageEngineToGame(RobotObservedFace(knownFace->face.GetID(),
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
        
        if(_robot.GetExternalInterface()) {
          using namespace ExternalInterface;
          _robot.GetExternalInterface()->Broadcast(MessageEngineToGame(RobotDeletedFace(faceIter->second.face.GetID(), _robot.GetID())));
        }
        
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

