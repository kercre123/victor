#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    Point_u8::Point_u8()
      : x(u8(0)), y(u8(0))
    {
    }

    Point_u8::Point_u8(const u8 x, const u8 y)
      : x(x), y(y)
    {
    }

    Point_u8::Point_u8(const Point_u8& pt)
      : x(pt.x), y(pt.y)
    {
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    Point_u8::Point_u8(const cv::Point_<u8>& pt)
      : x(pt.x), y(pt.y)
    {
    }

    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Array2dUnmanaged. No data is copied.
    cv::Point_<u8> Point_u8::get_CvPoint_()
    {
      return cv::Point_<u8>(x,y);
    }
#endif

    bool Point_u8::operator== (const Point_u8 &point2) const
    {
      if(this->x == point2.x && this->y == point2.y)
        return true;

      return false;
    }

    Point_u8 Point_u8::operator+ (const Point_u8 &point2) const
    {
      return Point_u8(this->x+point2.x, this->y+point2.y);
    }

    Point_u8 Point_u8::operator- (const Point_u8 &point2) const
    {
      return Point_u8(this->x-point2.x, this->y-point2.y);
    }

    void Point_u8::operator*=(const u8 value)
    {
      this->x *= value;
      this->y *= value;
    }
  } // namespace Embedded
} // namespace Anki