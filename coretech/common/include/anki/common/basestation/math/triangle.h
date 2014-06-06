//
//  triangle.h
//  Cozmo
//
//  Created by Andrew Stein on 5/28/14.
//
//

#ifndef CORETECH_BASESTATION_MATH_TRIANGLE_H
#define CORETECH_BASESTATION_MATH_TRIANGLE_H

#include "anki/common/basestation/math/point.h"

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


} // namespace Anki

#endif // CORETECH_BASESTATION_MATH_TRIANGLE_H
