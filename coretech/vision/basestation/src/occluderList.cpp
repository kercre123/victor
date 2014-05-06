//
//  occluderList.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 5/6/2014.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/rect.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/occluderList.h"
#include "anki/vision/basestation/observableObject.h"

namespace Anki {
  
  namespace Vision {
    
    OccluderList::OccluderList()
    : isSorted_(false)
    {
      
    }
    
    void OccluderList::Clear()
    {
      rectDepthPairs_.clear();
      isSorted_ = false;
    }
    
    /* Moved to Camera class
    void OccluderList::AddOccluder(const Vision::ObservableObject* object, const Vision::Camera& camera)
    {
      const Pose3d objectPoseWrtCamera(object->GetPose().getWithRespectTo(&camera.get_pose()));
      
      std::vector<Point3f> cornersAtPose;
      std::vector<Point2f> projectedCorners;
      
      // Project the objects's corners into the image and create an occluding
      // bounding rectangle from that
      object->GetCorners(cornersAtPose);
      camera.project3dPoints(cornersAtPose, projectedCorners);
      
      AddOccluderHelper(projectedCorners, objectPoseWrtCamera.get_translation().z());
    }
     */
    
    template<size_t NumPoints>
    void OccluderList::AddOccluder(const std::array<Point2f,NumPoints>& points, const f32 atDistance)
    {
      AddOccluderHelper(points, atDistance);
    }
    
    void OccluderList::AddOccluder(const Quad2f &quad, const f32 atDistance)
    {
      AddOccluderHelper(quad, atDistance);
    }
    
    void OccluderList::AddOccluder(const std::vector<Point2f>& points, const f32 atDistance)
    {
      AddOccluderHelper(points, atDistance);
    }
    
    template<class PointContainer>
    void OccluderList::AddOccluderHelper(const PointContainer& points, const f32 atDistance)
    {
      rectDepthPairs_.push_back(std::make_pair(Rectangle<f32>(points), atDistance));
      
      // If we add a new occluder, mark the vector as unsorted
      isSorted_ = false;
      
    }
    
    
    bool OccluderList::IsOccluded(const Quad2f &quad, const f32 atDistance)
    {
      if(not isSorted_) {
        std::sort(rectDepthPairs_.begin(), rectDepthPairs_.end(), CompareSecond<Quad2f, f32>());
        isSorted_ = true;
      }
      
      // Model the quad by its axis-aligned rectangle for simpler intersection
      // checking
      Rectangle<f32> boundingBox(quad);
      
      // Don't need to check against occluders in the container that are further
      // away than the quad I'm checking (they can't possibly be occluding), so
      // we only have to go until the current occluder is behind the quad we're
      // checking, since the vector is sorted
      auto currentOccluder = rectDepthPairs_.begin();
      auto end = rectDepthPairs_.end();
      while(atDistance > currentOccluder->second && currentOccluder != end) {
        
        if( boundingBox.Intersect(currentOccluder->first).area() > 0 ) {
          // The bounding box intersects this occluder, and is thus occluded
          // by it.
          return true;
        }
        
        ++currentOccluder;
      }
      
      // No intersections with occluders found
      return false;
      
    } // OccluderList::IsOccluded()
    
    
    bool OccluderList::IsOccluded(const Point2f &point, const f32 atDistance)
    {
      if(not isSorted_) {
        std::sort(rectDepthPairs_.begin(), rectDepthPairs_.end(), CompareSecond<Quad2f, f32>());
        isSorted_ = true;
      }
      
      // Don't need to check against occluders in the container that are further
      // away than the quad I'm checking (they can't possibly be occluding), so
      // we only have to go until the current occluder is behind the quad we're
      // checking, since the vector is sorted
      auto currentOccluder = rectDepthPairs_.begin();
      auto end = rectDepthPairs_.end();
      while(atDistance > currentOccluder->second && currentOccluder != end) {
        
        if(currentOccluder->first.Contains(point)) {
          // This occluder contains the point, and thus occludes it.
          return true;
        }
        
        ++currentOccluder;
      }
      
      // No intersections with occluders found
      return false;
      
    } // OccluderList::IsOccluded()

    
    
  } // namespace Vision
  
} // namespace Anki
