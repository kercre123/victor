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
  //typedef enum Result {RESULT_SUCCESS, RESULT_FAIL};
  enum Result {
    RESULT_OK = 0,
    RESULT_FAIL = 1,
    RESULT_FAIL_MEMORY = 10000,
    RESULT_FAIL_OUT_OF_MEMORY = 10001,
    RESULT_FAIL_IO = 20000,
    RESULT_FAIL_INVALID_PARAMETERS = 30000
  };

  template<typename T> class Point2
  {
  public:
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

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Matrix. No data is copied.
    cv::Point_<T>& get_CvPoint_()
    {
      return cv::Point_<T>(x,y);
    }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    T x, y;
  };

  template<typename T> class Point3
  {
  public:
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

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Matrix. No data is copied.
    cv::Point3_<T>& get_CvPoint3_()
    {
      return cv::Point3_<T>(x,y,z);
    }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    T x, y, z;
  };
} // namespace Anki

#endif // #ifndef _ANKICORETECH_COMMON_DATASTRUCTURES_H_
