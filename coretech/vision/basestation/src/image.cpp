/**
 * File: image.cpp
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Implements a container for images on the basestation.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/vision/basestation/image_impl.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#endif

namespace Anki {
namespace Vision {
  
#pragma mark --- ImageBase ---
  
  template<typename T>
  Result ImageBase<T>::Load(const std::string& filename)
  {
    this->get_CvMat_() = cv::imread(filename, (GetNumChannels() == 1 ?
                                               CV_LOAD_IMAGE_GRAYSCALE :
                                               CV_LOAD_IMAGE_COLOR));
    
    if(IsEmpty()) {
      return RESULT_FAIL;
    } else {
      return RESULT_OK;
    }
  }
  
  template<typename T>
  Result ImageBase<T>::Save(const std::string &filename, s32 quality)
  {
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(quality);
    
    // Convert color images to BGR(A) for saving (as assumed by imwrite)
    cv::Mat saveImg;
    switch(GetNumChannels())
    {
      case 1:
        saveImg = this->get_CvMat_();
        break;
        
      case 3:
        cv::cvtColor(this->get_CvMat_(), saveImg, cv::COLOR_RGB2BGR);
        break;
        
      case 4:
        cv::cvtColor(this->get_CvMat_(), saveImg, cv::COLOR_RGBA2BGRA);
        break;
        
      default:
        PRINT_NAMED_WARNING("ImageBase.Save.UnexpectedNumChannels",
                            "Don't know how to save %d-channel image",
                            GetNumChannels());
        return RESULT_FAIL;
    }
    
    Util::FileUtils::CreateDirectory(filename, true, true);
    
    try {
      const bool success = imwrite(filename, saveImg, compression_params);
      if(!success) {
        PRINT_NAMED_WARNING("ImageBase.Save.ImwriteFailed",
                            "Failed writing %dx%d image to %s",
                            GetNumCols(), GetNumRows(), filename.c_str());
        return RESULT_FAIL;
      }
    }
    catch (cv::Exception& ex) {
      PRINT_NAMED_WARNING("ImageBase.Save.ImwriteException",
                          "Failed writing to %s: %s",
                          filename.c_str(), ex.what());
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  template<typename T>
  ImageBase<T>& ImageBase<T>::operator= (const ImageBase<T> &other)
  {
    SetTimestamp(other.GetTimestamp());
    Array2d<T>::operator=(other);
    return *this;
  }
  
  template<typename T>
  void ImageBase<T>::Display(const char *windowName, s32 pauseTime_ms) const
  {
#   if defined(ANKI_PLATFORM_IOS) || defined(ANKI_PLATFORM_ANDROID)
    {
      PRINT_NAMED_WARNING("ImageBase.Display.NoDisplayOnAndroidOrIOS",
                          "Ignoring display request for %s", windowName);
      return;
    }
#   endif
    
    switch(GetNumChannels())
    {
      case 1:
      {
        cv::imshow(windowName, this->get_CvMat_());
        break;
      }
      case 3:
      {
        cv::Mat dispImg;
        cvtColor(this->get_CvMat_(), dispImg, CV_RGB2BGR); // imshow expects BGR color ordering
        cv::imshow(windowName, dispImg);
        break;
      }
      case 4:
      {
        cv::Mat dispImg;
        cvtColor(this->get_CvMat_(), dispImg, CV_RGBA2BGR);
        cv::imshow(windowName, dispImg);
        break;
      }
      default:
        PRINT_NAMED_ERROR("ImageBase.Display.InvaludNumChannels",
                          "Cannot display image with %d channels.", GetNumChannels());
        return;
    }
    
    cv::waitKey(pauseTime_ms);
  }

  template<typename T>
  void ImageBase<T>::CloseDisplayWindow(const char *windowName)
  {
#   if defined(ANKI_PLATFORM_IOS) || defined(ANKI_PLATFORM_ANDROID)
    {
      PRINT_NAMED_WARNING("ImageBase.CloseDisplayWindow.NoDisplayOnAndroidOrIOS",
                          "Ignoring close display request for %s", windowName);
      return;
    }
#   endif
    
    cv::destroyWindow(windowName);
  }
  
  template<typename T>
  void ImageBase<T>::CloseAllDisplayWindows()
  {
#   if defined(ANKI_PLATFORM_IOS) || defined(ANKI_PLATFORM_ANDROID)
    {
      // NOTE: Display should not be possible (via similar checks above), so
      // there's no real harm in calling this method, so just a debug, not
      // a warning.
      PRINT_NAMED_DEBUG("ImageBase.CloseAllDisplayWindows.NoDisplayOnAndroidOrIOS", "");
      return;
    }
#   endif
    
    cv::destroyAllWindows();
  }
  
  inline cv::Scalar GetCvColor(const ColorRGBA& color) {
    return CV_RGB(color.b(), color.g(), color.r());
  }
  
  template<typename T>
  void ImageBase<T>::DrawLine(const Point2f& start, const Point2f& end,
                              const ColorRGBA& color, const s32 thickness)
  {
    cv::line(this->get_CvMat_(), start.get_CvPoint_(), end.get_CvPoint_(),
             GetCvColor(color), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawPoint(const Point2f& point, const ColorRGBA& color, const s32 size)
  {
    cv::circle(this->get_CvMat_(), point.get_CvPoint_(), size, GetCvColor(color));
  }
  
  template<typename T>
  void ImageBase<T>::DrawQuad(const Quad2f& quad, const ColorRGBA& color, const s32 thickness)
  {
    using namespace Quad;
    DrawLine(quad[CornerName::TopLeft], quad[CornerName::TopRight], color, thickness);
    DrawLine(quad[CornerName::TopLeft], quad[CornerName::BottomLeft], color, thickness);
    DrawLine(quad[CornerName::TopRight], quad[CornerName::BottomRight], color, thickness);
    DrawLine(quad[CornerName::BottomLeft], quad[CornerName::BottomRight], color, thickness);
  }
  
  // Compile time "LUT" for converting from our resize method to OpenCV's
  static inline int GetOpenCvInterpMethod(ResizeMethod method)
  {
    switch(method)
    {
      case ResizeMethod::NearestNeighbor:
        return CV_INTER_NN;
        
      case ResizeMethod::Linear:
        return CV_INTER_LINEAR;
        
      case ResizeMethod::Cubic:
        return CV_INTER_CUBIC;
        
      case ResizeMethod::AverageArea:
        return CV_INTER_AREA;
    }
  }
  
  
  template<typename T>
  void ImageBase<T>::DrawRect(const Rectangle<f32>& rect, const ColorRGBA& color, const s32 thickness)
  {
    DrawLine(rect.GetTopLeft(), rect.GetTopRight(), color, thickness);
    DrawLine(rect.GetTopLeft(), rect.GetBottomLeft(), color, thickness);
    DrawLine(rect.GetTopRight(), rect.GetBottomRight(), color, thickness);
    DrawLine(rect.GetBottomLeft(), rect.GetBottomRight(), color, thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawText(const Point2f& position, const std::string& str,
                              const ColorRGBA& color, f32 scale, bool dropShadow)
  {
    if(dropShadow) {
      cv::Point shadowPos(position.get_CvPoint_());
      shadowPos.x += 1;
      shadowPos.y += 1;
      cv::putText(this->get_CvMat_(), str, shadowPos, CV_FONT_NORMAL, scale, GetCvColor(NamedColors::BLACK));
    }
    cv::putText(this->get_CvMat_(), str, position.get_CvPoint_(), CV_FONT_NORMAL, scale, GetCvColor(color));
  }
  
  template<typename T>
  void ImageBase<T>::Resize(f32 scaleFactor, ResizeMethod method)
  {
    cv::resize(this->get_CvMat_(), this->get_CvMat_(), cv::Size(), scaleFactor, scaleFactor,
               GetOpenCvInterpMethod(method));
  }
  
  template<typename T>
  void ImageBase<T>::Resize(s32 desiredRows, s32 desiredCols, ResizeMethod method)
  {
    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      const cv::Size desiredSize(desiredCols, desiredRows);
      cv::resize(this->get_CvMat_(), this->get_CvMat_(), desiredSize, 0, 0,
                 GetOpenCvInterpMethod(method));
    }
  }
  
  template<typename T>
  void ImageBase<T>::Resize(ImageBase<T>& resizedImage, ResizeMethod method) const
  {
    if(resizedImage.IsEmpty()) {
      printf("Image::Resize - Output image should already be the desired size.\n");
    } else {
      const cv::Size desiredSize(resizedImage.GetNumCols(), resizedImage.GetNumRows());
      cv::resize(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredSize, 0, 0,
                 GetOpenCvInterpMethod(method));
      resizedImage.SetTimestamp(this->GetTimestamp());
    }
  }
  
  template<typename T>
  void ImageBase<T>::CopyTo(ImageBase<T>& otherImage) const
  {
    Array2d<T>::CopyTo(otherImage);
    otherImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets copied too
  }
  
  // Explicit instantation for each image type:
  template class ImageBase<u8>;
  template class ImageBase<PixelRGB>;
  template class ImageBase<PixelRGBA>;
  
#pragma mark --- Image ---
  
  Image::Image()
  : ImageBase<u8>()
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols)
  : ImageBase<u8>(nrows, ncols)
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols, u8* data)
  : ImageBase<u8>(nrows,ncols,data)
  {
    
  }
  
#if ANKICORETECH_USE_OPENCV
  Image::Image(cv::Mat_<u8>& cvMat)
  : ImageBase<u8>(cvMat)
  {
    
  }
#endif
  
  Image& Image::Negate()
  {
    cv::bitwise_not(get_CvMat_(), get_CvMat_());
    return *this;
  }
 
  Image Image::GetNegative() const
  {
    Image output;
    cv::bitwise_not(get_CvMat_(), output.get_CvMat_());
    return output;
  }
  
  Image& Image::Threshold(u8 value)
  {
    get_CvMat_() = get_CvMat_() > value;
    return *this;
  }
  
  Image  Image::Threshold(u8 value) const
  {
    Image thresholdedImage;
    this->CopyTo(thresholdedImage);
    return thresholdedImage.Threshold(value);
  }

  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage) const
  {
    const s32 count = cv::connectedComponents(this->get_CvMat_(), labelImage.get_CvMat_());
    return count;
  } // GetConnectedComponents()
  
  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage, std::vector<ConnectedComponentStats>& stats) const
  {
    cv::Mat cvStats, cvCentroids;
    const s32 count = cv::connectedComponentsWithStats(this->get_CvMat_(), labelImage.get_CvMat_(), cvStats, cvCentroids);
    for(s32 iComp=0; iComp < count; ++iComp) 
    {
      const s32* compStats = cvStats.ptr<s32>(iComp);
      const f64* compCentroid = cvCentroids.ptr<f64>(iComp);

      ConnectedComponentStats stat{
        .area        = (size_t)compStats[cv::CC_STAT_AREA],
        .centroid    = Point2f(Util::Clamp(compCentroid[0], 0.0, (f64)(GetNumCols()-1)),
                               Util::Clamp(compCentroid[1], 0.0, (f64)(GetNumRows()-1))),
        .boundingBox = Rectangle<s32>(compStats[cv::CC_STAT_LEFT],  compStats[cv::CC_STAT_TOP],
                                      compStats[cv::CC_STAT_WIDTH], compStats[cv::CC_STAT_HEIGHT]),
      };
      
      stats.push_back(std::move(stat));
    }
    
    return count;
  }
  
#if 0
#pragma mark --- ImageRGBA ---
#endif
  
  ImageRGBA::ImageRGBA()
  : ImageBase<PixelRGBA>()
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols)
  : ImageBase<PixelRGBA>(nrows, ncols)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u32* data)
  : ImageBase<PixelRGBA>(nrows, ncols, reinterpret_cast<PixelRGBA*>(data))
  {
    
  }
  
  ImageRGBA::ImageRGBA(const ImageRGB& imageRGB, u8 alpha)
  : ImageRGBA(imageRGB.GetNumRows(), imageRGB.GetNumCols())
  {
    PixelRGBA* dataRGBA = GetDataPointer();
    const PixelRGB* dataRGB = imageRGB.GetDataPointer();
    
    for(s32 i=0; i<GetNumElements(); ++i)
    {
      dataRGBA[i].r() = dataRGB[i].r();
      dataRGBA[i].g() = dataRGB[i].g();
      dataRGBA[i].b() = dataRGB[i].b();
      dataRGBA[i].a() = alpha;
    }
    
    SetTimestamp(imageRGB.GetTimestamp());
  }
  
  Image ImageRGBA::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    grayImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets transferred!
    cv::cvtColor(this->get_CvMat_(), grayImage.get_CvMat_(), CV_RGBA2GRAY);
    return grayImage;
  }
  
  
#if 0 
#pragma mark --- ImageRGB ---
#endif 
  
  ImageRGB::ImageRGB()
  : ImageBase<PixelRGB>()
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols)
  : ImageBase<PixelRGB>(nrows, ncols)
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols, u8* data)
  : ImageBase<PixelRGB>(nrows, ncols, reinterpret_cast<PixelRGB*>(data))
  {
    
  }
  
  ImageRGB::ImageRGB(const ImageRGBA& imageRGBA)
  : ImageBase<PixelRGB>(imageRGBA.GetNumRows(), imageRGBA.GetNumCols())
  {
    PixelRGB* dataRGB = GetDataPointer();
    const PixelRGBA* dataRGBA = imageRGBA.GetDataPointer();
    
    for(s32 i=0; i<GetNumElements(); ++i)
    {
      // TODO: Is this faster? memcpy(dataRGB+i, dataRGBA+i, 3);
      dataRGB[i].r() = dataRGBA[i].r();
      dataRGB[i].g() = dataRGBA[i].g();
      dataRGB[i].b() = dataRGBA[i].b();
    }
    SetTimestamp(imageRGBA.GetTimestamp());
  }
  
  ImageRGB::ImageRGB(const Image& imageGray)
  : ImageBase<PixelRGB>(imageGray.GetNumRows(), imageGray.GetNumCols())
  {
    cv::cvtColor(imageGray.get_CvMat_(), this->get_CvMat_(), CV_GRAY2RGB);
    SetTimestamp(imageGray.GetTimestamp());
  }
  
  Image ImageRGB::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    grayImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets transferred!
    cv::cvtColor(this->get_CvMat_(), grayImage.get_CvMat_(), CV_RGB2GRAY);
    return grayImage;
  }
  
  Image ImageRGB::Threshold(u8 value, bool anyChannel) const
  {
    std::function<u8(const PixelRGB&)> thresholdFcn = [value,anyChannel](const PixelRGB& p)
    {
      if(p.IsBrighterThan(value, anyChannel)) {
        return 255;
      } else {
        return 0;
      }
    };
    
    Image out(GetNumRows(), GetNumCols());
    ApplyScalarFunction(thresholdFcn, out);
    
    return out;
  }
  

} // namespace Vision
} // namespace Anki
