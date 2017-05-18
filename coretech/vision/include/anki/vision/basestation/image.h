/**
 * File: image.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Defines a container for images on the basestation.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Anki_Coretech_Vision_Basestation_Image_H__
#define __Anki_Coretech_Vision_Basestation_Image_H__

#include "anki/common/types.h"

#include "anki/common/basestation/array2d.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/quad.h"
#include "anki/common/basestation/math/rect.h"

#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/colorPixelTypes.h"

#include <string>
#include <vector>

namespace Anki {
namespace Vision {
  
  enum class ResizeMethod : u8 {
    NearestNeighbor = 0,
    Linear,
    Cubic,
    AverageArea
  };

  template<typename T>
  class ImageBase : public Array2d<T>
  {
  public:
    ImageBase() : Array2d<T>() { }
    ImageBase(s32 nrows, s32 ncols) : Array2d<T>(nrows, ncols) { }
    ImageBase(s32 nrows, s32 ncols, const T& pixel) : Array2d<T>(nrows, ncols, pixel) { }
    ImageBase(s32 nrows, s32 ncols, const ColorRGBA& color);
    ImageBase(s32 nrows, s32 ncols, T* data) : Array2d<T>(nrows, ncols, data) { }
    ImageBase(const Array2d<T>& array) : Array2d<T>(array) { }
    
    // Read from file
    Result Load(const std::string& filename);
    
    // Write to a file, format determined by extension (quality is only used for JPEG)
    Result Save(const std::string& filename, s32 quality = 90);
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<T>
    ImageBase(cv::Mat_<T>& cvMat) : Array2d<T>(cvMat) { }
#   endif
    
    // Reference counting assignment (does not copy):
    ImageBase<T>& operator= (const ImageBase<T> &other);
    
    void CopyTo(ImageBase<T>& otherImage) const;
    
    void SetTimestamp(TimeStamp_t ts);
    TimeStamp_t GetTimestamp() const;
    
    void Display(const char *windowName, s32 pauseTime_ms = 5) const;
    
    static void CloseDisplayWindow(const char *windowName);
    static void CloseAllDisplayWindows();

    // Resize in place by scaleFactor
    void Resize(f32 scaleFactor, ResizeMethod method = ResizeMethod::Linear);
    
    // Resize in place to a specific size
    void Resize(s32 desiredRows, s32 desiredCols, ResizeMethod method = ResizeMethod::Linear);
    
    // Resize into the new image (which is already the desired size)
    void Resize(ImageBase<T>& resizedImage, ResizeMethod method = ResizeMethod::Linear) const;
    
    //
    // Draw operations expect image coordinates in pixels as <X,Y> NOT <row,col>.
    // Note <0,0> is top left corner, NOT bottom left corner.
    //
    
    // Draw line segment from start <X,Y> to end <X,Y>
    void DrawLine(const Point2f& start, const Point2f& end, const ColorRGBA& color, const s32 thickness = 1);
    
    // Draw circle at center <X,Y> with given radius and line thickness
    void DrawCircle(const Point2f& center, const ColorRGBA& color, const s32 radius, const s32 thickness = 1);
    
    // Draw filled circle at center <X,Y> with given radius
    void DrawFilledCircle(const Point2f& center, const ColorRGBA& color, const s32 radius);
    
    // Draw rectangle from top left <X,Y> to bottom right <X+width,Y+height>
    void DrawRect(const Rectangle<f32>& rect, const ColorRGBA& color, const s32 thickness = 1);
    
    // Draw filled rectangle from top left <X,Y> to bottom right <X+width,Y+height>
    void DrawFilledRect(const Rectangle<f32>& rect, const ColorRGBA& color);
    
    // Draw quadrangle defined by four given points
    void DrawQuad(const Quad2f& quad, const ColorRGBA& color, const s32 thickness = 1);
    
    // TODO: Expose font?
    void DrawText(const Point2f& position, const std::string& str, const ColorRGBA& color, f32 scale = 1.f, bool dropShadow = false);
    
    using Array2d<T>::GetDataPointer;
    using Array2d<T>::IsEmpty;
    using Array2d<T>::GetNumRows;
    using Array2d<T>::GetNumCols;
    
    virtual s32 GetNumChannels() const = 0;
    
  protected:
    template<typename DerivedType>
    DerivedType GetROI(Rectangle<s32>& roiRect);
    
    template<typename DerivedType>
    const DerivedType GetROI(Rectangle<s32>& roiRect) const;

  private:
    TimeStamp_t     _timeStamp;
    
  }; // class ImageBase
  
  
  // Grayscale (i.e. scalar-valued) image, 8bpp
  class Image : public ImageBase<u8>
  {
  public:
    
    // Create an empty image
    Image();
    
    // Allocate a new image
    Image(s32 nrows, s32 ncols);
    Image(s32 nrows, s32 ncols, const u8& pixel);
    Image(s32 nrows, s32 ncols, const ColorRGBA& color);
    
    // Wrap image "header" around given data pointer: no allocation.
    Image(s32 nrows, s32 ncols, u8* data);
    
    Image(const Array2d<u8>& array2d);
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<u8>
    Image(cv::Mat_<u8>& cvMat);
    using Array2d<u8>::get_CvMat_;
#   endif

    Image GetROI(Rectangle<s32>& roiRect) { return ImageBase<u8>::GetROI<Image>(roiRect); }
    const Image GetROI(Rectangle<s32>& roiRect) const { return ImageBase<u8>::GetROI<Image>(roiRect); }
    
    // Reference counting assignment (does not copy):
    Image& operator= (const ImageBase<u8> &other);
    
    // Sets all pixels > value to 255 and all values <= value to 0.
    Image& Threshold(u8 value);
    Image  Threshold(u8 value) const;
    
    s32 GetConnectedComponents(Array2d<s32>& labelImage) const;
    
    struct ConnectedComponentStats
    {
      size_t          area;
      Point2f         centroid;
      Rectangle<s32>  boundingBox;
    };
    
    s32 GetConnectedComponents(Array2d<s32>& labelImage, std::vector<ConnectedComponentStats>& stats) const;
    
    // Get image negatives (i.e. invert black-on-white to white-on-black)
    Image& Negate();
    Image  GetNegative() const;
    
    virtual s32 GetNumChannels() const override { return 1; }
    
  }; // class Image
  
  
  // Forward declaration:
  class ImageRGBA;
  
  // RGB Color image, 24bpp
  class ImageRGB : public ImageBase<PixelRGB>
  {
  public:
    ImageRGB();
    ImageRGB(s32 nrows, s32 ncols); // allocates
    ImageRGB(const ImageBase<PixelRGB>& imageBase) : ImageBase<PixelRGB>(imageBase) { }
    
    // No allocation, just wraps header around given data.
    // Assumes data is nrows*ncols*3 in length!
    ImageRGB(s32 nrows, s32 ncols, u8* data);
    
    // Removes alpha channel and squeezes into 24bpp
    ImageRGB(const ImageRGBA& imageRGBA);
    
    // Replicates grayscale image across all three channels
    ImageRGB(const Image& imageGray);
    
    ImageRGB(const Array2d<PixelRGB>& array2d) : ImageBase<PixelRGB>(array2d) { }
    
    ImageRGB GetROI(Rectangle<s32>& roiRect) { return ImageBase<PixelRGB>::GetROI<ImageRGB>(roiRect); }
    const ImageRGB GetROI(Rectangle<s32>& roiRect) const { return ImageBase<PixelRGB>::GetROI<ImageRGB>(roiRect); }
    
    Image ToGray() const;
    
    // Sets all pixels > value to 255 and all values <= value to 0.
    // If anyChannel=true, then any channel being above the value suffices. Otherwise, all channels must be.
    Image Threshold(u8 value, bool anyChannel) const;
    
    virtual s32 GetNumChannels() const override { return 4; }
    
  }; // class ImageRGB
  
  
  // RGBA Color image (i.e. RGB + alpha channel), 32bpp
  class ImageRGBA : public ImageBase<PixelRGBA>
  {
  public:
    ImageRGBA();
    ImageRGBA(s32 nrows, s32 ncols); // allocates
    ImageRGBA(const ImageBase<PixelRGBA>& imageBase) : ImageBase<PixelRGBA>(imageBase) { }
    
    // No allocation, just wraps a header around given data.
    // Assumes data_32bpp is nrows*ncols in length!
    ImageRGBA(s32 nrows, s32 ncols, u32* data_32bpp);
    
    // Expands 24bpp RGB image into 32bpp RGBA, setting alpha of every pixel
    // to the given value.
    ImageRGBA(const ImageRGB& imageRGB, u8 alpha = 255);
    
#   if ANKICORETECH_USE_OPENCV
    ImageRGBA(cv::Mat& cvMat);
#   endif
    
    Image ToGray() const;
    
    ImageRGBA GetROI(Rectangle<s32>& roiRect) {
      return ImageBase<PixelRGBA>::GetROI<ImageRGBA>(roiRect);
    }
    
    virtual s32 GetNumChannels() const override { return 3; }
    
  }; // class ImageRGBA

  
  //
  // Inlined implementations
  //
  template<typename T>
  inline ImageBase<T>::ImageBase(s32 nrows, s32 ncols, const ColorRGBA& color) : Array2d<T>(nrows, ncols)
  {
    // We can't set elements of Array2d<T> directly because OpenCV does not expose an operator to convert from
    // ColorRGBA or cvScalar to arbitrary pixel type <T>.  Workaround is to draw a filled rectangle that
    // covers the entire image.
    const Rectangle<f32> rect(0, 0, ncols, nrows);
    DrawFilledRect(rect, color);
  }
  
  template<typename T>
  inline void ImageBase<T>::SetTimestamp(TimeStamp_t ts)
  {
    _timeStamp = ts;
  }
  
  template<typename T>
  inline TimeStamp_t ImageBase<T>::GetTimestamp() const
  {
    return _timeStamp;
  }
  
  template<typename T>
  template<typename DerivedType>
  DerivedType ImageBase<T>::GetROI(Rectangle<s32>& roiRect)
  {
    DerivedType roi(Array2d<T>::GetROI(roiRect));
    roi.SetTimestamp(GetTimestamp());
    return roi;
  }
  
  template<typename T>
  template<typename DerivedType>
  const DerivedType ImageBase<T>::GetROI(Rectangle<s32>& roiRect) const
  {
    DerivedType roi(Array2d<T>::GetROI(roiRect));
    roi.SetTimestamp(GetTimestamp());
    return roi;
  }
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Coretech_Vision_Basestation_Image_H__
