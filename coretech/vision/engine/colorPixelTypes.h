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
#include "util/math/numericCast.h"
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
    
    // Convert to gray (green simply gets double weight and efficient integer math is used)
    Type gray() const;
    
    // Convert to gray using weights like OpenCV: gray = 0.299*R + 0.587*G + 0.114*B
    //   http://docs.opencv.org/2.4/modules/imgproc/doc/miscellaneous_transformations.html#cvtcolor
    Type weightedGray() const;
    
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
    u8  g() const { return this->operator[](1); }
    u8  b() const { return this->operator[](2); }
    u8  a() const { return this->operator[](3); }
    
    // Non-const accessors
    u8& r() { return this->operator[](0); }
    u8& g() { return this->operator[](1); }
    u8& b() { return this->operator[](2); }
    u8& a() { return this->operator[](3); }
    
    // Convert to gray (see PixelRGB for details)
    u8 gray() const;
    u8 weightedGray() const;
    
    // Return true if all channels are > or < than given value.
    // If "any" is set to true, then returns true if any channel is > or <.
    // NOTE: Ignores alpha channel!
    bool IsBrighterThan(u8 value, bool any = false) const;
    bool IsDarkerThan(u8 value, bool any = false) const;
    
  }; // class PixelRGBA
  
  static_assert(sizeof(PixelRGBA)==4, "PixelRGBA not 4 bytes!");
  
  template<bool SwapBytes>
  class PixelRGB565_
  {
  public:
    
    PixelRGB565_(u16 value = 0) : _value(value) { }
    
    PixelRGB565_(u8 r, u8 g, u8 b)
    : _value( (u16(0xF8 & r) << 8) | (u16(0xFC & g) << 3) | (u16(0xF8 & b) >> 3) )
    {
      _value = SwapBytesHelper(_value);
    }
    
    PixelRGB565_(const PixelRGB& pixRGB)
    : PixelRGB565_(pixRGB.r(), pixRGB.g(), pixRGB.b())
    {
      
    }
    
    u8 r() const;
    u8 g() const;
    u8 b() const;
    
    inline PixelRGB ToPixelRGB() const;
    
    // Masks and shifts to convert an RGB565 color into a
    // 32-bit BGRA color (i.e. 0xBBGGRRAA), e.g. which webots expects
    inline u32 ToBGRA32(u8 alpha = 0xFF) const;
    
  private:
    
    static constexpr u16 Rmask = 0xF800;
    static constexpr u16 Gmask = 0x07E0;
    static constexpr u16 Bmask = 0x001F;
    
    static inline u16 SwapBytesHelper(u16 value);
    
    u16 _value;
  };
  
  using PixelRGB565 = PixelRGB565_<true>; // Default is to swap the bytes
  
  static_assert(sizeof(PixelRGB565)==2,  "PixelRGB565 not 2 bytes!");
  
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
  
  inline u8 SimpleRGB2Gray(u8 r, u8 g, u8 b)
  {
    u16 gray = static_cast<u16>(r + (g << 1) + b); // give green double weight
    gray = gray >> 2; // divide by 4
    return Util::numeric_cast<u8>(gray);
  }
  
  inline f32 WeightedRGB2Gray(f32 r, f32 g, f32 b)
  {
    const f32 gray = 0.299f*r + 0.587f*g + 0.114f*b;
    return gray;
  }
  
  template<>
  inline u8 PixelRGB_<u8>::gray() const {
    return SimpleRGB2Gray(r(), g(), b());
  }
 
  template<>
  inline u8 PixelRGB_<u8>::weightedGray() const {
    return Util::numeric_cast<u8>(WeightedRGB2Gray(r(), g(), b()));
  }
  
  template<>
  inline f32 PixelRGB_<f32>::gray() const {
    f32 gray = r() + g()*2.f + b(); // give green double weight
    gray *= 0.25f;
    return gray;
  }
  
  template<>
  inline f32 PixelRGB_<f32>::weightedGray() const {
    return WeightedRGB2Gray(r(), g(), b());
  }
  
  inline u8 PixelRGBA::gray() const {
    return SimpleRGB2Gray(r(), g(), b());
  }
  
  inline u8 PixelRGBA::weightedGray() const {
    return Util::numeric_cast<u8>(WeightedRGB2Gray(r(), g(), b()));
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
  
  // Swapped and unswapped versions of r,g,b accessors for PixelRGB565:
  
  template<>
  inline u8 PixelRGB565_<true>::r() const {
    // Without first unswapping the bytes, R is in the upper 5 bits of the LSByte
    return (0x00F8 & _value);
  }
  
  template<>
  inline u8 PixelRGB565_<false>::r() const {
    // In unswapped version, R is the uppermost 5 bits. Mask and shift over
    return ((0xF800 & _value) >> 8);
  }
  
  template<>
  inline u8 PixelRGB565_<true>::g() const {
    // Without first unswapping the bytes, part of G comes from each byte.
    return ((0x0007 & _value) << 5) | ((0xE000 & _value) >> 11);
  }
  
  template<>
  inline u8 PixelRGB565_<false>::g() const {
    // In unswapped version, G is the middle 6 bits
    return ((0x07E0 & _value) >> 3);
  }
  
  template<>
  inline u8 PixelRGB565_<true>::b() const {
    // Without first unswapping the bytes, B is in the lower 5 bits of the MSByte
    return ((0x1F00 & _value) >> 5);
  }
  
  template<>
  inline u8 PixelRGB565_<false>::b() const {
    // In the unswapped version, B is the lower 5 bits
    return ((0x001F & _value) << 3);
  }
  
  template<>
  inline u16 PixelRGB565_<true>::SwapBytesHelper(u16 value)
  {
    const u16 swappedBytes = ((value>>8) & 0xFF) | ((value & 0xFF)<<8);
    return swappedBytes;
  }
  
  template<>
  inline u16 PixelRGB565_<false>::SwapBytesHelper(u16 value)
  {
    return value;
  }
  
  template<bool SwapBytes>
  inline PixelRGB PixelRGB565_<SwapBytes>::ToPixelRGB() const
  {
    const u16 value = SwapBytesHelper(_value);
    const PixelRGB pixRGB(((Rmask & value) >> 8),
                          ((Gmask & value) >> 3),
                          ((Bmask & value) << 3));
    return pixRGB;
  }
  
  template<bool SwapBytes>
  inline u32 PixelRGB565_<SwapBytes>::ToBGRA32(u8 alpha) const
  {
    const u16 Rshift = 0;
    const u16 Gshift = 13;
    const u16 Bshift = 27;
    
    const u16 value = SwapBytesHelper(_value);
    
    const u32 color = (((u32) (value & Rmask) << Rshift) |
                       ((u32) (value & Gmask) << Gshift) |
                       ((u32) (value & Bmask) << Bshift) |
                       alpha);
    
    return color;
  }
  
  
} // namespace Vision
} // namespace Anki


