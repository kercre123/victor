/**
 * File: faceTracker.h
 *
 * Author: Andrew Stein
 * Date:   8/18/2015
 *
 * Description: Wrapper for underlying face detection and tracking mechanism, 
 *              the details of which are hidden inside a private implementation.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Vision_FaceTracker_H__
#define __Anki_Vision_FaceTracker_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"

#include <opencv2/core/core.hpp>

namespace Anki {  
namespace Vision {
 
  // TODO: This should probably be a full-blown class
  // FaceLandmarks are a vector of "polygons", where each polygon
  //  is a vector of points paired with a bool indicating whether
  //  it is a closed polygon or not.
  using FaceLandmarks = std::vector<std::pair<std::vector<Point2f>,bool> >;
  
  class FaceTrackerImpl;
  
  class FaceTracker
  {
  public:
    
    FaceTracker(const std::string& modelPath);
    ~FaceTracker();
    
    Result Update(const cv::Mat& frameOrig);
    
    void GetFaces(std::vector<cv::Rect>& faceRects) const;
    
    void GetFaceLandmarks(std::vector<FaceLandmarks>& landmarks) const;
    
    void EnableDisplay(bool enabled);
    
  private:
    
    FaceTrackerImpl* _pImpl;
    
  }; // class FaceTracker
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_FaceTracker_H__