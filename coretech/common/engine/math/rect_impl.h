/**
 * File: rect_impl.h
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

#ifndef _ANKICORETECH_COMMON_RECT_IMPL_H_
#define _ANKICORETECH_COMMON_RECT_IMPL_H_

#include "coretech/common/engine/math/rect.h"

#include "coretech/common/engine/math/quad_impl.h"

#include "util/logging/logging.h"

namespace Anki {
  
  template<typename T>
  Rectangle<T>::Rectangle()
#if ANKICORETECH_USE_OPENCV
  : cv::Rect_<T>()
#endif
  {
    
  }
  
  template<typename T>
  Rectangle<T>::Rectangle(T x, T y, T width, T height)
#if ANKICORETECH_USE_OPENCV
  : cv::Rect_<T>(x,y,width,height)
#endif
  {
    DEV_ASSERT(width  >= T(0), "Rectangle.XYWidthHeightConstructor.NegativeWidth");
    DEV_ASSERT(height >= T(0), "Rectangle.XYWidthHeightConstructor.NegativeHeight");
  }
  
  template<typename T>
  Rectangle<T>::Rectangle(const Point2<T>& corner1, const Point2<T>& corner2)
#if ANKICORETECH_USE_OPENCV
  : cv::Rect_<T>(corner1.get_CvPoint_(), corner2.get_CvPoint_())
#endif
  {
    DEV_ASSERT(width  >= T(0), "Rectangle.TwoPointConstructor.NegativeWidth");
    DEV_ASSERT(height >= T(0), "Rectangle.TwoPointConstructor.NegativeHeight");
  }
  
  
#if ANKICORETECH_USE_OPENCV
  template<typename T>
  Rectangle<T>::Rectangle(const cv::Rect_<T>& cvRect)
  : cv::Rect_<T>(cvRect)
  {
    
  }
  
  template<typename T>
  cv::Rect_<T>& Rectangle<T>::get_CvRect_()
  {
    return *this;
  }
  
  template<typename T>
  const cv::Rect_<T>& Rectangle<T>::get_CvRect_() const
  {
    return *this;
  }
  
  template<typename T>
  T Rectangle<T>::GetX()      const
  { return x; }
  
  template<typename T>
  T Rectangle<T>::GetY()      const
  { return y; }
  
  template<typename T>
  T Rectangle<T>::GetWidth()  const
  { return width; }
  
  template<typename T>
  T Rectangle<T>::GetHeight() const
  { return height; }

  template<typename T>
  T Rectangle<T>::GetXmax()   const
  { return x + width; }
  
  template<typename T>
  T Rectangle<T>::GetYmax()   const
  { return y + height; }

  template<typename T>
  T Rectangle<T>::GetXmid()   const
  { return x + width/2; }
  
  template<typename T>
  T Rectangle<T>::GetYmid()   const
  { return y + height/2; }

  template<typename T>
  Point<2,T> Rectangle<T>::GetMidPoint() const
  {
    return Point<2,T>(GetXmid(), GetYmid());
  }
  
  template<typename T>
  void Rectangle<T>::GetQuad(Quadrilateral<2,T>& quad) const
  {
    quad[Quad::TopLeft].x() = x;
    quad[Quad::TopLeft].y() = y;
    
    quad[Quad::BottomLeft].x() = x;
    quad[Quad::BottomLeft].y() = y + height;
    
    quad[Quad::TopRight].x() = x + width;
    quad[Quad::TopRight].y() = y;
    
    quad[Quad::BottomRight].x() = x + width;
    quad[Quad::BottomRight].y() = y + height;
  }
  
  template<typename T>
  Point<2,T> Rectangle<T>::GetTopLeft() const {
    return Point<2,T>(GetX(),GetY());
  }
  
  template<typename T>
  Point<2,T> Rectangle<T>::GetTopRight() const {
    return Point<2,T>(GetXmax(),GetY());
  }
  
  template<typename T>
  Point<2,T> Rectangle<T>::GetBottomLeft() const {
    return Point<2,T>(GetX(),GetYmax());
  }
  
  template<typename T>
  Point<2,T> Rectangle<T>::GetBottomRight() const {
    return Point<2,T>(GetXmax(), GetYmax());
  }

  
#endif
  
  
  template<typename T>
  template<class PointContainer>
  void Rectangle<T>::InitFromPointContainer(const PointContainer& points)
  {
    const size_t N = points.size();
    if(N > 0) {
      
      auto pointIter = points.begin();
      
      T xmax(pointIter->x()), ymax(pointIter->y());
      
#if ANKICORETECH_USE_OPENCV
      this->x = pointIter->x();
      this->y = pointIter->y();
#endif
      
      ++pointIter;
      while(pointIter != points.end()) {
#if ANKICORETECH_USE_OPENCV
        this->x = std::min(this->x, static_cast<T>(pointIter->x()));
        this->y = std::min(this->y, static_cast<T>(pointIter->y()));
#endif
        xmax = std::max(xmax, static_cast<T>(pointIter->x()));
        ymax = std::max(ymax, static_cast<T>(pointIter->y()));
        
        ++pointIter;
      }
      
#if ANKICORETECH_USE_OPENCV
      this->width  = xmax - this->x;
      this->height = ymax - this->y;
#endif
      
    } else {
#if ANKICORETECH_USE_OPENCV
      this->x = 0;
      this->y = 0;
      this->width = 0;
      this->height = 0;
#endif
    }
  } // Rectangle<T>::initFromPointContainer
  
  template<typename T>
  template<typename T_other>
  Rectangle<T>::Rectangle(const Quadrilateral<2,T_other>& quad)
  {
    InitFromPointContainer(quad);
  }
  
  template<typename T>
  template<typename T_other>
  Rectangle<T>::Rectangle(const std::vector<Point<2,T_other> >& points)
  {
    InitFromPointContainer(points);
  }
  
  template<typename T>
  template<size_t NumPoints>
  Rectangle<T>::Rectangle(const std::array<Point<2,T>,NumPoints>& points)
  {
    InitFromPointContainer<std::array<Point<2,T>,NumPoints> >(points);
  }
  
  template<typename T>
  Rectangle<T> Rectangle<T>::Scale(const f32 scaleFactor) const
  {
    return Rectangle<T>(x + static_cast<T>(static_cast<f32>(width)*.5f * (1.f - scaleFactor)),
                        y + static_cast<T>(static_cast<f32>(height)*.5f * (1.f - scaleFactor)),
                        static_cast<T>(scaleFactor*static_cast<f32>(width)),
                        static_cast<T>(scaleFactor*static_cast<f32>(height)));
  }

  template<typename T>
  Rectangle<T> Rectangle<T>::Intersect(const Rectangle<T>& other) const
  {
#if ANKICORETECH_USE_OPENCV
    cv::Rect_<T> temp = this->get_CvRect_() & other.get_CvRect_();
    return Rectangle<T>(temp.x, temp.y, temp.width, temp.height);
#else
    CORETECH_THROW("Rectangle::Intersect() currently relies on OpenCV.");
#endif    
  }
  
  template<typename T>
  f32 Rectangle<T>::ComputeOverlapScore(const Rectangle<T>& other) const
  {
    // Note that union = area1 + area2 - intersection!
    const T thisArea = Area();
    const T otherArea = other.Area();
    const T intersection = Intersect(other).Area();
    if(thisArea==0 || otherArea==0 || intersection==0) {
      return 0.f;
    }
    const f32 denom = static_cast<f32>(thisArea + otherArea - intersection);
    if(denom <= 0.f) {
      PRINT_NAMED_ERROR("Rectangle.ComputeOverlapScore.InvalidDenom",
                        "Area sum minus intersection should be strictly > 0, not %f", denom);
      return 0.f;
    }
    return static_cast<f32>(intersection) / denom;
  }
  
  template<typename T>
  bool Rectangle<T>::Contains(const Point<2,T>& point) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->contains(point.get_CvPoint_());
#else
    CORETECH_THROW("Rectangle::Contains() currently relies on OpenCV.");
#endif
  }
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_RECT_IMPL_H_
