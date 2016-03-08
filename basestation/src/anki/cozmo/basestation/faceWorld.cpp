#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
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
    if(robot.HasExternalInterface()) {
      SetupEventHandlers(*robot.GetExternalInterface());
    }
  }
  
  void FaceWorld::SetupEventHandlers(IExternalInterface& externalInterface)
  {
    using EventType = AnkiEvent<ExternalInterface::MessageGameToEngine>;
    
    // ClearAllObjects
    _eventHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::ClearAllObjects,
                                                        [this] (const EventType& event)
                                                        {
                                                          ClearAllFaces();
                                                        }));
    
    // SetOwnerFace
    _eventHandles.push_back(externalInterface.Subscribe(ExternalInterface::MessageGameToEngineTag::SetOwnerFace,
                                                        [this](const EventType& event)
                                                        {
                                                          const s32 ownerID = event.GetData().Get_SetOwnerFace().ownerID;
                                                          if(ownerID < 0) {
                                                            SetOwnerID(Vision::TrackedFace::UnknownFace);
                                                          } else {
                                                            SetOwnerID(ownerID);
                                                          }
                                                        }));
  }
  
  void FaceWorld::RemoveFace(KnownFaceIter& knownFaceIter, bool broadcast)
  {
    if(broadcast) {
      using namespace ExternalInterface;
      _robot.Broadcast(MessageEngineToGame(RobotDeletedFace(knownFaceIter->first, _robot.GetID())));
    }
    _robot.GetContext()->GetVizManager()->EraseVizObject(knownFaceIter->second.vizHandle);
    knownFaceIter = _knownFaces.erase(knownFaceIter);
  }
  
  void FaceWorld::ClearAllFaces()
  {
    for(auto knownFaceIter = _knownFaces.begin(); knownFaceIter != _knownFaces.end(); /* inc in loop */)
    {
      // NOTE: RemoveFace increments the iterator for us
      RemoveFace(knownFaceIter);
    }

    _lastObservedFaceTimeStamp = 0;
  }
  
  void FaceWorld::RemoveFaceByID(Vision::TrackedFace::ID_t faceID)
  {
    auto knownFaceIter = _knownFaces.find(faceID);
    
    if(knownFaceIter != _knownFaces.end())
    {
      PRINT_NAMED_INFO("FaceWorld.RemoveFaceByID",
                       "Removing face %d", faceID);
      
      RemoveFace(knownFaceIter);
    }
  }
  
  Result FaceWorld::ChangeFaceID(Vision::TrackedFace::ID_t oldID,
                                 Vision::TrackedFace::ID_t newID)
  {
    auto knownFaceIter = _knownFaces.find(oldID);
    if(knownFaceIter != _knownFaces.end()) {
      // TODO: Is there a more efficient move operation I could do here?
      knownFaceIter->second.face.SetID(newID);
      _knownFaces.insert({newID, knownFaceIter->second.face});
      RemoveFace(knownFaceIter, false); // NOTE: don't broadcast the deletion
      
      ExternalInterface::RobotChangedObservedFaceID msg;
      msg.oldID = oldID;
      msg.newID = newID;
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
      
      PRINT_NAMED_INFO("FaceWorld.ChangeFaceID.Success",
                       "Updating old face %d to new ID %d",
                       msg.oldID, msg.newID);
      
      return RESULT_OK;
    } else if(oldID > 0){
      PRINT_NAMED_WARNING("FaceWorld.ChangeFaceID.BadOldID",
                          "ID %d does not exist, cannot update to %d",
                          oldID, newID);
      return RESULT_FAIL;
    } else {
      // Probably no match for old ID because it was a tracked ID and wasn't
      // even added to face world before being recognized and being assigned this
      // new recognized ID
      return RESULT_OK;
    }
  }
  
  Result FaceWorld::AddOrUpdateFace(Vision::TrackedFace& face)
  {
    // The incoming TrackedFace should be w.r.t. the arbitrary historical world origin.
    if(face.GetHeadPose().GetParent() == nullptr || !face.GetHeadPose().GetParent()->IsOrigin())
    {
      PRINT_NAMED_ERROR("FaceWorld.AddOrUpdateFace.BadPoseParent",
                        "TrackedFace's head pose parent should be an origin.");
      return RESULT_FAIL;
    }

    // Head pose is stored w.r.t. historical world origin, but needs its parent
    // set up to be the robot's world origin here:
    Pose3d headPose = face.GetHeadPose();
    headPose.SetParent(_robot.GetWorldOrigin());
    face.SetHeadPose(headPose);

    /*
    PRINT_NAMED_INFO("FaceWorld.AddOrUpdateFace",
                     "Updating with face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
                     "at t=%d Pose: roll=%.1f, pitch=%.1f yaw=%.1f, T=(%.1f,%.1f,%.1f).",
                     face.GetRect().GetX(), face.GetRect().GetY(),
                     face.GetRect().GetWidth(), face.GetRect().GetHeight(),
                     face.GetTimeStamp(),
                     face.GetHeadRoll().getDegrees(),
                     face.GetHeadPitch().getDegrees(),
                     face.GetHeadYaw().getDegrees(),
                     face.GetHeadPose().GetTranslation().x(),
                     face.GetHeadPose().GetTranslation().y(),
                     face.GetHeadPose().GetTranslation().z());
    */
    
    static const Point3f humanHeadSize{148.f, 225.f, 195.f};
    
    KnownFace* knownFace = nullptr;

    if(false == Vision::FaceTracker::IsRecognitionSupported())
    {
      // Can't get an ID from face recognition, so use pose instead
      bool foundMatch = false;
      
      // Look through known faces and compare pose and image rectangles:
      f32 IOU_threshold = 0.5f;
      for(auto knownFaceIter = _knownFaces.begin(); knownFaceIter != _knownFaces.end(); ++knownFaceIter)
      {
        
        // Note we're using really loose thresholds for checking pose sameness
        // since our ability to accurately localize face's 3D pose is limited.
        Vec3f Tdiff;
        Radians angleDiff;
        
        const auto & knownRect = knownFaceIter->second.face.GetRect();
        const f32 currentIOU = face.GetRect().ComputeOverlapScore(knownRect);
        if(currentIOU > IOU_threshold)
        {
          if(foundMatch) {
            // If we had already found a match, delete the last one, because this
            // new face matches multiple existing faces
            assert(nullptr != knownFace);
            RemoveFaceByID(knownFace->face.GetID());
          }
          
          IOU_threshold = currentIOU;
          foundMatch = true;
          knownFace = &knownFaceIter->second;
        }
        else
        {
          auto posDiffVec = knownFaceIter->second.face.GetHeadPose().GetTranslation() - face.GetHeadPose().GetTranslation();
          float posDiffSq = posDiffVec.LengthSq();
          if(posDiffSq <= headCenterPointThreshold * headCenterPointThreshold) {
            if(foundMatch) {
              // If we had already found a match, delete the last one, because this
              // new face matches multiple existing faces
              assert(nullptr != knownFace);
              RemoveFaceByID(knownFace->face.GetID());
            }
            
            knownFace = &knownFaceIter->second;
            foundMatch = true;
          }
        }
      } // for each known face
      
      if(foundMatch) {
        const Vision::TrackedFace::ID_t matchedID = knownFace->face.GetID();
        
        // Verbose! Useful for debugging
        //PRINT_NAMED_DEBUG("FaceWorld.UpdateFace.UpdatingKnownFaceByPose",
        //                  "Updating face with ID=%lld from t=%d to %d, observed %d times",
        //                  matchedID, knownFace->face.GetTimeStamp(), face.GetTimeStamp(),
        //                  face.GetNumTimesObserved());
        
        knownFace->face = face;
        knownFace->face.SetID(matchedID);
      }
      
      // Didn't find a match based on pose, so add a new face with a new ID:
      else {
        PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace",
                         "Added new face with ID=%d at t=%d.", _idCtr, face.GetTimeStamp());
        face.SetID(_idCtr); // Use our own ID here for the new face
        auto insertResult = _knownFaces.insert({_idCtr, face});
        if(insertResult.second == false) {
          PRINT_NAMED_ERROR("FaceWorld.UpdateFace.ExistingID",
                            "Did not find a match by pose, but ID %d already in use.",
                            _idCtr);
          return RESULT_FAIL;
        }
        knownFace = &insertResult.first->second;
        
        ++_idCtr;
      }
      
    } else {
      // Use face recognition to get ID
      auto insertResult = _knownFaces.insert({face.GetID(), face});
      knownFace = &insertResult.first->second;
      
      if(insertResult.second) {
        PRINT_NAMED_INFO("FaceWorld.UpdateFace.NewFace",
                         "Added new face with ID=%d at t=%d.",
                         face.GetID(), face.GetTimeStamp());
      } else {
        // Update the existing face:
        if(!face.HasEyes()) {
          // If no eyes were detected, the translation we have at this point was
          // computed using "fake" eye locations, so just use the last translation
          // estimate since we matched this to an existing face:
          Pose3d headPose = face.GetHeadPose();
          headPose.SetTranslation(insertResult.first->second.face.GetHeadPose().GetTranslation());
          face.SetHeadPose(headPose);
        }
        insertResult.first->second.face = face;
      }
    } // if(false == Vision::FaceTracker::IsRecognitionSupported()
    
    // By now, we should have either created a new face or be pointing at an
    // existing one!
    assert(knownFace != nullptr);
    
    knownFace->numTimesObserved++;
    
    if(knownFace->numTimesObserved >= MinTimesToSeeFace)
    {
#     if 0 // !USE_POSE_FOR_ID
      // Remove any known faces whose poses overlap with this observed face
      for(auto knownFaceIter = _knownFaces.begin(); knownFaceIter != _knownFaces.end(); /* in loop */)
      {
        if(knownFaceIter != insertResult.first)
        {
          // Note we're using really loose thresholds for checking pose sameness
          // since our ability to accurately localize face's 3D pose is limited.
          if(knownFaceIter->second.face.GetHeadPose().IsSameAs(face.GetHeadPose(), 100.f, DEG_TO_RAD(45)))
          {
            PRINT_NAMED_INFO("FaceWorld.UpdateFace.RemovingOverlappingFace",
                             "Removing old face with ID=%lld because it overlaps new face %lld",
                             knownFaceIter->second.face.GetID(), face.GetID());
            RemoveFace(knownFaceIter);
          } else {
            ++knownFaceIter;
          }
        } else {
          ++knownFaceIter;
        }
      } // for each known face
#     endif // if !USE_POSE_FOR_ID
      
      // Update the last observed face pose.
      // If more than one was observed in the same timestamp then take the closest one.
      if (((_lastObservedFaceTimeStamp != knownFace->face.GetTimeStamp()) ||
          (ComputeDistanceBetween(_robot.GetPose(), _lastObservedFacePose) >
           ComputeDistanceBetween(_robot.GetPose(), knownFace->face.GetHeadPose())))) 
      {
        _lastObservedFacePose = knownFace->face.GetHeadPose();
        _lastObservedFaceTimeStamp = knownFace->face.GetTimeStamp();

      // Draw 3D face
      knownFace->vizHandle = _robot.GetContext()->GetVizManager()->DrawHumanHead(0,
                                                                      humanHeadSize,
                                                                      knownFace->face.GetHeadPose(),
                                                                      ::Anki::NamedColors::DARKGRAY);
      }

      // Draw 3D face
      ColorRGBA drawFaceColor;
      if(knownFace->face.GetID() == _ownerID)
      {
        // Draw owner in orange
        drawFaceColor = NamedColors::ORANGE;
      } else {
        // Rotate through other colors for other faces
        drawFaceColor = ColorRGBA::CreateFromColorIndex((u32)knownFace->face.GetID());
      }
      
      /*
      PRINT_NAMED_INFO("FaceWorld.AddOrUpdateFace",
                       "Known face at (x,y,w,h)=(%.1f,%.1f,%.1f,%.1f), "
                       "at t=%d Pose: roll=%.1f, pitch=%.1f yaw=%.1f, T=(%.1f,%.1f,%.1f).",
                       knownFace->face.GetRect().GetX(), knownFace->face.GetRect().GetY(),
                       knownFace->face.GetRect().GetWidth(), knownFace->face.GetRect().GetHeight(),
                       knownFace->face.GetTimeStamp(),
                       knownFace->face.GetHeadRoll().getDegrees(),
                       knownFace->face.GetHeadPitch().getDegrees(),
                       knownFace->face.GetHeadYaw().getDegrees(),
                       knownFace->face.GetHeadPose().GetTranslation().x(),
                       knownFace->face.GetHeadPose().GetTranslation().y(),
                       knownFace->face.GetHeadPose().GetTranslation().z());
      */
      
      const s32 vizID = (s32)knownFace->face.GetID() + (knownFace->face.GetID() >= 0 ? 1 : 0);
      knownFace->vizHandle = _robot.GetContext()->GetVizManager()->DrawHumanHead(vizID,
                                                                                 humanHeadSize,
                                                                                 knownFace->face.GetHeadPose(),
                                                                                 drawFaceColor);

      // Draw box around recognized face (with ID) now that we have the real ID set
      _robot.GetContext()->GetVizManager()->DrawCameraFace(knownFace->face, drawFaceColor);

      
      // Send out an event about this face being observed
      using namespace ExternalInterface;
      const Vec3f& trans = knownFace->face.GetHeadPose().GetTranslation();
      const UnitQuaternion<f32>& q = knownFace->face.GetHeadPose().GetRotation().GetQuaternion();
      _robot.Broadcast(MessageEngineToGame(RobotObservedFace(knownFace->face.GetID(),
                                                             _robot.GetID(),
                                                             face.GetTimeStamp(),
                                                             trans.x(),
                                                             trans.y(),
                                                             trans.z(),
                                                             q.w(),
                                                             q.x(),
                                                             q.y(),
                                                             q.z(),
                                                             knownFace->face.GetName())));
      
      /*
      const Vision::Image& faceThumbnail = knownFace->face.GetThumbnail();
      if(!faceThumbnail.IsEmpty()) {
        // TODO: Expose quality parameter
        const std::vector<int> compressionParams = {
          CV_IMWRITE_JPEG_QUALITY, 75
        };
        
        ExternalInterface::FaceEnrollmentImage msg;
        msg.faceID = knownFace->face.GetID();
        cv::imencode(".jpg",  faceThumbnail.get_CvMat_(), msg.jpgImage, compressionParams);

        _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
      }
      */
    }
    
    return RESULT_OK;
  }
  
  Result FaceWorld::Update()
  {
    // Delete any faces we haven't seen in awhile
    for(auto faceIter = _knownFaces.begin(); faceIter != _knownFaces.end(); )
    {
      Vision::TrackedFace& face = faceIter->second.face;
      
      if(_robot.GetVisionComponent().GetLastProcessedImageTimeStamp() > _deletionTimeout_ms + face.GetTimeStamp()) {
        
        PRINT_NAMED_INFO("FaceWorld.Update.DeletingOldFace",
                         "Removing face %d at t=%d, because it hasn't been seen since t=%d.",
                         faceIter->first, _robot.GetLastImageTimeStamp(),
                         faceIter->second.face.GetTimeStamp());
        
        RemoveFace(faceIter); // Increments faceIter!
        
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

  std::vector<Vision::TrackedFace::ID_t> FaceWorld::GetKnownFaceIDs() const
  {
    std::vector<Vision::TrackedFace::ID_t> faceIDs;
    for (auto pair : _knownFaces) {
      faceIDs.push_back(pair.first);
    }
    return faceIDs;
  }
  
  std::list<Vision::TrackedFace::ID_t> FaceWorld::GetKnownFaceIDsObservedSince(TimeStamp_t seenSinceTime_ms) const
  {
    std::list<Vision::TrackedFace::ID_t> faceIDs;
    for (auto pair : _knownFaces) {
      if (pair.second.face.GetTimeStamp() >= seenSinceTime_ms) {
        faceIDs.push_back(pair.second.face.GetID());
      }
    }
    return faceIDs;
  }
  
  
  TimeStamp_t FaceWorld::GetLastObservedFace(Pose3d& p) const
  {
    if (_lastObservedFaceTimeStamp > 0) {
      p = _lastObservedFacePose;
    }
    
    return _lastObservedFaceTimeStamp;
  }
  
} // namespace Cozmo
} // namespace Anki

