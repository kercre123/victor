/**
 * File: faceTrackerImpl_opencv.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Wrapper for OpenCV face detector / tracker.
 *
 * NOTE: This file should only be included by faceTracker.cpp
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/shared/math/rect_impl.h"
#include "coretech/common/shared/math/rotation.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <list>
#include <map>

namespace Anki {
namespace Vision {
  
  class FaceTracker::Impl : public Profiler
  {
  public:
    Impl(const std::string& modelPath, const Json::Value& config);
    
    Result Update(const Vision::Image& frameOrig,
                  std::list<TrackedFace>& faces,
                  std::list<FaceTracker::UpdatedID>& updatedIDs);
    
    void EnableDisplay(bool enabled) { }
    
    static bool IsRecognitionSupported() { return false; }
    
    std::vector<u8> GetSerializedAlbum() { return std::vector<u8>(); }
    Result SetSerializedAlbum(const std::vector<u8>& serializedAlbum) { return RESULT_FAIL; }
    
    Result GetSerializedData(std::vector<u8>& albumData,
                             std::vector<u8>& enrollData)
    {
      return RESULT_FAIL;
    }

    Result SetSerializedData(const std::vector<u8>& albumData,
                             const std::vector<u8>& enrollData,
                             std::list<FaceNameAndID>& namesAndIDs)
    {
      return RESULT_FAIL;
    }

    // No-ops but req'd by FaceTracker
    Result AssignNameToID(FaceID_t faceID, const std::string &name) { return RESULT_FAIL; }

    FaceID_t EraseFace(const std::string& name) { return UnknownFaceID; }
    Result   EraseFace(FaceID_t faceID) { return RESULT_FAIL; }

    Result SaveAlbum(const std::string& albumName) { return RESULT_FAIL; }
    Result LoadAlbum(const std::string& albumName, std::list<std::string>& names) { return RESULT_FAIL; }
    void   PrintTiming() { }
    
  private:
    
    bool _isInitialized = false;
    
    cv::CascadeClassifier _faceCascade;
    cv::CascadeClassifier _eyeCascade;
    
    u32 _faceCtr = 0;
    
    u32 _frameCtr = 0;
    const u32 _checkForNewFacesEveryNthFrame = 5; // TODO: Make a settable parameter?
    
    std::map<FaceID_t, TrackedFace> _faces;
    
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();

  }; // class FaceTracker::Impl
  
  
  FaceTracker::Impl::Impl(const std::string& modelPath, const Json::Value& config)
  {
    const std::string faceCascadeFilename = modelPath + "/haarcascade_frontalface_alt2.xml";
    
    bool loadSuccess = _faceCascade.load(faceCascadeFilename);
    if(!loadSuccess) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.LoadFaceCascade",
                        "Failed to load face cascade from %s",
                        faceCascadeFilename.c_str());
      return;
    }
    
    const std::string eyeCascadeFilename = modelPath + "/haarcascade_eye_tree_eyeglasses.xml";
    
    loadSuccess = _eyeCascade.load(eyeCascadeFilename);
    if(!loadSuccess) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.LoadEyeCascade",
                        "Failed to load eye cascade from %s",
                        eyeCascadeFilename.c_str());
      return;
    }
    
    // TODO: Make a settable parameter, and/or apply this further up the chain
    clahe->setClipLimit(4);
    
    _isInitialized = true;
  }
  
  Result FaceTracker::Impl::Update(const Vision::Image& frame,
                                   std::list<TrackedFace>& returnedFaces,
                                   std::list<FaceTracker::UpdatedID>& updatedIDs)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("FaceTracker.Impl.Update.Uninitialized", "");
      return RESULT_FAIL;
    }
    
    // Apply contrast-limited adaptive histogram equalization (CLAHE) to help
    // normalize illumination
    // TODO: apply this further up the chain
    cv::Mat cvFrame;
    clahe->apply(frame.get_CvMat_(), cvFrame);
    
    if(_frameCtr++ == _checkForNewFacesEveryNthFrame)
    {
      _frameCtr = 0;
      
      // Search entire frame
      std::vector<cv::Rect> newFaceRects;
      _faceCascade.detectMultiScale(cvFrame, newFaceRects,
                                    opencvDetectScaleFactor, 2, 0,
                                    opencvDetectMinFaceSize);
      
      // Match detections to existing faces if they overlap enough
      const f32 intersectionOverUnionThreshold = 0.5f;
      
      // Keep a list of existing faces to check and remove any that we find matches
      // for in the loop below
      std::list<std::map<FaceID_t,TrackedFace>::iterator> existingFacesToCheck;
      for(auto iter = _faces.begin(); iter != _faces.end(); ++iter) {
        existingFacesToCheck.emplace_back(iter);
      }
      
      for(auto & newFaceRect : newFaceRects)
      {
        Rectangle<f32> rect_f32(newFaceRect);
        
        bool matchFound = false;
        for(auto existingIter = existingFacesToCheck.begin();
            !matchFound && existingIter != existingFacesToCheck.end(); )
        {
          TrackedFace& existingFace = (*existingIter)->second;
          const Rectangle<f32>& existingRect = existingFace.GetRect();
          const f32 IoU = existingRect.ComputeOverlapScore(rect_f32);
          if(IoU > intersectionOverUnionThreshold) {
            // Update existing face and remove it from additional checking for matches
            existingFace.SetRect(std::move(rect_f32));
            existingIter = existingFacesToCheck.erase(existingIter);
            matchFound = true;
            break;
          } else {
            ++existingIter;
          }
        } // for each existing face
        
        if(!matchFound) {
          // Add as new face
          TrackedFace newFace;
          
          newFace.SetRect(Rectangle<f32>(newFaceRect));
          newFace.SetID(_faceCtr++);
          _faces.emplace(newFace.GetID(), newFace);
        }
      }
      
      // Remove any faces we are no longer seeing
      for(auto oldFace : existingFacesToCheck) {
        _faces.erase(oldFace);
      }
      
    } else {
      // Search in ROIs around existing faces
      std::vector<cv::Rect> updatedRects;
      for(auto faceIter = _faces.begin(); faceIter != _faces.end(); )
      {
        Rectangle<f32> faceRect = faceIter->second.GetRect().Scale(1.5f);
        cv::Rect_<f32> cvFaceRect = (faceRect.get_CvRect_() &
                                    cv::Rect_<f32>(0, 0, frame.GetNumCols(), frame.GetNumRows()));
        
        cv::Mat faceRoi = cvFrame(cvFaceRect);
        
        _faceCascade.detectMultiScale(faceRoi, updatedRects,
                                      opencvDetectScaleFactor, 2, 0,
                                      cv::Size(cvFaceRect.width/2, cvFaceRect.height/2));
        if(updatedRects.empty()) {
          // Lost face
          faceIter = _faces.erase(faceIter);
        } else {
          // Update face
          if(updatedRects.size() > 1) {
            PRINT_NAMED_WARNING("FaceTracker.Impl.Update.MultipleFaceUpdates",
                                "Found more than one face in vicinity of existing face. Using first.");
          }
          faceIter->second.SetRect(Rectangle<f32>(cvFaceRect.x + updatedRects[0].x,
                                                  cvFaceRect.y + updatedRects[0].y,
                                                  updatedRects[0].width, updatedRects[0].height));
          ++faceIter;
        }
      }
    }
    
    // Update all remaining existing faces
    for(auto & facePair : _faces)
    {
      TrackedFace& face = facePair.second;
      
      face.SetTimeStamp(frame.GetTimestamp());
      
      // Just use assumed eye locations within the rectangle
      std::vector<cv::Rect> eyeRects;
      const cv::Rect_<float>& faceRect = face.GetRect().get_CvRect_();
      cv::Mat faceRoi = cvFrame(faceRect);
      _eyeCascade.detectMultiScale(faceRoi, eyeRects, 1.25, 2, 0, cv::Size(5,5),
                                    cv::Size(faceRoi.cols/4,faceRoi.rows/4));
      if(eyeRects.size() == 2) {
        // Iff we find two eyes within the face rectangle, use them
        cv::Rect& leftEyeRect = eyeRects[0];
        cv::Rect& rightEyeRect = eyeRects[1];
        if(eyeRects[0].x > eyeRects[1].x) {
          std::swap(leftEyeRect, rightEyeRect);
        }
        
        Point2f leftEyeCen(faceRect.x + leftEyeRect.x + leftEyeRect.width/2,
                           faceRect.y + leftEyeRect.y + leftEyeRect.height/2);
        Point2f rightEyeCen(faceRect.x + rightEyeRect.x + rightEyeRect.width/2,
                            faceRect.y + rightEyeRect.y + rightEyeRect.height/2);

        // Use the line connecting the eyes to estimate head roll:
        Point2f eyeLine(rightEyeCen);
        eyeLine -= leftEyeCen;
        face.SetHeadOrientation(std::atan2f(eyeLine.y(), eyeLine.x()), 0, 0);
        
        face.SetEyeCenters(std::move(leftEyeCen), std::move(rightEyeCen));
      } 
      
    }
    
    // Populate the returned faces list 
    for(auto & existingFace : _faces) {
      returnedFaces.emplace_back(existingFace.second);
    }
    
    return RESULT_OK;
  } // Update()
  
} // namespace Vision
} // namespace Anki


