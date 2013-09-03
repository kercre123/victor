#ifndef _ANKICORETECHEMBEDDED_COMMON_POINT_H_
#define _ANKICORETECHEMBEDDED_COMMON_POINT_H_

//#include <cmath>

#include "anki/embeddedCommon/config.h"

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
namespace cv
{
  template<typename _Tp> class Point_;
  template<typename _Tp> class Point3_;
}
#endif

namespace Anki
{
  namespace Embedded
  {
    // 2D Point Class:
    class Point_u8
    {
    public:
      Point_u8();

      Point_u8(const u8 x, const u8 y);

      Point_u8(const Point_u8& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_u8(const cv::Point_<u8>& pt);

      cv::Point_<u8> get_CvPoint_();
#endif

      bool operator== (const Point_u8 &point2) const;

      Point_u8 operator+ (const Point_u8 &point2) const;

      Point_u8 operator- (const Point_u8 &point2) const;

      void operator*=(const u8 value);

      u8 x, y;
    }; // class Point_u8<T>

    // 2D Point Class:
    class Point_s8
    {
    public:
      Point_s8();

      Point_s8(const s8 x, const s8 y);

      Point_s8(const Point_s8& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_s8(const cv::Point_<s8>& pt);

      cv::Point_<s8> get_CvPoint_();
#endif

      bool operator== (const Point_s8 &point2) const;

      Point_s8 operator+ (const Point_s8 &point2) const;

      Point_s8 operator- (const Point_s8 &point2) const;

      void operator*=(const s8 value);

      s8 x, y;
    }; // class Point_s8<T>
    // 2D Point Class:
    class Point_u16
    {
    public:
      Point_u16();

      Point_u16(const u16 x, const u16 y);

      Point_u16(const Point_u16& pt) ;

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_u16(const cv::Point_<u16>& pt);

      cv::Point_<u16> get_CvPoint_();
#endif

      bool operator== (const Point_u16 &point2) const;

      Point_u16 operator+ (const Point_u16 &point2) const;

      Point_u16 operator- (const Point_u16 &point2) const;

      void operator*=(const u16 value);

      u16 x, y;
    }; // class Point_u16<T>

    // 2D Point Class:
    class Point_s16
    {
    public:
      Point_s16();

      Point_s16(const s16 x, const s16 y);

      Point_s16(const Point_s16& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_s16(const cv::Point_<s16>& pt);

      cv::Point_<s16> get_CvPoint_();
#endif

      bool operator== (const Point_s16 &point2) const;

      Point_s16 operator+ (const Point_s16 &point2) const;

      Point_s16 operator- (const Point_s16 &point2) const;

      void operator*=(const s16 value);

      s16 x, y;
    }; // class Point_s16<T>

    // 2D Point Class:
    class Point_u32
    {
    public:
      Point_u32();

      Point_u32(const u32 x, const u32 y);

      Point_u32(const Point_u32& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_u32(const cv::Point_<u32>& pt);

      cv::Point_<u32> get_CvPoint_();
#endif

      bool operator== (const Point_u32 &point2) const;

      Point_u32 operator+ (const Point_u32 &point2) const;

      Point_u32 operator- (const Point_u32 &point2) const;

      void operator*=(const u32 value);

      u32 x, y;
    }; // class Point_u32<T>

    // 2D Point Class:
    class Point_s32
    {
    public:
      Point_s32();

      Point_s32(const s32 x, const s32 y);

      Point_s32(const Point_s32& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_s32(const cv::Point_<s32>& pt);

      cv::Point_<s32> get_CvPoint_();
#endif

      bool operator== (const Point_s32 &point2) const;

      Point_s32 operator+ (const Point_s32 &point2) const;

      Point_s32 operator- (const Point_s32 &point2) const;

      void operator*=(const s32 value);

      s32 x, y;
    }; // class Point_s32<T>

    // 2D Point Class:
    class Point_f32
    {
    public:
      Point_f32();

      Point_f32(const f32 x, const f32 y);

      Point_f32(const Point_f32& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_f32(const cv::Point_<f32>& pt);

      cv::Point_<f32> get_CvPoint_();
#endif

      bool operator== (const Point_f32 &point2) const;

      Point_f32 operator+ (const Point_f32 &point2) const;

      Point_f32 operator- (const Point_f32 &point2) const;

      void operator*=(const f32 value);

      f32 x, y;
    }; // class Point_f32<T>

    // 2D Point Class:
    class Point_f64
    {
    public:
      Point_f64();

      Point_f64(const f64 x, const f64 y);

      Point_f64(const Point_f64& pt);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      Point_f64(const cv::Point_<f64>& pt);

      cv::Point_<f64> get_CvPoint_();
#endif

      bool operator== (const Point_f64 &point2) const;

      Point_f64 operator+ (const Point_f64 &point2) const;

      Point_f64 operator- (const Point_f64 &point2) const;

      void operator*=(const f64 value);

      f64 x, y;
    }; // class Point_f64<T>
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_H_
