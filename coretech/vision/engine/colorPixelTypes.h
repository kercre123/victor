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

#include "coretech/common/shared/types.h"
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
  
  class PixelRGB565 : public cv::Vec2b
  {
  public:
    
    PixelRGB565(u16 value = 0) { SetValue(value); }
    
    PixelRGB565(u8 r, u8 g, u8 b)
    {
      SetValue(r, g, b);
    }
    
    PixelRGB565(const PixelRGB& pixRGB)
    : PixelRGB565(pixRGB.r(), pixRGB.g(), pixRGB.b())
    {
      
    }
    
    u8 r() const { return ((Rmask & GetValue()) >> 8); }
    u8 g() const { return ((Gmask & GetValue()) >> 3); }
    u8 b() const { return ((Bmask & GetValue()) << 3); }
    
    inline PixelRGB ToPixelRGB() const;
    
    // Masks and shifts to convert an RGB565 color into a
    // 32-bit BGRA color (i.e. 0xBBGGRRAA), e.g. which webots expects
    inline u32 ToBGRA32(u8 alpha = 0xFF) const;
    // 32-bit RGBA color (i.e. 0xRRGGBBAA)
    inline u32 ToRGBA32(u8 alpha = 0xFF) const;

    inline void SetValue(u16 value){ *((u16*)&(this->operator[](0))) = value; }
    inline void SetValue(u8 r, u8 g, u8 b) { SetValue((u16(0xF8 & r) << 8) | (u16(0xFC & g) << 3) | (u16(0xF8 & b) >> 3)); };
    inline u16 GetValue()    const { return *((u16*)&(this->operator[](0))); }
    inline u8  GetHighByte() const { return this->operator[](1); }
    inline u8  GetLowByte()  const { return this->operator[](0); }

  private:
    
    static constexpr u16 Rmask = 0xF800;
    static constexpr u16 Gmask = 0x07E0;
    static constexpr u16 Bmask = 0x001F;
  };

  
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
  
  
  inline PixelRGB PixelRGB565::ToPixelRGB() const
  {
    return PixelRGB(r(), g(), b());
  }
  
  
  inline u32 PixelRGB565::ToBGRA32(u8 alpha) const
  {
    union cast_t {
      struct {
#if defined(ANDROID)
        u8 b;
        u8 g;
        u8 r;
        u8 a;
#else
        u8 a;
        u8 r;
        u8 g;
        u8 b;
#endif

      };
      u32 result;
    } pixel;

    pixel.r = r();
    pixel.g = g();
    pixel.b = b();
    pixel.a = alpha;
    return pixel.result;
  }
  
  
  inline u32 PixelRGB565::ToRGBA32(u8 alpha) const
  {
    union cast_t {
      struct {
#if defined(ANDROID)
        u8 r;
        u8 g;
        u8 b;
        u8 a;
#else
        u8 b;
        u8 g;
        u8 r;
        u8 a;
#endif
      };
      u32 result;
    } pixel;

    pixel.r = r();
    pixel.g = g();
    pixel.b = b();
    pixel.a = alpha;
    return pixel.result;
  }
} // namespace Vision
} // namespace Anki


// "Register" our RGB/RGBA pixels as DataTypes with OpenCV
namespace cv {

  template<> class DataDepth<Anki::Vision::PixelRGB_<u8>  >     : public DataDepth<DataType<Vec<u8, 3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<s16> >     : public DataDepth<DataType<Vec<s16,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB_<f32> >     : public DataDepth<DataType<Vec<f32,3> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGBA>           : public DataDepth<DataType<Vec<u8, 4> > >  { };
  template<> class DataDepth<Anki::Vision::PixelRGB565>         : public DataDepth<DataType<Vec<u8, 2> > >  { };
  
  template<> class DataType<Anki::Vision::PixelRGB_<u8>  >      : public DataType<Vec<u8, 3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<s16> >      : public DataType<Vec<s16,3> > { };
  template<> class DataType<Anki::Vision::PixelRGB_<f32> >      : public DataType<Vec<f32,3> > { };
  template<> class DataType<Anki::Vision::PixelRGBA>            : public DataType<Vec<u8, 4> > { };
  template<> class DataType<Anki::Vision::PixelRGB565>          : public DataType<Vec<u8, 2> > { };
  
} // namespace cv

#endif // __Anki_Coretech_Vision_Basestation_RGBPixel_H__
