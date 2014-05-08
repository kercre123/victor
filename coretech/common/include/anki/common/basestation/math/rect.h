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
    
    using cv::Rect_<T>::area;
    
    Rectangle<T> Intersect(const Rectangle<T>& other) const;
    
    bool Contains(const Point<2,T>& point) const;
    
#if ANKICORETECH_USE_OPENCV
    Rectangle(const cv::Rect_<T>& cvRect);
    const cv::Rect_<T>& get_CvRect_() const;
    cv::Rect_<T>& get_CvRect_();
#endif
    
  protected:
    
    template<class PointContainer>
    void initFromPointContainer(const PointContainer& points);
    
    using cv::Rect_<T>::x;
    using cv::Rect_<T>::y;
    using cv::Rect_<T>::width;
    using cv::Rect_<T>::height;
    
  }; // class Rectangle
  
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
#endif
  
  template<typename T>
  template<class PointContainer>
  void Rectangle<T>::initFromPointContainer(const PointContainer& points)
  {
    const size_t N = points.size();
    if(N > 0) {
      
      auto pointIter = points.begin();
      
      T xmax(pointIter->x()), ymax(pointIter->y());
      
      this->x = pointIter->x();
      this->y = pointIter->y();
      
      ++pointIter;
      while(pointIter != points.end()) {
        this->x = std::min(this->x, pointIter->x());
        this->y = std::min(this->y, pointIter->y());
        xmax = std::max(xmax, pointIter->x());
        ymax = std::max(ymax, pointIter->y());
        
        ++pointIter;
      }
      
      this->width  = xmax - this->x;
      this->height = ymax - this->y;
      
    } else {
      this->x = 0;
      this->y = 0;
      this->width = 0;
      this->height = 0;
    }
  } // Rectangle<T>::initFromPointContainer
  
  template<typename T>
  Rectangle<T>::Rectangle(const Quadrilateral<2,T>& quad)
  {
    initFromPointContainer(quad);
  }
  
  template<typename T>
  Rectangle<T>::Rectangle(const std::vector<Point<2,T> >& points)
  {
    initFromPointContainer(points);
  }
  
  template<typename T>
  template<size_t NumPoints>
  Rectangle<T>::Rectangle(const std::array<Point<2,T>,NumPoints>& points)
  {
    initFromPointContainer<std::array<Point<2,T>,NumPoints> >(points);
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
  bool Rectangle<T>::Contains(const Point<2,T>& point) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->contains(point.get_CvPoint_());
#else
    CORETECH_THROW("Rectangle::Contains() currently relies on OpenCV.");
#endif
  }
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_RECT_H_
