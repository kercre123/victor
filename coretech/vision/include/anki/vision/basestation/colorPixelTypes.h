/**
 * File: rgbPixel.h
 *
 * Author: Andrew Stein
 * Date:   11/9/2015
 *
 * Description: Defines an RGB & RGBA pixels for color images and sets them up to be
 *              understandable/usable by OpenCV.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Coretech_Vision_Basestation_ColorPixelTypes_H__
#define __Anki_Coretech_Vision_Basestation_ColorPixelTypes_H__

#include <opencv2/core/core.hpp>

namespace Anki {
namespace Vision {
  
  class PixelRGB : private cv::Vec3b
  {
  public:
    
    PixelRGB(u8 r, u8 g, u8 b) : cv::Vec3b(r,g,b) { }
    PixelRGB() : PixelRGB(0,0,0) { }
    
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
    
  }; // class PixelRGB
  
  static_assert(sizeof(PixelRGB)==3, "PixelRGB not 3 bytes!");
  
  
  class PixelRGBA : private cv::Vec4b
  {
  public:
    
    PixelRGBA(u8 r, u8 g, u8 b, u8 a) : cv::Vec4b(r,g,b,a) { }
    PixelRGBA() : PixelRGBA(0,0,0,255) { }
    
    PixelRGBA(const PixelRGB& pixelRGB, u8 alpha = 255)
    : PixelRGBA(pixelRGB.r(), pixelRGB.g(), pixelRGB.g(), alpha) { }
    
    // Const accessors
    u8  r() const { return this->operator[](0); }
    u8  b() const { return this->operator[](1); }
    u8  g() const { return this->operator[](2); }
    u8  a() const { return this->operator[](3); }
    
    // Non-const accessors
    u8& r() { return this->operator[](0); }
    u8& b() { return this->operator[](1); }
    u8& g() { return this->operator[](2); }
    u8& a() { return this->operator[](3); }
    
    // Convert to gray
    u8 gray() const {
      u16 gray = r() + (g() << 1) + b(); // give gray double weight
      gray = gray >> 2; // divide by 4
      assert(gray <= u8_MAX);
      return static_cast<u8>(gray);
    }
    
  }; // class PixelRGBA
  
  static_assert(sizeof(PixelRGBA)==4, "PixelRGBA not 4 bytes!");
  
} // namespace Vision
} // namespace Anki


// "Register" our RGB/RGBA pixels as DataTypes with OpenCV
namespace cv {

  template<> class cv::DataType<Anki::Vision::PixelRGB>
  {
  public:
    typedef u8  value_type;
    typedef s32 work_type;
    typedef u8  channel_type;
    enum { channels = 3, fmt='u', type = CV_8UC3 };
  };

  template<> class cv::DataType<Anki::Vision::PixelRGBA>
  {
  public:
    typedef u8  value_type;
    typedef s32 work_type;
    typedef u8  channel_type;
    enum { channels = 4, fmt='u', type = CV_8UC4 };
  };

} // namespace cv

#endif // __Anki_Coretech_Vision_Basestation_RGBPixel_H__
