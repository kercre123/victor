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

#include "coretech/common/shared/types.h"

#include "coretech/common/engine/array2d.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/engine/math/rect.h"

#include "coretech/vision/engine/colorPixelTypes.h"

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
    Result Save(const std::string& filename, s32 quality = 90) const;
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<T>
    ImageBase(cv::Mat_<T>& cvMat) : Array2d<T>(cvMat) { }
#   endif
    
    // Reference counting assignment (does not copy):
    ImageBase<T>& operator= (const ImageBase<T> &other);
    
    void CopyTo(ImageBase<T>& otherImage) const;
    
    void SetTimestamp(TimeStamp_t ts);
    TimeStamp_t GetTimestamp() const;

    void SetImageId(u32 imageId);
    u32 GetImageId() const;
    
    void Display(const char *windowName, s32 pauseTime_ms = 5) const;
    
    static void CloseDisplayWindow(const char *windowName);
    static void CloseAllDisplayWindows();

    // Resize in place by scaleFactor
    void Resize(f32 scaleFactor, ResizeMethod method = ResizeMethod::Linear);
    
    // Resize in place to a specific size
    void Resize(s32 desiredRows, s32 desiredCols, ResizeMethod method = ResizeMethod::Linear);
    
    // Resize into the new image (which is already the desired size)
    void Resize(ImageBase<T>& resizedImage, ResizeMethod method = ResizeMethod::Linear) const;

    // Resize in place to a specific size, keeps the aspect ratio
    // If onlyReduceSize=true, does not resize if new size is larger than requested size
    void ResizeKeepAspectRatio(s32 desiredRows, s32 desiredCols,
                               ResizeMethod method = ResizeMethod::Linear, bool onlyReduceSize = false);

    // Resize into the new image (which is already the desired size), keeps the aspect ratio
    void ResizeKeepAspectRatio(ImageBase<T>& resizedImage, ResizeMethod method = ResizeMethod::Linear) const;

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
    void DrawRect(const Rectangle<s32>& rect, const ColorRGBA& color, const s32 thickness = 1);
    
    // Draw filled rectangle from top left <X,Y> to bottom right <X+width,Y+height>
    void DrawFilledRect(const Rectangle<f32>& rect, const ColorRGBA& color);
    void DrawFilledRect(const Rectangle<s32>& rect, const ColorRGBA& color);
    
    // Draw quadrangle defined by four given points
    void DrawQuad(const Quad2f& quad, const ColorRGBA& color, const s32 thickness = 1);

    // Draw a filled convex polygon defined by a list of points
    void DrawFilledConvexPolygon(const std::vector<Point2i> points, const ColorRGBA& color);

    // TODO: Expose font?
    void DrawText(const Point2f& position, 
                  const std::string& str, 
                  const ColorRGBA& color, 
                  f32 scale = 1.f, 
                  bool dropShadow = false, 
                  int thickness = 1);

    // Returns the bounding box dimensions of the text that would be drawn
    // if the same parameters were passed into DrawText()
    static Vec2f GetTextSize(const std::string& str, f32 scale, int thickness);

    // DrawSubImage also exists - see derived class for implemetation
    
    using Array2d<T>::GetDataPointer;
    using Array2d<T>::IsEmpty;
    using Array2d<T>::GetNumRows;
    using Array2d<T>::GetNumCols;
    
    virtual s32 GetNumChannels() const = 0;

    Rectangle<s32> GetBoundingRect(){return Rectangle<s32>(0, 0, GetNumCols(), GetNumRows());}
    
    // Converts image to a format that is usable by imshow and imwrite
    // (BGR in general)
    virtual void ConvertToShowableFormat(cv::Mat& showImg) const = 0;

    // Runs a boxFilter of the given size on this image outputing filtered
    virtual void BoxFilter(ImageBase<T>& filtered, u32 size) const;

  protected:
    template<typename DerivedType>
    DerivedType GetROI(Rectangle<s32>& roiRect);
    
    template<typename DerivedType>
    const DerivedType GetROI(Rectangle<s32>& roiRect) const;

    // If drawBlankPixels is set to false the draw will require slightly more computation
    // but no pixels with empty value will be copied allowing "transparency" in undrawn areas
    template<typename DerivedType>
    void DrawSubImage(const DerivedType& subImage, const Point2f& topLeftCorner, T blankPixelValue, bool drawBlankPixels);

    virtual cv::Scalar GetCvColor(const ColorRGBA& color) const;

    // Sets image from a showable format (either gray or BGR)
    virtual void SetFromShowableFormat(const cv::Mat& showImg) = 0;

  private:
    TimeStamp_t     _timeStamp;
    u32             _imageId;
    
  }; // class ImageBase
  
  
  // Grayscale (i.e. scalar-valued) image, 8bpp
  class Image : public ImageBase<u8>
  {
  public:
    
    // Create an empty image
    Image();
    virtual ~Image();
    
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

    // Methods to access the raw data pointer to match what exists
    // in ImageRGB565
    u8* GetRawDataPointer() {
      return ImageBase<u8>::GetDataPointer();
    }
    const u8* GetRawDataPointer() const {
      return ImageBase<u8>::GetDataPointer();
    }
    
    void DrawSubImage(const Image& subImage, const Point2f& topLeftCorner, bool drawBlankPixels = true) { 
      return ImageBase<u8>::DrawSubImage<Image>(subImage, topLeftCorner, 0, drawBlankPixels); 
    }
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

    virtual void ConvertToShowableFormat(cv::Mat& showImg) const override;

    void BoxFilter(ImageBase<u8>& filtered, u32 size) const override;

  protected:
    virtual cv::Scalar GetCvColor(const ColorRGBA& color) const override;

    virtual void SetFromShowableFormat(const cv::Mat& showImg) override;
  
  }; // class Image
  
  
  // Forward declaration:
  class ImageRGBA;
  class ImageRGB565;
  
  // RGB Color image, 24bpp
  class ImageRGB : public ImageBase<PixelRGB>
  {
  public:
    ImageRGB();
    ImageRGB(s32 nrows, s32 ncols); // allocates
    ImageRGB(s32 nrows, s32 ncols, const PixelRGB& fillValue);
    ImageRGB(const ImageBase<PixelRGB>& imageBase) : ImageBase<PixelRGB>(imageBase) { }
    
    // No allocation, just wraps header around given data.
    // Assumes data is nrows*ncols*3 in length!
    ImageRGB(s32 nrows, s32 ncols, u8* data);
    
    // Removes alpha channel and squeezes into 24bpp
    ImageRGB(const ImageRGBA& imageRGBA);
    
    ImageRGB(const Array2d<PixelRGB>& array2d) : ImageBase<PixelRGB>(array2d) { }
    
    ImageRGB(const ImageRGB565& imgRGB565);
    
    void DrawSubImage(const ImageRGB& subImage, const Point2f& topLeftCorner, bool drawBlankPixels = true) { 
      return ImageBase<PixelRGB>::DrawSubImage<ImageRGB>(subImage, topLeftCorner, Vision::PixelRGB(), drawBlankPixels); 
    }
    ImageRGB GetROI(Rectangle<s32>& roiRect) { return ImageBase<PixelRGB>::GetROI<ImageRGB>(roiRect); }
    const ImageRGB GetROI(Rectangle<s32>& roiRect) const { return ImageBase<PixelRGB>::GetROI<ImageRGB>(roiRect); }
    
    // To/From Gray:
    // (Gray->Color replicates grayscale image across all three channels.)
    ImageRGB(const Image& imageGray);             // Construct new RGB image from a gray one
    Image ToGray() const;                         // Return a new gray image from this RGB one
    void FillGray(Image& grayOut) const;          // Fill existing gray image from this RGB image
    ImageRGB& SetFromGray(const Image& grayIn);   // Set this RGB image from given gray image
    ImageRGB& SetFromRGB565(const ImageRGB565& rgb565); // Set from given RGB565 image
    
    // Sets all pixels > value to 255 and all values <= value to 0.
    // If anyChannel=true, then any channel being above the value suffices. Otherwise, all channels must be.
    Image Threshold(u8 value, bool anyChannel) const;
    
    virtual s32 GetNumChannels() const override { return 3; }
    
    // Divide each channel by the sum across channels. Multiply result by 255 to keep in 8bit range.
    // Optionally, you can provide a working array to avoid allocation (will allocate to [NumRows x NumCols x 1])
    ImageRGB& NormalizeColor(Array2d<s32>* workingArray = nullptr); // in place
    void GetNormalizedColor(ImageRGB& imgNorm, Array2d<s32>* workingArray = nullptr) const;
    
    virtual void ConvertToShowableFormat(cv::Mat& showImg) const override;

    // Conversion from hsv to rgb565
    void ConvertHSV2RGB565(ImageRGB565& output);

  protected:
    virtual void SetFromShowableFormat(const cv::Mat& showImg) override;

  }; // class ImageRGB
  
  
  class ImageRGB565 : public ImageBase<PixelRGB565>
  {
  public:
    ImageRGB565();
    ImageRGB565(s32 nrows, s32 ncols);
    explicit ImageRGB565(const ImageRGB& imageRGB);
    explicit ImageRGB565(const ImageBase<PixelRGB565>& imageBase) : ImageBase<PixelRGB565>(imageBase) { }
    ImageRGB565(const Array2d<PixelRGB565>& array2d) : ImageBase<PixelRGB565>(array2d) { }
    
    ImageRGB565& SetFromImage(const Image& image);
    ImageRGB565& SetFromImageRGB(const ImageRGB& imageRGB);
    ImageRGB565& SetFromImageRGB(const ImageRGB& imageRGB, const std::array<u8, 256>& gammaLUT);
    ImageRGB565& SetFromImageRGB565(const ImageRGB565& imageRGB565);
    
    // Reference counting assignment (does not copy):
    ImageRGB565& operator= (const ImageBase<PixelRGB565> &other);

    void DrawSubImage(const ImageRGB565& subImage, const Point2f& topLeftCorner, bool drawBlankPixels = true) { 
      return ImageBase<PixelRGB565>::DrawSubImage<ImageRGB565>(subImage, topLeftCorner, Vision::PixelRGB565(), drawBlankPixels); 
    }
    ImageRGB565 GetROI(Rectangle<s32>& roiRect) { return ImageBase<PixelRGB565>::GetROI<ImageRGB565>(roiRect); }
    const ImageRGB565 GetROI(Rectangle<s32>& roiRect) const { return ImageBase<PixelRGB565>::GetROI<ImageRGB565>(roiRect); }

    const u16* GetRawDataPointer() const {
      DEV_ASSERT(IsContinuous(), "ImageRGB565.GetRawDataPointer.NotContinuous");
      static_assert(sizeof(PixelRGB565) == sizeof(u16), "Unexpected size for PixelRGB565");
      return reinterpret_cast<const u16*>(Array2d<PixelRGB565>::GetDataPointer());
    }
    
    u16* GetRawDataPointer() {
      DEV_ASSERT(IsContinuous(), "ImageRGB565.GetRawDataPointer.NotContinuous");
      static_assert(sizeof(PixelRGB565) == sizeof(u16), "Unexpected size for PixelRGB565");
      return reinterpret_cast<u16*>(Array2d<PixelRGB565>::GetDataPointer());
    }

    virtual s32 GetNumChannels() const override { return 3; }

    virtual void ConvertToShowableFormat(cv::Mat& showImg) const override;

  protected:
    virtual cv::Scalar GetCvColor(const ColorRGBA& color) const override;

    virtual void SetFromShowableFormat(const cv::Mat& showImg) override;
  };
  
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

    ImageRGBA(const Array2d<PixelRGBA>& array2d) : ImageBase<PixelRGBA>(array2d) { }

