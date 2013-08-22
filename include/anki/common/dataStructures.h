//
//  dataStructures.h
//  CoreTech_Common
//
//  Created by Peter Barnum on 8/7/13.
//  Copyright (c) 2013 Peter Barnum. All rights reserved.
//

#ifndef _ANKICORETECH_COMMON_DATASTRUCTURES_H_
#define _ANKICORETECH_COMMON_DATASTRUCTURES_H_

#include "anki/common/config.h"

#if defined(ANKICORETECH_USE_OPENCV)
namespace cv
{
  template<typename _Tp> class Point_;
  template<typename _Tp> class Point3_;
}
#endif

namespace Anki
{
  // Return values:
  enum Result {
    RESULT_OK = 0,
    RESULT_FAIL = 1,
    RESULT_FAIL_MEMORY = 10000,
    RESULT_FAIL_OUT_OF_MEMORY = 10001,
    RESULT_FAIL_IO = 20000,
    RESULT_FAIL_INVALID_PARAMETERS = 30000
  };

  // 2D Point Class:
  template<typename T> class Point2
  {
  public:
    Point2( void ) : x(T(0)), y(T(0))
    {
    }

    Point2(T x, T y) : x(x), y(y)
    {
    }

    Point2(const Point2& pt) : x(pt.x), y(pt.y)
    {
    }

#if defined(ANKICORETECH_USE_OPENCV)
    Point2(const cv::Point_<T>& pt) : x(pt.x), y(pt.y)
    {
    }

    friend bool operator== (const Point2 &point1, const Point2 &point2)
    {
      if(point1.x == point2.x && point1.y == point2.y)
        return true;

      return false;
    }

    friend Point2 operator+ (const Point2 &point1, const Point2 &point2)
    {
      return Point2(point1.x+point2.x, point1.y+point2.y);
    }

    friend Point2 operator- (const Point2 &point1, const Point2 &point2)
    {
      return Point2(point1.x-point2.x, point1.y-point2.y);
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Array2dUnmanaged. No data is copied.
    cv::Point_<T>& get_CvPoint_()
    {
      return cv::Point_<T>(x,y);
    }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    T x, y;
  }; // class Point2<T>

  typedef Point2<float> Point2f;

  // 3D Point Class:
  template<typename T> class Point3
  {
  public:
    Point3( void ) : x(T(0)), y(T(0)), z(T(0))
    {
    }

    Point3(T x, T y, T z) : x(x), y(y), z(z)
    {
    }

    Point3(const Point3& pt) : x(pt.x), y(pt.y), z(pt.z)
    {
    }

#if defined(ANKICORETECH_USE_OPENCV)
    Point3(const cv::Point3_<T>& pt) : x(pt.x), y(pt.y), z(pt.z)
    {
    }

    friend bool operator== (const Point3 &point1, const Point3 &point2)
    {
      if(point1.x == point2.x && point1.y == point2.y && point1.z == point2.z)
        return true;

      return false;
    }

    friend Point3 operator+ (const Point3 &point1, const Point3 &point2)
    {
      return Point3(point1.x+point2.x, point1.y+point2.y, point1.z+point2.z);
    }

    friend Point3 operator- (const Point3 &point1, const Point3 &point2)
    {
      return Point3(point1.x-point2.x, point1.y-point2.y, point1.z+point2.z);
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Array2dUnmanaged. No data is copied.
    cv::Point3_<T>& get_CvPoint3_()
    {
      return cv::Point3_<T>(x,y,z);
    }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    T x, y, z;
  }; // class Point3<T>

  typedef Point3<float> Point3f;

  // TODO: Do we really need a separate Vec3 class or is it the same as Point3?
  typedef Point3<float> Vec3f;
} // namespace Anki

#endif // #ifndef _ANKICORETECH_COMMON_DATASTRUCTURES_H_
