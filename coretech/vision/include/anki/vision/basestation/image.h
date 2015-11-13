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

#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/colorPixelTypes.h"

namespace Anki {
namespace Vision {

  template<typename T>
  class ImageBase : public Array2d<T>
  {
  public:
    ImageBase() : Array2d<T>() { }
    ImageBase(s32 nrows, s32 ncols) : Array2d<T>(nrows, ncols) { }
    ImageBase(s32 nrows, s32 ncols, T* data) : Array2d<T>(nrows, ncols, data) { }
    
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

    // Resize in place by scaleFactor
    void Resize(f32 scaleFactor);
    
    // Resize in place to a specific size
    void Resize(s32 desiredRows, s32 desiredCols);
    
    // Resize into the new image (which is already the desired size)
    void Resize(ImageBase<T>& resizedImage) const;
    
    void DrawLine(const Point2f& start, const Point2f& end,
                  const ColorRGBA& color, const s32 thickness);
    
    void DrawPoint(const Point2f& point, const ColorRGBA& color, const s32 size);

    using Array2d<T>::GetDataPointer;
    using Array2d<T>::IsEmpty;
    using Array2d<T>::GetNumRows;
    using Array2d<T>::GetNumCols;
    
    virtual s32 GetNumChannels() const = 0;
    
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
    
    // Wrap image "header" around given data pointer: no allocation.
    Image(s32 nrows, s32 ncols, u8* data);
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<u8>
    Image(cv::Mat_<u8>& cvMat);
    using Array2d<u8>::get_CvMat_;
#   endif

    s32 GetConnectedComponents(Array2d<s32>& labelImage,
                               std::vector<std::vector< Point2<s32> > >& regionPoints) const;
    
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
    
    // No allocation, just wraps header around given data.
    // Assumes data is nrows*ncols*3 in length!
    ImageRGB(s32 nrows, s32 ncols, u8* data);
    
    // Removes alpha channel and squeezes into 24bpp
    ImageRGB(const ImageRGBA& imageRGBA);
    
    // Replicates grayscale image across all three channels
    ImageRGB(const Image& imageGray);
    
    Image ToGray() const;
    
    virtual s32 GetNumChannels() const override { return 4; }
    
  }; // class ImageRGB
  
  
  // RGBA Color image (i.e. RGB + alpha channel), 32bpp
  class ImageRGBA : public ImageBase<PixelRGBA>
  {
  public:
    ImageRGBA();
    ImageRGBA(s32 nrows, s32 ncols); // allocates
    
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
    
    virtual s32 GetNumChannels() const override { return 3; }
    
  }; // class ImageRGBA

  
  //
  // Inlined implemenations
  //
  template<typename T>
  inline void ImageBase<T>::SetTimestamp(TimeStamp_t ts) {
    _timeStamp = ts;
  }
  
  template<typename T>
  inline TimeStamp_t ImageBase<T>::GetTimestamp() const {
    return _timeStamp;
  }
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Coretech_Vision_Basestation_Image_H__
