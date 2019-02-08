//
//  occluderList.cpp
//  CoreTech_Vision
//
//  Created by Andrew Stein on 5/6/2014.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/shared/math/rect_impl.h"

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/occluderList.h"
#include "coretech/vision/engine/observableObject.h"

namespace Anki {
  
  namespace Vision {
    
    OccluderList::OccluderList()
    {
      
    }
    
    void OccluderList::Clear()
    {
      rectDepthPairs_.clear();
    }
    
    /* Moved to Camera class
    void OccluderList::AddOccluder(const Vision::ObservableObject* object, const Vision::Camera& camera)
    {
      const Pose3d objectPoseWrtCamera(object->GetPose().GetWithRespectTo(&camera.get_pose()));
      
      std::vector<Point3f> cornersAtPose;
      std::vector<Point2f> projectedCorners;
      
      // Project the objects's corners into the image and create an occluding
      // bounding rectangle from that
      object->GetCorners(cornersAtPose);
      camera.Project3dPoints(cornersAtPose, projectedCorners);
      
      AddOccluderHelper(projectedCorners, objectPoseWrtCamera.GetTranslation().z());
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
      Rectangle<f32> occluder(points);
      
      auto currentOccluderAtDistance = rectDepthPairs_.find(atDistance);
      if(currentOccluderAtDistance != rectDepthPairs_.end()) {
        // Already have an occluder at this distance.  Keep the larger one
        if(occluder.Area() > currentOccluderAtDistance->second.Area()) {
          currentOccluderAtDistance->second = occluder;
        }
      } else {
        rectDepthPairs_[atDistance] = occluder;
      }
    }
    
    
    bool OccluderList::IsOccluded(const Quad2f &quad, const f32 atDistance) const
    {
      if(!IsEmpty()) {
        // Model the quad by its axis-aligned rectangle for simpler intersection
        // checking
        Rectangle<f32> boundingBox(quad);
        
        // Don't need to check against occluders in the container that are further
        // away than the quad I'm checking (they can't possibly be occluding), so
        // we only have to go until the current occluder is behind the quad we're
        // checking, since the vector is sorted
        auto currentOccluder = rectDepthPairs_.begin();
        auto end = rectDepthPairs_.end();
        while(currentOccluder != end && atDistance > currentOccluder->first) {
          
          if( boundingBox.Intersect(currentOccluder->second).Area() > 0 ) {
            // The bounding box intersects this occluder, and is thus occluded
            // by it.
            return true;
          }
          
          ++currentOccluder;
        }
      } // if not empty
      
      // No intersections with occluders found
      return false;
      
    } // OccluderList::IsOccluded(quad)
    
    
    bool OccluderList::IsOccluded(const Point2f &point, const f32 atDistance) const
    {
      if(!IsEmpty()) {
        // Don't need to check against occluders in the container that are further
        // away than the quad I'm checking (they can't possibly be occluding), so
        // we only have to go until the current occluder is behind the quad we're
        // checking, since the vector is sorted
        auto currentOccluder = rectDepthPairs_.begin();
        auto end = rectDepthPairs_.end();
        while(currentOccluder != end && atDistance > currentOccluder->first) {
          
          if(currentOccluder->second.Contains(point)) {
            // This occluder contains the point, and thus occludes it.
            return true;
          }
          
          ++currentOccluder;
        }
      } // if not empty
      
      // No intersections with occluders found
      return false;
      
    } // OccluderList::IsOccluded(point)

    
    bool OccluderList::IsAnythingBehind(const Point2f &point, const f32 atDistance) const
    {
      if(!IsEmpty()) {
        // Jump to first occluder farther away than the given point
        auto currentOccluder = rectDepthPairs_.begin();
        auto end = rectDepthPairs_.end();
        while(currentOccluder != end && atDistance >= currentOccluder->first) {
          ++currentOccluder;
        }
        
        // Check the remaining occluders from here back, if there are any
        while(currentOccluder != end) {
          // If we get here, the current occluder should be one that is further
          // away than the specified distance
          CORETECH_ASSERT(atDistance < currentOccluder->first);
          
          if(currentOccluder->second.Contains(point)) {
            // If the point is within the occluder, then the occluder is behind it
            return true;
          }
          
          ++currentOccluder;
        }
          
      } // if not empty
      
      return false;
      
    } // IsAnythingBehind()
    
    
    bool OccluderList::IsAnythingBehind(const Quad2f &quad, const f32 atDistance) const
    {
      if(!IsEmpty()) {
        // Jump to first occluder farther away than the given point
        auto currentOccluder = rectDepthPairs_.begin();
        auto end = rectDepthPairs_.end();
        while(currentOccluder != end && atDistance >= currentOccluder->first) {
          ++currentOccluder;
        }
        
        const Rectangle<f32> boundingBox(quad);
        
        // Check the remaining occluders from here back, if there are any
        while(currentOccluder != end) {
          // If we get here, the current occluder should be one that is further
          // away than the specified distance
          CORETECH_ASSERT(atDistance < currentOccluder->first);
          
          if(boundingBox.Intersect(currentOccluder->second).Area() > 0) {
            // If the quad's bounding box intersects the occluder, then the occluder is behind it
            return true;
          }
          
          ++currentOccluder;
        }
        
      } // if not empty
      
      return false;
      
    } // IsAnythingBehind()
    
    
    void OccluderList::GetAllOccluders(std::vector<Rectangle<f32> >& occluders) const
    {
      for(const auto& rectDepthPair : rectDepthPairs_)
      {
        occluders.emplace_back(rectDepthPair.second);
      }
    }
    
    
  } // namespace Vision
  
} // namespace Anki
