//
//  triangle_impl.h
//  CoreTech Common Basestation
//
//  Created by Andrew Stein on 5/28/14.
//
//

#ifndef CORETECH_ENGINE_MATH_TRIANGLE_IMPL_H
#define CORETECH_ENGINE_MATH_TRIANGLE_IMPL_H


#include "coretech/common/engine/math/triangle.h"


namespace Anki {
  
  template<typename T>
  Triangle<T>::Triangle(const Point2<T>& point1,
                        const Point2<T>& point2,
                        const Point2<T>& point3)
  : std::array<Point<2,T>,3>{{point1, point2, point3}}
  {
    
  }
  
  
  template<typename T>
  Triangle<T>::Triangle(std::initializer_list<Point2<T> >& points)
  : std::array<Point2<T>,3>(points)
  {
    
  }
  
  template<typename T>
  bool Triangle<T>::Contains(const Point<2,T>& point) const
  {
    return IsPointWithinTriangle(point, *this);
  }
  
  template<typename T>
  T Triangle<T>::GetArea() const
  {
    // Area is half the length of the cross product of the vectors pointing
    // from one of the points to the other two
    
    const T x1 = (*this)[1].x() - (*this)[0].x();
    const T y1 = (*this)[1].y() - (*this)[0].y();
    
    const T x2 = (*this)[2].x() - (*this)[0].x();
    const T y2 = (*this)[2].y() - (*this)[0].y();
    
    const T area = std::abs(x1*y2 - x2*y1) / T(2);
    
    return area;
  }
  
  
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
                             const std::array<Point<2,T>, 3>& triangle)
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
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<typename T>
  Point<2, T> Triangle<T>::GetCentroid() const
  {
    // pick any 2 edges and find the middle point
    const Point<2, T>& midPoint = ((*this)[0] + (*this)[1]) * 0.5f;
    const Point<2, T>& oppositePointTowardsMidPoint = midPoint - (*this)[2];
    Point<2, T> centroid = (*this)[2] + oppositePointTowardsMidPoint * 0.666f; // it's 2/3 away from the point along the median
    return centroid;
  }
  
} // namespace Anki

#endif // CORETECH_ENGINE_MATH_TRIANGLE_IMPL_H
