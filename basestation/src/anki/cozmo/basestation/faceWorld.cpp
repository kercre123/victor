/**
 * File: faceWorld.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 2014
 *
 * Description: Implements a container for mirroring on the main thread, the known faces
 *              from the vision system (which generally runs on another thread).
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/enrolledFaceStorage.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
namespace Cozmo {
  
  // How long before deleting an unnamed, unobserved face.
  // NOTE: we never delete _named_ faces.
  CONSOLE_VAR(u32, kDeletionTimeout_ms, "Vision.FaceWorld", 15000);
  
  // The distance threshold inside of which to head positions are considered to be the same face
  CONSOLE_VAR(float, kHeadCenterPointThreshold_mm, "Vision.FaceWorld", 220.f);
  
  // We don't log session-only (unnamed) faces to DAS until we consider them "stable"
  CONSOLE_VAR(u32, kNumTimesToSeeFrontalToBeStable, "Vision.FaceWorld", 30);
  
  // Log recognition to DAS if we haven't seen a face for this long and then re-see it
  CONSOLE_VAR(u32, kTimeUnobservedBeforeReLoggingToDAS_ms, "Vision.FaceWorld", 10000);
  
  // Ignore faces detected below the robot (except when picked up), to help reduce false positives
  CONSOLE_VAR(bool, kIgnoreFacesBelowRobot, "Vision.FaceWorld", true);
  
  // Ignore new faces detected while rotating too fast
  CONSOLE_VAR(f32, kHeadTurnSpeedThreshFace_degs,  "WasRotatingTooFast.Face.Head_deg/s",    10.f);
  CONSOLE_VAR(f32, kBodyTurnSpeedThreshFace_degs,  "WasRotatingTooFast.Face.Body_deg/s",    30.f);
  CONSOLE_VAR(u8,  kNumImuDataToLookBackFace,      "WasRotatingTooFast.Face.NumToLookBack", 5);
  
  static const char * const kLoggingChannelName = "FaceRecognizer";
  static const char * const kIsNamedStringDAS = "1";
  static const char * const kIsSessionOnlyStringDAS = "0";
  
  static const Point3f kHumanHeadSize{148.f, 225.f, 195.f};
  
  FaceWorld::KnownFace::KnownFace(const Vision::TrackedFace& faceIn)
  : face(faceIn)
  , vizHandle(VizManager::INVALID_HANDLE)
  {
  
  }

  inline bool FaceWorld::KnownFace::HasStableID() const
  {
    // Only true for non-tracking faces which are named or have been seen enough
    // times from the front
    DEV_ASSERT(!IsNamed() || face.GetID() > 0, "FaceWorld.KnownFace.HasStableID.NamedFaceWithNonPositiveID");
    return face.GetID() > 0 && (IsNamed() || numTimesObservedFacingCamera >= kNumTimesToSeeFrontalToBeStable);
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
//    using namespace ExternalInterface;
//    auto helper = MakeAnkiEventUtil(externalInterface, *this, _eventHandles);
//    helper.SubscribeGameToEngine<MessageGameToEngineTag::ClearAllObjects>();
  }
  
  void FaceWorld::RemoveFace(KnownFaceIter& knownFaceIter, bool broadcast)
  {
    if(broadcast)
    {
      using namespace ExternalInterface;
      _robot.Broadcast(MessageEngineToGame(RobotDeletedFace(knownFaceIter->first, _robot.GetID())));
    }
    
    if(knownFaceIter->second.vizHandle != VizManager::INVALID_HANDLE)
    {
      _robot.GetContext()->GetVizManager()->EraseVizObject(knownFaceIter->second.vizHandle);
    }
    
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
  
  void FaceWorld::RemoveFaceByID(Vision::FaceID_t faceID)
  {
    auto knownFaceIter = _knownFaces.find(faceID);
    
    if(knownFaceIter != _knownFaces.end())
    {
      PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.RemoveFaceByID",
                    "Removing face %d", faceID);
      
      RemoveFace(knownFaceIter);
    }
  }
  
  Result FaceWorld::ChangeFaceID(const Vision::UpdatedFaceID& update)
  {
    const Vision::FaceID_t oldID   = update.oldID;
    const Vision::FaceID_t newID   = update.newID;
    const std::string&     newName = update.newName;
    
    auto knownFaceIter = _knownFaces.find(oldID);
    if(knownFaceIter != _knownFaces.end())
    {
      Vision::TrackedFace& face = knownFaceIter->second.face;
     
      PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.ChangeFaceID.Success",
                    "Updating old face %d (%s) to new ID %d (%s)",
                    oldID, face.HasName() ? Util::HidePersonallyIdentifiableInfo(face.GetName().c_str()) : "<NoName>",
                    newID, newName.empty() ? "<NoName>" : Util::HidePersonallyIdentifiableInfo(newName.c_str()));
      
      const bool existingFaceHasDifferentName = face.HasName() && (newName != face.GetName());
      if(existingFaceHasDifferentName)
      {
        PRINT_NAMED_WARNING("FaceWorld.ChangeFaceID.ChangingName",
                            "OldID:%d OldName:%s, NewID:%d NewName:%s",
                            oldID, Util::HidePersonallyIdentifiableInfo(face.GetName().c_str()),
                            newID, Util::HidePersonallyIdentifiableInfo(newName.c_str()));
      }
      
      face.SetID(newID);
      face.SetName(newName);
      
      // TODO: Is there a more efficient move operation I could do here?
      auto result = _knownFaces.insert({newID, face});
      RemoveFace(knownFaceIter, false); // NOTE: don't broadcast the deletion
      
      // Re-draw the face and update the viz handle
      DrawFace(result.first->second);

      // Log ID changes to DAS when they are not tracking IDs and the new face is
      // either named or a "stable" session-only face.
      // Store old ID in DDATA and new ID in s_val.
      const KnownFace& newKnownFace = result.first->second;
      if(oldID > 0 && newID > 0 && newKnownFace.HasStableID())
      {
        Util::sEventF("robot.vision.update_face_id",
                      {{DDATA, std::to_string(oldID).c_str()}},
                      "%d", newID);
      }
      
    } else if(oldID > 0){
      PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.ChangeFaceID.UnknownOldID",
                    "ID %d does not exist, cannot update to %d",
                    oldID, newID);
    } else {
      // Probably no match for old ID because it was a tracked ID and wasn't
      // even added to face world before being recognized and being assigned this
      // new recognized ID
    }
    
    // Always notify game: let it decide whether or not it cares or knows about oldID
    using namespace ExternalInterface;
    _robot.Broadcast(MessageEngineToGame(RobotChangedObservedFaceID(oldID, newID)));

    return RESULT_OK;
  }
  
  Result FaceWorld::AddOrUpdateFace(const Vision::TrackedFace& face)
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
    Pose3d headPoseWrtWorldOrigin;
    const bool success = face.GetHeadPose().GetWithRespectTo(*_robot.GetWorldOrigin(), headPoseWrtWorldOrigin);
    if(!success)
    {
      PRINT_NAMED_INFO("FaceWorld.AddOrUpdateFace.MismatchedOrigins",
                       "Receveid observation of face %d from different origin (%s) than robot (%s). Ignoring",
                       face.GetID(), face.GetHeadPose().FindOrigin().GetName().c_str(),
                       _robot.GetWorldOrigin()->GetName().c_str());
      
      return RESULT_FAIL_ORIGIN_MISMATCH;
    }
    
    const bool robotOnTreads = _robot.GetOffTreadsState() == OffTreadsState::OnTreads;
    const bool headBelowRobot = headPoseWrtWorldOrigin.GetTranslation().z() < 0.f;
    if(kIgnoreFacesBelowRobot && robotOnTreads && headBelowRobot)
    {
      // Don't report faces that are below the origin (which we are assuming is on the ground plane)
      //PRINT_NAMED_DEBUG("VisionSystem.DetectFaces.IgnoreFaceBelowRobot",
      //                  "z=%.2f", headPose.GetTranslation().z());
      PRINT_CH_DEBUG(kLoggingChannelName, "FaceWorld.AddOrUpdateFace.IgnoringFaceBelowRobot",
                     "z=%f", headPoseWrtWorldOrigin.GetTranslation().z());
      return RESULT_OK;
    }
    
    /*
    PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.AddOrUpdateFace",
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
    
    KnownFace* knownFace = nullptr;
    TimeStamp_t timeSinceLastSeen_ms = 0;
    
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
          auto posDiffVec = (knownFaceIter->second.face.GetHeadPose().GetTranslation() -
                             headPoseWrtWorldOrigin.GetTranslation());
          float posDiffSq = posDiffVec.LengthSq();
          if(posDiffSq <= kHeadCenterPointThreshold_mm * kHeadCenterPointThreshold_mm) {
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
        const Vision::FaceID_t matchedID = knownFace->face.GetID();
        
        // Verbose! Useful for debugging
        //PRINT_CH_DEBUG(kLoggingChannelName, "FaceWorld.UpdateFace.UpdatingKnownFaceByPose",
        //               "Updating face with ID=%lld from t=%d to %d, observed %d times",
        //               matchedID, knownFace->face.GetTimeStamp(), face.GetTimeStamp(),
        //               face.GetNumTimesObserved());
        
        knownFace->face = face;
        knownFace->face.SetID(matchedID);
      }
      
      // Didn't find a match based on pose, so add a new face with a new ID:
      else {
        PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.UpdateFace.NewFace",
                      "Added new face with ID=%d at t=%d.", _idCtr, face.GetTimeStamp());
        
        auto insertResult = _knownFaces.insert({_idCtr, face});
        if(insertResult.second == false) {
          PRINT_NAMED_ERROR("FaceWorld.UpdateFace.ExistingID",
                            "Did not find a match by pose, but ID %d already in use.",
                            _idCtr);
          return RESULT_FAIL;
        }
        knownFace = &insertResult.first->second;
        knownFace->face.SetID(_idCtr); // Use our own ID here for the new face
        
        ++_idCtr;
      }
      
    }
    else
    {
      // Use face recognition to get ID
      auto existingIter = _knownFaces.find(face.GetID());
      
      const bool isNewFace = (existingIter == _knownFaces.end());
      if(isNewFace)
      {
        // Make sure we aren't rotating too fast to add a new face (this helps safeguard against false positives)
        const bool rotatingTooFastCheckEnabled = (Util::IsFltGT(kBodyTurnSpeedThreshFace_degs, 0.f) ||
                                                  Util::IsFltGT(kHeadTurnSpeedThreshFace_degs, 0.f));
        const bool wasRotatingTooFast = (rotatingTooFastCheckEnabled &&
                                         _robot.GetVisionComponent().WasRotatingTooFast(face.GetTimeStamp(),
                                                                                        DEG_TO_RAD(kBodyTurnSpeedThreshFace_degs),
                                                                                        DEG_TO_RAD(kHeadTurnSpeedThreshFace_degs),
                                                                                        (face.IsBeingTracked() ? kNumImuDataToLookBackFace : 0)));
        
        if(wasRotatingTooFast)
        {
          return RESULT_OK;
        }
        else
        {
          PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.UpdateFace.NewFace",
                        "Added new face with ID=%d at t=%d.",
                        face.GetID(), face.GetTimeStamp());
          
          auto result = _knownFaces.emplace(face.GetID(), face);
          knownFace = &result.first->second;
        }
      }
      else
      {
        // Update the existing face:
        knownFace = &existingIter->second;
        
        if(face.GetTimeStamp() > knownFace->face.GetTimeStamp())
        {
          timeSinceLastSeen_ms = face.GetTimeStamp() - knownFace->face.GetTimeStamp();
        }
        else
        {
          PRINT_NAMED_WARNING("FaceWorld.UpdateFace.BadTimeStamp",
                              "Face observed before previous observation (%u <= %u)",
                              face.GetTimeStamp(), knownFace->face.GetTimeStamp());
        }
        
        if(!face.HasEyes()) {
          // If no eyes were detected, the translation we have at this point was
          // computed using "fake" eye locations, so just use the last translation
          // estimate since we matched this to an existing face:
          headPoseWrtWorldOrigin.SetTranslation(knownFace->face.GetHeadPose().GetTranslation());
        }
        
        knownFace->face = face;
      }
    } // if(false == Vision::FaceTracker::IsRecognitionSupported()
    
    // By now, we should have either created a new face or be pointing at an
    // existing one!
    assert(knownFace != nullptr);
    
    knownFace->face.SetHeadPose(headPoseWrtWorldOrigin);
    knownFace->numTimesObserved++;
    
    // Keep up with how many times non-tracking-only faces have been seen facing
    // facing the camera (and thus potentially recognizable)
    if(knownFace->face.IsFacingCamera())
    {
      knownFace->numTimesObservedFacingCamera++;
    }
    
    // Log any DAS events based on this face observation
    const bool isNamed = knownFace->IsNamed();
    if(knownFace->numTimesObserved == 1 && isNamed)
    {
      // Log to DAS that we immediately recognized a new face with a name
      Util::sEventF("robot.vision.face_recognition.immediate_recognition", {{DDATA, kIsNamedStringDAS}},
                    "%d", knownFace->face.GetID());
    }
    else if(!isNamed && knownFace->face.GetID() > 0 && knownFace->numTimesObservedFacingCamera == kNumTimesToSeeFrontalToBeStable)
    {
      // Log to DAS that we've seen this session-only face for awhile and not
      // recognized it as someone else (so this is a stable session-only face)
      // NOTE: we do this just once, when we cross the num times observed threshold
      Util::sEventF("robot.vision.face_recognition.persistent_session_only", {{DDATA, kIsSessionOnlyStringDAS}},
                    "%d", knownFace->face.GetID());
      
      // HACK: increment the counter again so we don't send this multiple times if not seeing frontal anymore
      knownFace->numTimesObservedFacingCamera++;
    }
    else if(timeSinceLastSeen_ms > kTimeUnobservedBeforeReLoggingToDAS_ms && knownFace->HasStableID())
    {
      // Log to DAS that we are re-seeing this face after not having seen it for a bit
      // (and recognizing it as an existing named person or stable session-only ID)
      Util::sEventF("robot.vision.face_recognition.re_recognized", {{DDATA, isNamed ? kIsNamedStringDAS : kIsSessionOnlyStringDAS}}, "%d", knownFace->face.GetID());
    }
    
    // Wait to report this face until we've seen it enough times to be convinced it's
    // not a false positive (random detection), or if it has been recognized already.
    if(knownFace->numTimesObserved >= MinTimesToSeeFace || !knownFace->face.GetName().empty())
    {
      // Update the last observed face pose.
      // If more than one was observed in the same timestamp then take the closest one.
      if (((_lastObservedFaceTimeStamp != knownFace->face.GetTimeStamp()) ||
          (ComputeDistanceBetween(_robot.GetPose(), _lastObservedFacePose) >
           ComputeDistanceBetween(_robot.GetPose(), knownFace->face.GetHeadPose())))) 
      {
        _lastObservedFacePose = knownFace->face.GetHeadPose();
        _lastObservedFaceTimeStamp = knownFace->face.GetTimeStamp();

        // Draw 3D face for the last observed pose
        knownFace->vizHandle = _robot.GetContext()->GetVizManager()->DrawHumanHead(0,
                                                                                   kHumanHeadSize,
                                                                                   knownFace->face.GetHeadPose(),
                                                                                   ::Anki::NamedColors::DARKGRAY);
      }

      // Draw face in 3D and in camera
      DrawFace(*knownFace);


      /*
      PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.AddOrUpdateFace",
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
      
      // Send out an event about this face being observed
      using namespace ExternalInterface;
      
      std::vector<CladPoint2d> leftEye;
      std::vector<CladPoint2d> rightEye;
      std::vector<CladPoint2d> nose;
      std::vector<CladPoint2d> mouth;
      
      const std::vector<std::pair<std::vector<CladPoint2d>*,Vision::TrackedFace::FeatureName>> features{
        {&leftEye,   Vision::TrackedFace::FeatureName::LeftEye},
        {&rightEye,  Vision::TrackedFace::FeatureName::RightEye},
        {&nose,      Vision::TrackedFace::FeatureName::Nose},
        {&mouth,     Vision::TrackedFace::FeatureName::UpperLip},
      };
      
      for(auto const& feature : features)
      {
        for(auto const& pt : face.GetFeature(feature.second))
        {
          feature.first->emplace_back(pt.x(), pt.y());
        }
      }
        
      _robot.Broadcast(MessageEngineToGame(RobotObservedFace(knownFace->face.GetID(),
                                                             _robot.GetID(),
                                                             knownFace->face.GetTimeStamp(),
                                                             knownFace->face.GetHeadPose().ToPoseStruct3d(_robot.GetPoseOriginList()),
                                                             CladRect(knownFace->face.GetRect().GetX(),
                                                                      knownFace->face.GetRect().GetY(),
                                                                      knownFace->face.GetRect().GetWidth(),
                                                                      knownFace->face.GetRect().GetHeight()),
                                                             knownFace->face.GetName(),
                                                             knownFace->face.GetMaxExpression(),
                                                             knownFace->face.GetSmileAmount(),
                                                             knownFace->face.GetGaze(),
                                                             knownFace->face.GetBlinkAmount(),
                                                             knownFace->face.GetExpressionValues(),
                                                             leftEye, rightEye, nose, mouth)));
      
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
  
  Result FaceWorld::Update(const std::list<Vision::TrackedFace>& observedFaces)
  {
    ANKI_CPU_PROFILE("FaceWorld::Update");
    
    for(auto & obsFace : observedFaces)
    {
      Result result = AddOrUpdateFace(obsFace);
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("FaceWorld.Update.AddOrUpdateFaceFailed",
                            "ObservedFace ID=%d", obsFace.GetID());
      }
    }
    
    const TimeStamp_t lastProcImageTime = _robot.GetVisionComponent().GetLastProcessedImageTimeStamp();
    
    // Delete any unnamed faces we haven't seen in awhile
    for(auto faceIter = _knownFaces.begin(); faceIter != _knownFaces.end(); )
    {
      Vision::TrackedFace& face = faceIter->second.face;
      
      if(face.GetName().empty() && (lastProcImageTime > kDeletionTimeout_ms + face.GetTimeStamp()))
      {
        PRINT_CH_INFO(kLoggingChannelName, "FaceWorld.Update.DeletingOldFace",
                      "Removing unnamed face %d at t=%d, because it hasn't been seen since t=%d.",
                      faceIter->first, lastProcImageTime, face.GetTimeStamp());
        
        if(faceIter->second.HasStableID())
        {
          // Log to DAS the removal of any "stable" face that gets removed because
          // we haven't seen it in awhile
          Util::sEventF("robot.vision.remove_unobserved_session_only_face",
                        {{DDATA, std::to_string(face.GetTimeStamp()).c_str()}},
                        "%d", faceIter->first);
        }
        
        RemoveFace(faceIter); // Increments faceIter!
      }
      else
      {
        ++faceIter;
      }
    }
    
    return RESULT_OK;
  } // Update()
  
  const Vision::TrackedFace* FaceWorld::GetFace(Vision::FaceID_t faceID) const
  {
    auto faceIter = _knownFaces.find(faceID);
    if(faceIter == _knownFaces.end()) {
      return nullptr;
    } else {
      return &faceIter->second.face;
    }
  }

  std::set<Vision::FaceID_t> FaceWorld::GetKnownFaceIDs(bool includeTrackingOnlyFaces) const
  {
    std::set<Vision::FaceID_t> faceIDs;
    for (const auto& pair : _knownFaces) {
      if( includeTrackingOnlyFaces || pair.first > 0 ) {
        faceIDs.insert(pair.first);
      }
    }
    return faceIDs;
  }
  
  std::set<Vision::FaceID_t> FaceWorld::GetKnownFaceIDsObservedSince(TimeStamp_t seenSinceTime_ms,
                                                                        bool includeTrackingOnlyFaces) const
  {
    std::set<Vision::FaceID_t> faceIDs;
    for (const auto& pair : _knownFaces) {
      if (pair.second.face.GetTimeStamp() >= seenSinceTime_ms) {
        if( includeTrackingOnlyFaces || pair.first > 0 ) {
          faceIDs.insert(pair.second.face.GetID());
        }
      }
    }
    return faceIDs;
  }

  bool FaceWorld::HasKnownFaces(TimeStamp_t seenSinceTime_ms, bool includeTrackingOnlyFaces) const
  {
    for (const auto& pair : _knownFaces) {
      if (pair.second.face.GetTimeStamp() >= seenSinceTime_ms) {
        if( includeTrackingOnlyFaces || pair.first > 0 ) {
          return true;
        }
      }
    }
    return false;
  }
  
  TimeStamp_t FaceWorld::GetLastObservedFace(Pose3d& p) const
  {
    if (_lastObservedFaceTimeStamp > 0) {
      p = _lastObservedFacePose;
    }
    
    return _lastObservedFaceTimeStamp;
  }

  TimeStamp_t FaceWorld::GetLastObservedFaceWithRespectToRobot(Pose3d& p) const
  {
    if (_lastObservedFaceTimeStamp > 0) {
      p = _lastObservedFacePose;

      if( ! p.GetWithRespectTo(_robot.GetPose(), p ) ) {
        // don't have a real transform, so just pretend the origins are the same
        Pose3d lastFaceWRTOrigin = _lastObservedFacePose.GetWithRespectToOrigin();
        p = Pose3d{ lastFaceWRTOrigin.GetRotation(),
                    lastFaceWRTOrigin.GetTranslation(),
                    _robot.GetWorldOrigin() };

        if( ! p.GetWithRespectTo(_robot.GetPose(), p) ) {
          PRINT_NAMED_ERROR("FaceWorld.LastObservedFace.PoseError",
                            "BUG: couldn't get new pose with respect to robot. This should never happen");
          return 0;
        }
      }
    }
    
    return _lastObservedFaceTimeStamp;    
  }

  bool FaceWorld::HasTurnedTowardsFace(Vision::FaceID_t faceID) const
  {
    const auto& it = _knownFaces.find(faceID);
    if( it == _knownFaces.end() ) {
      // either this is a bad ID, or the face was deleted, so assume we haven't animated at it. Note that (as
      // of this comment writing...) named faces are not deleted
      return false;
    }

    return it->second.hasTurnedTowards;
  }

  void FaceWorld::SetTurnedTowardsFace(Vision::FaceID_t faceID, bool val)
  {
    auto it = _knownFaces.find(faceID);
    if( it == _knownFaces.end() ) {
      PRINT_NAMED_WARNING("FaceWorld.SetTurnedTowardsFaceAndAnimation.InvalidFace",
                          "Claiming that we animated at face %d, but that face doesn't exist in FaceWorld",
                          faceID);
      return;
    }
    
    it->second.hasTurnedTowards = val;
  }

  void FaceWorld::DrawFace(KnownFace& knownFace)
  {
    const Vision::TrackedFace& trackedFace = knownFace.face;
    
    ColorRGBA drawFaceColor = ColorRGBA::CreateFromColorIndex((u32)trackedFace.GetID());
    
    const s32 vizID = (s32)trackedFace.GetID() + (trackedFace.GetID() >= 0 ? 1 : 0);
    knownFace.vizHandle = _robot.GetContext()->GetVizManager()->DrawHumanHead(vizID,
                                                                              kHumanHeadSize,
                                                                              trackedFace.GetHeadPose(),
                                                                              drawFaceColor);
    
    // Draw box around recognized face (with ID) now that we have the real ID set
    _robot.GetContext()->GetVizManager()->DrawCameraFace(trackedFace, drawFaceColor);

  }
  
  void FaceWorld::Enroll(Vision::FaceID_t faceID)
  {
    // If starting session enrollment, then set the num enrollments to -1 to get "ongoing"
    // enrollment. Otherwise, use the max we can store.
    const bool sessionOnly = (Vision::UnknownFaceID == faceID);
    const s32 numEnrollmentsRequired = (sessionOnly ? -1 :
                                        (s32)Vision::FaceRecognitionConstants::MaxNumEnrollDataPerAlbumEntry);
    
    _robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight,
                                                      faceID,
                                                      numEnrollmentsRequired);
  }

} // namespace Cozmo
} // namespace Anki