// "Register" our RGB/RGBA pixels as DataTypes with OpenCV
namespace cv {

  template<> class DataDepth<Anki::Vision::PixelRGB_<u8>  >     : public DataDepth<DataType<Vec<u8, 3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<s16> >     : public DataDepth<DataType<Vec<s16,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<f32> >     :  public DataDepth<DataType<Vec<f32,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGBA>           : public DataDepth<DataType<Vec<u8, 4> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB565_<true>>  : public DataDepth<DataType<u16> >          { };
  template<> class DataDepth<Anki::Vision::PixelRGB565_<false>> : public DataDepth<DataType<u16> >          { };
  
  template<> class DataType<Anki::Vision::PixelRGB_<u8>  >      : public DataType<Vec<u8, 3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<s16> >      : public DataType<Vec<s16,3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<f32> >      : public DataType<Vec<f32,3> > { };
  template<> class DataType<Anki::Vision::PixelRGBA>            : public DataType<Vec<u8, 4> > { };
  template<> class DataType<Anki::Vision::PixelRGB565_<true>>   : public DataType<u16>         { };
  template<> class DataType<Anki::Vision::PixelRGB565_<false>>  : public DataType<u16>         { };
  
} // namespace cv

#endif // __Anki_Coretech_Vision_Basestation_RGBPixel_H__
