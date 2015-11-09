/**
 * File: rgbPixel.h
 *
 * Author: Andrew Stein
 * Date:   11/9/2015
 *
 * Description: Defines an RGB pixel for color images and sets it up to be
 *              understandable/usable by OpenCV.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Coretech_Vision_Basestation_RGBPixel_H__
#define __Anki_Coretech_Vision_Basestation_RGBPixel_H__

#include <opencv2/core/core.hpp>

namespace Anki {
namespace Vision {
  
  class RGBPixel : private cv::Vec3b
  {
  public:
    
    RGBPixel(u8 r, u8 g, u8 b) : cv::Vec3b(r,g,b) { }
    RGBPixel() : RGBPixel(0,0,0) { }
    
    // Const accessors
    u8  r() const { return this->operator[](0); }
    u8  b() const { return this->operator[](1); }
    u8  g() const { return this->operator[](2); }
    
    // Non-const accessors
    u8& r() { return this->operator[](0); }
    u8& b() { return this->operator[](1); }
    u8& g() { return this->operator[](2); }
    
    // Convert to gray
    u8 gray() const {
      u16 gray = r() + (g() << 1) + b(); // give gray double weight
      gray = gray >> 2; // divide by 4
      assert(gray <= u8_MAX);
      return static_cast<u8>(gray);
    }
    
  }; // struct RGBPixel
  
  static_assert(sizeof(RGBPixel)==3, "RGB struct not 3 bytes!");
  
} // namespace Vision
} // namespace Anki

// "Register" our RGBPixel struct as a 3-channel u8 DataType with OpenCV
namespace cv {
  template<> class cv::DataType<Anki::Vision::RGBPixel>
  {
  public:
    typedef u8  value_type;
    typedef s32 work_type;
    typedef u8  channel_type;
    enum { channels = 3, fmt='u', type = CV_8UC3 };
  };
} // namespace cv

#endif // __Anki_Coretech_Vision_Basestation_RGBPixel_H__
