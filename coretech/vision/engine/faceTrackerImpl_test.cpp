/**
 * File: faceTrackerImpl_test.cpp
 *
 * Author: Robert Cosgriff
 * Date:   9/3/2018
 *
 * Description: see header
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#if FACE_TRACKER_PROVIDER == FACE_TRACKER_TEST

#include <unistd.h>

#include "faceTrackerImpl_test.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/camera.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vision {

// TODO these are duplicated from the OKAO implementation
// and eventually they should be moved to a shared location
static const f32 DistanceBetweenEyes_mm = 62.f;
static const f32 MinDistBetweenEyes_pixels = 6;

// This controls how many frames are needed before we populate an updated face
CONSOLE_VAR(s32, kNumberOfFramesBeforeUpdatedFace,         "Vision.FaceTracker",  200);

// This is the delay for the detection of faces, when not enrolling
CONSOLE_VAR(s32, kFaceDetectionDelay_ms,                   "Vision.FaceTracker",  100);

// This is the delay for the detection of faces, when enrolling 
CONSOLE_VAR(s32, kFaceDetectionDelayDuringEnrollment_ms,    "Vision.FaceTracker",  1000);

// The minimum number of recognition frames needed before an enrollment completes
CONSOLE_VAR(s32, kFramesToCompleteEnrollment,               "Vision.FaceTracker",  50);

// This is how many frames are needed before we lose a face
CONSOLE_VAR(s32, kFramesToLoseFaceAfterEnrollment,          "Vision.FaceTracker",  2000);

// This is the delay for recognition. It isn't all that useful in testing because
// it assumes that the OKAO implementation is running in synchronous mode. For the
// situations we are trying to test here we probably don't care about the synchorous
// use case.
CONSOLE_VAR(s32, kFaceRecognitionDelay_ms,                  "Vision.FaceTracker",  0);

FaceTracker::Impl::Impl(const Camera&        camera,
                         const std::string&   modelPath,
                         const Json::Value&   config)
  : _camera(camera)
{
}

FaceTracker::Impl::~Impl()
{
}

Result FaceTracker::Impl::Update(const Vision::Image& frameOrig,
                                 std::list<TrackedFace>& faces,
                                 std::list<UpdatedFaceID>& updatedIDs)
{
  // This is the detection time delay to simulate the face detector
  // portion of face tracker running slow. The time delay can be varied
  // depending on whether we are enrolling or not.
  if (_startedEnrolling)
  {
    usleep(kFaceDetectionDelayDuringEnrollment_ms*1000);
  }
  else
  {
    usleep(kFaceDetectionDelay_ms*1000);
  }

  // Check if we should just quit to simulate "losing" the face.
  if (_startedEnrolling)
  {
    if (_numberOfEnrollmentFrames >= kNumberOfFramesToLoseFaceAfterEnrollment)
    {
      return RESULT_OK;
    }
  }

  // Add a new face to the list.
  faces.emplace_back();
  const s32 nWidth  = frameOrig.GetNumCols();
  const s32 nHeight = frameOrig.GetNumRows();

  const s32 detectRectWidth = 100;
  const s32 detectRectHeight = 100;

  TrackedFace& face = faces.back();
  face.SetIsBeingTracked(true);

  // Center the face so the robot doesn't keep trying to turn towards it.
  face.SetRect(Rectangle<f32>(nWidth / 2 - (detectRectWidth / 2),
                              nHeight / 2 - (detectRectHeight / 2),
                              detectRectWidth,
                              detectRectHeight));

  face.SetTimeStamp(frameOrig.GetTimestamp());

  f32 intraEyeDist = -1.f;
  SetFacePoseWithoutParts(nHeight, nWidth, face, intraEyeDist);
  face.SetID(1);

  if (_totalNumberOfCalls > kNumberOfFramesBeforeUpdatedFace)
  {
    UpdatedFaceID update{
      .oldID   = 1,
      .newID   = 3,
      .newName = ""
    };
    if(updatedIDs.empty() ||
       (update.oldID != updatedIDs.back().oldID &&
        update.newID != updatedIDs.back().newID))
    {
      updatedIDs.push_back(std::move(update));
    }
  }

  // Only do recognition once we start enrolling.
  if (_startedEnrolling)
  {
    // This is the delay specific to recognition. This wasn't the most useful
    // knob to turn for turn for testing with behavior enroll face.
    usleep(kFaceRecognitionDelay_ms*1000);

    // The more useful bit with testing enrollment was to vary when the
    // enrollment is completed, this happens below.
    if (_numberOfEnrollmentFrames > kFramesToCompleteEnrollment)
    {
      _enrollmentComplete = 1;
      face.SetNumEnrollments(_enrollmentComplete);
      PRINT_NAMED_INFO("FaceTrackerImpl.Update.NumFakeEnrollments",
                       "numer of enrollments is now %d", _numberOfEnrollments);

      // Reset some book keeping so we can do multiple enrollments without
      // needing to redeploy/restart the robot.
      _enrollmentComplete = 0;
      _startedEnrolling = false;
      _numberOfEnrollmentFrames = 0;
    }
    else
    {
      ++_numberOfEnrollmentFrames;
    }
  }

  _totalNumberOfCalls += 1;
  return RESULT_OK;
}

// This is called when we should start simulating enrollment
void FaceTracker::Impl::SetFaceEnrollmentMode(Vision::FaceEnrollmentPose pose,
                                              Vision::FaceID_t forFaceID,
                                              s32 numEnrollments)
{
  PRINT_NAMED_INFO("FaceTrackerImpl.SetFaceEnrollmentMode.NumFakeEnrollments",
                   "numer of enrollments %d for face %d", forFaceID, numEnrollments);
  if (numEnrollments > 0)
  {
    _startedEnrolling = true;
  }
  else
  {
    _startedEnrolling = false;
  }
}

// TODO this is duplicated code but isn't compiled in from the OKAO implementation,
// eventually we should move this some place where it isn't duplicated.
static Vec3f GetTranslation(const Point2f& leftEye, const Point2f& rightEye, const f32 intraEyeDist,
                            const CameraCalibration& scaledCalib)
{
  // Get unit vector along camera ray from the point between the eyes in the image
  Point2f eyeMidPoint(leftEye);
  eyeMidPoint += rightEye;
  eyeMidPoint *= 0.5f;

  Vec3f ray(eyeMidPoint.x(), eyeMidPoint.y(), 1.f);
  ray = scaledCalib.GetInvCalibrationMatrix() * ray;
  ray.MakeUnitLength();

  ray *= scaledCalib.GetFocalLength_x() * DistanceBetweenEyes_mm / intraEyeDist;

  return ray;
}

// TODO same as above this is duplicated code but isn't compiled in from the OKAO implementation,
// eventually we should move to some place where it isn't duplicated.
Result FaceTracker::Impl::SetFacePoseWithoutParts(const s32 nrows, const s32 ncols, TrackedFace& face, f32& intraEyeDist)
{
  // Without face parts detected (which includes eyes), use fake eye centers for finding pose
  auto const& rect = face.GetRect();
  DEV_ASSERT(rect.Area() > 0, "FaceTrackerImpl.SetFacePoseWithoutParts.InvalidFaceRectangle");
  const Point2f leftEye( rect.GetXmid() - .25f*rect.GetWidth(),
                        rect.GetYmid() - .125f*rect.GetHeight() );
  const Point2f rightEye( rect.GetXmid() + .25f*rect.GetWidth(),
                         rect.GetYmid() - .125f*rect.GetHeight() );

  intraEyeDist = std::max((rightEye - leftEye).Length(), MinDistBetweenEyes_pixels);

  const CameraCalibration& scaledCalib = _camera.GetCalibration()->GetScaled(nrows, ncols);

  // Use the eye positions and raw intra-eye distance to compute the head's translation
  const Vec3f& T = GetTranslation(leftEye, rightEye, intraEyeDist, scaledCalib);
  Pose3d headPose = face.GetHeadPose();
  headPose.SetTranslation(T);
  headPose.SetParent(_camera.GetPose());
  face.SetHeadPose(headPose);

  // We don't know anything about orientation without parts, so don't update it and assume
  // _not_ facing the camera (without actual evidence that we are)
  face.SetIsFacingCamera(false);

  return RESULT_OK;
}

// The rest of these methods are just to be consistent with the api
// defined in face tracker, but don't do anything significant for this
// testing implementation.
void FaceTracker::Impl::SetRecognitionIsSynchronous(bool isSynchronous)
{
}

void FaceTracker::Impl::AddAllowedTrackedFace(const FaceID_t faceID)
{
}

bool FaceTracker::Impl::HaveAllowedTrackedFaces()
{
  return false;
}

void FaceTracker::Impl::ClearAllowedTrackedFaces()
{
}

bool FaceTracker::Impl::IsRecognitionSupported()
{
  return true;
}

float FaceTracker::Impl::GetMinEyeDistanceForEnrollment()
{
  return 0.f;
}

bool FaceTracker::Impl::CanAddNamedFace() const
{
  return true;
}

Result FaceTracker::Impl::AssignNameToID(FaceID_t faceID, const std::string &name, FaceID_t mergeWithID)
{
  return RESULT_OK;
}

Result FaceTracker::Impl::EraseFace(FaceID_t faceID)
{
  return RESULT_OK;
}

void FaceTracker::Impl::EraseAllFaces()
{
}

std::vector<Vision::LoadedKnownFace> FaceTracker::Impl::GetEnrolledNames() const
{
  std::vector<Vision::LoadedKnownFace> newStuff;
  return newStuff;
}

Result FaceTracker::Impl::RenameFace(FaceID_t faceID, const std::string& oldName, const std::string& newName,
                                     Vision::RobotRenamedEnrolledFace& renamedFace)
{
  return RESULT_OK;
}

Result FaceTracker::Impl::SaveAlbum(const std::string& albumName)
{
  return RESULT_OK;
}

Result FaceTracker::Impl::LoadAlbum(const std::string& albumName, std::list<LoadedKnownFace>& loadedFaces)
{
  return RESULT_OK;
}

void FaceTracker::Impl::PrintTiming()
{
}

Result FaceTracker::Impl::GetSerializedData(std::vector<u8>& albumData,
                                            std::vector<u8>& enrollData)
{
  return RESULT_OK;
}

Result FaceTracker::Impl::SetSerializedData(const std::vector<u8>& albumData,
                                            const std::vector<u8>& enrollData,
                                            std::list<LoadedKnownFace>& loadedFaces)
{
  return RESULT_OK;
}

}
}

#endif 
