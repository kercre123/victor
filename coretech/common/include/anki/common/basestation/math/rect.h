/**
 * File: rect.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 5/10/2014
 *
 *
 * Description: Defines a templated class for storing 2D rectangles according to 
 *              their upper left corner and width + height.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_COMMON_RECT_H_
#define _ANKICORETECH_COMMON_RECT_H_

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#include "anki/common/basestation/math/quad.h"

namespace Anki {
  
  // A class for storing axis-aligned 2D rectangles.
  // (x,y) is upper left corner
  // bottom/right boundaries are non inclusive (like OpenCV)
  // so x in [rectangle.x, rectangle.x + rectangle.width) are inside, and
  //    y in [rectangle.y, rectangle.y + rectangle.height) are inside
  // Note the inclusive "[" vs. the non-inclusive ")".
  template<typename T>
  class Rectangle
#if ANKICORETECH_USE_OPENCV
  : private cv::Rect_<T> // private inheritance from cv::Rect
#endif
  {
  public:
    Rectangle();
    Rectangle(T x, T y, T width, T height);
    
    // Construct bounding boxes:
    Rectangle(const Quadrilateral<2,T>& quad);
    Rectangle(const std::vector<Point<2,T> >& points);
    
    template<size_t NumPoints>
    Rectangle(const std::array<Point<2,T>,NumPoints>& points);
    
#if ANKICORETECH_USE_OPENCV
    using cv::Rect_<T>::area;
#else
    const float area() const { return 0.0f; }
#endif
    
    // (x,y) is the top left
    inline T GetX()      const;
    inline T GetY()      const;
    inline T GetWidth()  const;
    inline T GetHeight() const;
    
    // Get x+width or y+height
    inline T GetXmax()   const;
    inline T GetYmax()   const;
    
    void GetQuad(Quadrilateral<2,T>& quad) const;
    
    Rectangle<T> Intersect(const Rectangle<T>& other) const;
    
    bool Contains(const Point<2,T>& point) const;
    
#if ANKICORETECH_USE_OPENCV
    Rectangle(const cv::Rect_<T>& cvRect);
    const cv::Rect_<T>& get_CvRect_() const;
    cv::Rect_<T>& get_CvRect_();
#endif
    
  protected:
    
    template<class PointContainer>
    void InitFromPointContainer(const PointContainer& points);
    
#if ANKICORETECH_USE_OPENCV
    using cv::Rect_<T>::x;
    using cv::Rect_<T>::y;
    using cv::Rect_<T>::width;
    using cv::Rect_<T>::height;
#endif
    
  }; // class Rectangle
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_RECT_H_
