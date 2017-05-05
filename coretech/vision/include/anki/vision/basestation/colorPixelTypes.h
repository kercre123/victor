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

#include "anki/common/types.h"
#include <opencv2/core.hpp>

namespace Anki {
namespace Vision {
  
  template<typename Type>
  class PixelRGB_ : public cv::Vec<Type,3>
  {
  public:
    
    PixelRGB_(Type r, Type g, Type b) : cv::Vec<Type,3>(r,g,b) { }
    PixelRGB_(Type value = Type(0)) : PixelRGB_<Type>(value, value, value) { }
    
    // Const accessors
    Type  r() const { return this->operator[](0); }
    Type  g() const { return this->operator[](1); }
    Type  b() const { return this->operator[](2); }
    
    // Non-const accessors
    Type& r() { return this->operator[](0); }
    Type& g() { return this->operator[](1); }
    Type& b() { return this->operator[](2); }
    
    // Convert to gray
    Type gray() const;
    
    // Return min/max across channels
    Type max() const;
    Type min() const;
    
    // Return true if all channels are > or < than given value.
    // If "any" is set to true, then returns true if any channel is > or <.
    bool IsBrighterThan(Type value, bool any = false) const;
    bool IsDarkerThan(Type value, bool any = false) const;
    
    PixelRGB_<Type>& AlphaBlendWith(const PixelRGB_<Type>& other, f32 alpha);
    
  }; // class PixelRGB_
  
  static_assert(sizeof(PixelRGB_<u8>)==3, "PixelRGB not 3 bytes!");
  static_assert(sizeof(PixelRGB_<f32>) == 3*sizeof(f32), "PixelRGB<f32> is not 3X sizeof float");
  
  using PixelRGB = PixelRGB_<u8>;
  
  
  class PixelRGBA : public cv::Vec4b
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
      u16 gray = static_cast<u16>(r() + (g() << 1) + b()); // give green double weight
      gray = gray >> 2; // divide by 4
      assert(gray <= std::numeric_limits<u8>::max());
      return static_cast<u8>(gray);
    }
    
    // Return true if all channels are > or < than given value.
    // If "any" is set to true, then returns true if any channel is > or <.
    // NOTE: Ignores alpha channel!
    bool IsBrighterThan(u8 value, bool any = false) const;
    bool IsDarkerThan(u8 value, bool any = false) const;
    
  }; // class PixelRGBA
  
  static_assert(sizeof(PixelRGBA)==4, "PixelRGBA not 4 bytes!");
  
  
  //
  // Inlined implementations
  //
  
  template<typename Type>
  inline bool PixelRGB_<Type>::IsBrighterThan(Type value, bool any) const {
    if(any) {
      return r() > value || g() > value || b() > value;
    } else {
      return r() > value && g() > value && b() > value;
    }
  }
  
  template<typename Type>
  inline bool PixelRGB_<Type>::IsDarkerThan(Type value, bool any) const {
    if(any) {
      return r() < value || g() < value || b() < value;
    } else {
      return r() < value && g() < value && b() < value;
    }
  }

  inline bool PixelRGBA::IsBrighterThan(u8 value, bool any) const {
    if(any) {
      return r() > value || g() > value || b() > value;
    } else {
      return r() > value && g() > value && b() > value;
    }
  }
  
  inline bool PixelRGBA::IsDarkerThan(u8 value, bool any) const {
    if(any) {
      return r() < value || g() < value || b() < value;
    } else {
      return r() < value && g() < value && b() < value;
    }
  }

  template<typename Type>
  inline PixelRGB_<Type>& PixelRGB_<Type>::AlphaBlendWith(const PixelRGB_<Type>& other, f32 alpha)
  {
    r() = static_cast<u8>(alpha*static_cast<f32>(r()) + (1.f-alpha)*static_cast<f32>(other.r()));
    g() = static_cast<u8>(alpha*static_cast<f32>(g()) + (1.f-alpha)*static_cast<f32>(other.g()));
    b() = static_cast<u8>(alpha*static_cast<f32>(b()) + (1.f-alpha)*static_cast<f32>(other.b()));
    return *this;
  }
  
  template<>
  inline u8 PixelRGB_<u8>::gray() const {
    u16 gray = static_cast<u16>(r() + (g() << 1) + b()); // give green double weight
    gray = gray >> 2; // divide by 4
    assert(gray <= std::numeric_limits<u8>::max());
    return static_cast<u8>(gray);
  }
 
  template<>
  inline f32 PixelRGB_<f32>::gray() const {
    f32 gray = r() + g()*0.5f + b(); // give green double weight
    gray *= 0.25f;
    return gray;
  }
  
  template<typename Type>
  inline Type PixelRGB_<Type>::max() const
  {
    return std::max(r(), std::max(g(), b()));
  }
  
  template<typename Type>
  inline Type PixelRGB_<Type>::min() const
  {
    return std::min(r(), std::min(g(), b()));
  }
  
} // namespace Vision
} // namespace Anki


// "Register" our RGB/RGBA pixels as DataTypes with OpenCV
namespace cv {

  template<> class DataDepth<Anki::Vision::PixelRGB_<u8>  > : public DataDepth<DataType<Vec<u8, 3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<s16> > : public DataDepth<DataType<Vec<s16,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<f32> > : public DataDepth<DataType<Vec<f32,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGBA>       : public DataDepth<DataType<Vec<u8, 4> > >  { };

  template<> class DataType<Anki::Vision::PixelRGB_<u8>  > : public DataType<Vec<u8, 3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<s16> > : public DataType<Vec<s16,3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<f32> > : public DataType<Vec<f32,3> > { };
  template<> class DataType<Anki::Vision::PixelRGBA>       : public DataType<Vec<u8, 4> > { };

} // namespace cv

#endif // __Anki_Coretech_Vision_Basestation_RGBPixel_H__
