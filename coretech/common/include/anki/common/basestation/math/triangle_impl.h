//
//  triangle_impl.h
//  CoreTech Common Basestation
//
//  Created by Andrew Stein on 5/28/14.
//
//

#ifndef CORETECH_BASESTATION_MATH_TRIANGLE_IMPL_H
#define CORETECH_BASESTATION_MATH_TRIANGLE_IMPL_H


#include "anki/common/basestation/math/triangle.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
  
  template<typename T>
  bool IsPointWithinTriangleHelper(const Point<2,T>& point,
                                   const f32 x1, const f32 y1,
                                   const f32 x2, const f32 y2,
                                   const f32 x3, const f32 y3)
  {
    const f32 ZERO = -std::numeric_limits<f32>::epsilon();
    const f32 ONE  = 1.f + std::numeric_limits<f32>::epsilon();
    
    // This test uses the barycentric coordinates of the point. See here
    // for more information:
    //
    //  http://en.wikipedia.org/wiki/Barycentric_coordinate_system
    
    const f32 xdiff = static_cast<f32>(point.x()) - x3;
    const f32 ydiff = static_cast<f32>(point.y()) - y3;
    const f32 divisor = 1.f / ((y2-y3)*(x1-x3) + (x3-x2)*(y1-y3)); // == 1 / determinant
    
    const f32 lambda1 = ((y2-y3)*xdiff + (x3-x2)*ydiff) * divisor;
    
    if(lambda1 > ZERO && lambda1 < ONE) {
      const f32 lambda2 = ((y3-y1)*xdiff + (x1-x3)*ydiff) * divisor;
      
      if(lambda2 > ZERO && lambda2 < ONE) {
        const f32 lambda3 = 1.f - lambda1 - lambda2;
        
        if(lambda3 > ZERO && lambda3 < ONE) {
          // All three barycentric coordinates are between 0 and 1, so this
          // point must be within the triangle
          return true;
        }
      }
    }
    
    // Point not within triangle
    return false;
    
  } // IsPointWithinTriangleHelper()
  

  // Triangle given as array of 3 pointers to 2D points
  template<typename T>
  bool IsPointWithinTriangle(const Point<2,T>&                      point,
                             const std::array<const Point<2,T>*,3>& triangle)
  {
    const f32 x1 = static_cast<f32>(triangle[0]->x());
    const f32 y1 = static_cast<f32>(triangle[0]->y());
    const f32 x2 = static_cast<f32>(triangle[1]->x());
    const f32 y2 = static_cast<f32>(triangle[1]->y());
    const f32 x3 = static_cast<f32>(triangle[2]->x());
    const f32 y3 = static_cast<f32>(triangle[2]->y());
    
    const bool isContained = IsPointWithinTriangleHelper(point,
                                                         x1, y1,
                                                         x2, y2,
                                                         x3, y3);
    
    return isContained;
  }
  
  // Triangle given as array of 3 2D points
  template<typename T>
  bool IsPointWithinTriangle(const Point<2,T>&                      point,
                             const std::array<const Point<2,T>, 3>& triangle)
  {
    const f32 x1 = static_cast<f32>(triangle[0].x());
    const f32 y1 = static_cast<f32>(triangle[0].y());
    const f32 x2 = static_cast<f32>(triangle[1].x());
    const f32 y2 = static_cast<f32>(triangle[1].y());
    const f32 x3 = static_cast<f32>(triangle[2].x());
    const f32 y3 = static_cast<f32>(triangle[2].y());
    
    const bool isContained = IsPointWithinTriangleHelper(point,
                                                         x1, y1,
                                                         x2, y2,
                                                         x3, y3);
    
    return isContained;
  }
  
  
  
} // namespace Anki

#endif // CORETECH_BASESTATION_MATH_TRIANGLE_IMPL_H
