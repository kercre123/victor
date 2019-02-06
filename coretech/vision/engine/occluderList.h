//
//  occluderList
//  CoreTech_Vision
//
//  Created by Andrew Stein on 5/6/2014.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef ANKI_CORETECH_VISION_OCCLUDERLIST_H
#define ANKI_CORETECH_VISION_OCCLUDERLIST_H

#include <vector>
#include <array>

#include "coretech/common/shared/math/rect.h"

namespace Anki {
    
  namespace Vision {
    
    // Forward declarations:
    class ObservableObject;
    
    class OccluderList
    {
    public:
      OccluderList();
      
      void AddOccluder(const Quad2f& quad, const f32 atDistance);
      
      template<size_t NumPoints>
      void AddOccluder(const std::array<Point2f,NumPoints>& points, const f32 atDistance);
      
      void AddOccluder(const std::vector<Point2f>& points, const f32 atDistance);
      
      //void AddOccluder(const ObservableObject* object, const Camera& camera);
      
      bool IsOccluded(const Quad2f& quad, const f32 atDistance)   const;
      bool IsOccluded(const Point2f& point, const f32 atDistance) const;
      
      bool IsAnythingBehind(const Point2f& point, const f32 atDistance) const;
      bool IsAnythingBehind(const Quad2f&  quad,  const f32 atDistance) const;
      
      void Clear();
      
      bool IsEmpty() const;
      
      // Populates the given vector with all rectangular occluders in the list.
      // It is the caller's responsibility to empty the vector if desired
      void GetAllOccluders(std::vector<Rectangle<f32> >& occluders) const;
      
    protected:
      //std::priority_queue<std::pair<Quad2f,f32>, CompareSecond<Quad2f,f32>() > quadDepthPairs_;
      
      //std::vector<std::pair<Rectangle<f32>,f32> > rectDepthPairs_;
      
      std::map<f32, Rectangle<f32> > rectDepthPairs_;
      
      //bool isSorted_;
      
      template<class PointContainer>
      void AddOccluderHelper(const PointContainer& points, const f32 atDistance);
      
    }; // class OccluderList
    
    
    inline bool OccluderList::IsEmpty() const {
      return rectDepthPairs_.empty();
    }
    
    
  } // namespace Vision
  
} // namespace Anki

#endif // ANKI_CORETECH_VISION_OCCLUDERLIST_H
