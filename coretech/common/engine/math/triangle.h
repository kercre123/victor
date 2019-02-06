//
//  triangle.h
//  Cozmo
//
//  Created by Andrew Stein on 5/28/14.
//
//

#ifndef CORETECH_ENGINE_MATH_TRIANGLE_H
#define CORETECH_ENGINE_MATH_TRIANGLE_H

#include "coretech/common/shared/math/point_fwd.h"
#include "coretech/common/shared/types.h"

namespace Anki {
  
  // A class for storing 2D triangles
  template<typename T>
  class Triangle : public std::array<Point<2,T>, 3>
  {
  public:
    
    Triangle() { }
    
    Triangle(const Point2<T>& point1,
             const Point2<T>& point2,
             const Point2<T>& point3);
    
    Triangle(std::initializer_list<Point2<T> >& points);
    
    bool Contains(const Point<2,T>& point) const;
    
    T GetArea() const;
    
    // Calculates centroid of the triangle (barycenter), which always divides each median in a 2:1 ratio, or in
    // other words, it's always 2/3 away from each vertex
    Point<2, T> GetCentroid() const;
    
  }; // class Triangle
  
  
  // Each method returns true if the given point is within the given triangle,
  // and false otherwise.  The only difference is whether the triangle is
  // given as three 2D Points or three _pointers_ to 2D points.
  
  template<typename T>
  bool IsPointWithinTriangle(const Point<2,T>&                      point,
                             const std::array<const Point<2,T>*,3>& triangle);
  
  
  template<typename T>
  bool IsPointWithinTriangle(const Point<2,T>&                point,
                             const std::array<Point<2,T>, 3>& triangle);


// common typedefs
using Triangle2f = Triangle<f32>;

} // namespace Anki


#endif // CORETECH_ENGINE_MATH_TRIANGLE_H