#   if ANKICORETECH_USE_OPENCV
    ImageRGBA(cv::Mat& cvMat);
#   endif
    
    Image ToGray() const;
    void FillGray(Image& grayOut) const;
    ImageRGBA& SetFromGray(const Image& imageGray);   // Set this RGBA image from given gray image
    ImageRGBA& SetFromRGB565(const ImageRGB565& rgb565, const u8 alpha = 255); // Set from given RGB565 image

    void DrawSubImage(const ImageRGBA& subImage, const Point2f& topLeftCorner, bool drawBlankPixels = true) { 
      return ImageBase<PixelRGBA>::DrawSubImage<ImageRGBA>(subImage, topLeftCorner, Vision::PixelRGBA(0,0,0,255), drawBlankPixels); 
    }
    ImageRGBA GetROI(Rectangle<s32>& roiRect) {
      return ImageBase<PixelRGBA>::GetROI<ImageRGBA>(roiRect);
    }
    
    virtual s32 GetNumChannels() const override { return 4; }
    
    virtual void ConvertToShowableFormat(cv::Mat& showImg) const override;

  protected:
    virtual void SetFromShowableFormat(const cv::Mat& showImg) override;

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
  inline void ImageBase<T>::SetImageId(u32 imageId)
  {
    _imageId = imageId;
  }
  
  template<typename T>
  inline u32 ImageBase<T>::GetImageId() const
  {
    return _imageId;
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
