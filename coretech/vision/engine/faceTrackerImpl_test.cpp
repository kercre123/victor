/**
 * File: faceTrackerImpl_okao.cpp
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for OKAO Vision face detection library.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#if 1

#include <unistd.h>

#include "faceTrackerImpl_test.h"

#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/okaoParamInterface.h"

#include "util/console/consoleInterface.h"
#include "util/helpers/boundedWhile.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Vision {

static const f32 DistanceBetweenEyes_mm = 62.f;
static const f32 MinDistBetweenEyes_pixels = 6;

CONSOLE_VAR(s32, kNumberOfTimesCalledBeforeRecog,         "Vision.FaceTracker",  200);
CONSOLE_VAR(s32, kFaceDetectionDelay_us,                  "Vision.FaceTracker",  100*1000);
CONSOLE_VAR(s32, kFaceDetectionDelayDuringEnrollment_us,  "Vision.FaceTracker",  1000*1000);
CONSOLE_VAR(s32, kFrequencyOfRecognitionPrints,           "Vision.FaceTracker",  10);
CONSOLE_VAR(s32, kMinimumRecognitionFrames,               "Vision.FaceTracker",  50);
CONSOLE_VAR(s32, kLoseFaceAfterEnrollmentFrames,          "Vision.FaceTracker",  20);
//CONSOLE_VAR(s32, kFaceRecognitionDelay_us,         "Vision.FaceTracker",  1000*1000);

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
  // Need a time delay
  PRINT_NAMED_INFO("FaceTrackerImpl.Update.FakeUpdate", "");
  if (_startedEnrolling)
  {
    usleep(kFaceDetectionDelayDuringEnrollment_us);
  }
  else
  {
    usleep(kFaceDetectionDelay_us);
  }


  // Check if we should just quit, because we're trying to simulate 
  // "losing" the face. Not sure if this is going to work though. I might
  // need to delete something but ... I don't think so.
  if (_startedEnrolling)
  {
    if (_numberOfEnrollmentFrames >= kLoseFaceAfterEnrollmentFrames)
    {
      return RESULT_OK;
    }
  }

  // Add a new face to the list
  faces.emplace_back();
  const s32 nWidth  = frameOrig.GetNumCols();
  const s32 nHeight = frameOrig.GetNumRows();

  // Add a new face to the list
  faces.emplace_back();
  TrackedFace& face = faces.back();

  face.SetIsBeingTracked(true);
  // TODO need better fake rectangle
  // we want this to be centered so we don't keep moving ... I think
  face.SetRect(Rectangle<f32>(.45, .45, .1, .1));

  face.SetTimeStamp(frameOrig.GetTimestamp());

  // Not sure what to do with recognizer, do we even need it?

  // Not sure if I need this
  f32 intraEyeDist = -1.f;
  SetFacePoseWithoutParts(nHeight, nWidth, face, intraEyeDist);
  // Not entirely sure what or who this id should be
  // hopefully it just works
  face.SetID(1);

  if (_totalNumberOfCalls > kNumberOfTimesCalledBeforeRecog)
  {
    // Not sure if these id's are what they should be
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

  if (_startedEnrolling)
  {
    if (_numberOfEnrollmentFrames % kFrequencyOfRecognitionPrints == 0)
    {
      PRINT_NAMED_INFO("FaceTrackerImpl.Update.NumberOfEnrollmentFames",
                       "numer of enrollment frames is now %d", _numberOfEnrollmentFrames);
    }

    // usleep(faceRecognitionDelay_us);
    if (_numberOfEnrollmentFrames > kMinimumRecognitionFrames)
    {
      _enrollmentComplete = 1;
      face.SetNumEnrollments(_enrollmentComplete);
      PRINT_NAMED_INFO("FaceTrackerImpl.Update.NumFakeEnrollments",
                       "numer of enrollments is now %d", _numberOfEnrollments);
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
