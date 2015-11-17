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

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#endif

namespace Anki {
namespace Vision {
  
#pragma mark --- ImageBase ---
  
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
#   if ANKICORETECH_USE_OPENCV
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
#   endif
  }

  template<typename T>
  void ImageBase<T>::DrawLine(const Point2f& start, const Point2f& end,
                              const ColorRGBA& color, const s32 thickness)
  {
    cv::line(this->get_CvMat_(), start.get_CvPoint_(), end.get_CvPoint_(),
             CV_RGB(color.r(), color.g(), color.b()), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawPoint(const Point2f& point, const ColorRGBA& color, const s32 size)
  {
    cv::circle(this->get_CvMat_(), point.get_CvPoint_(), size,
               CV_RGB(color.r(), color.g(), color.b()));
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
  
  template<typename T>
  void ImageBase<T>::Resize(f32 scaleFactor)
  {
    cv::resize(this->get_CvMat_(), this->get_CvMat_(), cv::Size(), scaleFactor, scaleFactor, CV_INTER_LINEAR);
  }
  
  template<typename T>
  void ImageBase<T>::Resize(s32 desiredRows, s32 desiredCols)
  {
    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      const cv::Size desiredSize(desiredCols, desiredRows);
      cv::resize(this->get_CvMat_(), this->get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
    }
  }
  
  template<typename T>
  void ImageBase<T>::Resize(ImageBase<T>& resizedImage) const
  {
    if(resizedImage.IsEmpty()) {
      printf("Image::Resize - Output image should already be the desired size.\n");
    } else {
      const cv::Size desiredSize(resizedImage.GetNumCols(), resizedImage.GetNumRows());
      cv::resize(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
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
 
  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage,
                                    std::vector<std::vector< Point2<s32> > >& regionPoints) const
  {
    // Until we start using OpenCV 3, which has an actual connected components implementation,
    // this is adapted from here:
    //    http://nghiaho.com/uploads/code/opencv_connected_component/blob.cpp
    //
    
    if(labelImage.GetNumRows() != GetNumRows() || labelImage.GetNumCols() != GetNumCols()) {
      PRINT_NAMED_ERROR("Image.GetConnectedComponents.SizeMismatch",
                        "Label image should be %dx%d, not %dx%d",
                        GetNumCols(), GetNumRows(), labelImage.GetNumCols(), labelImage.GetNumRows());
      return -1;
    }
    
    // Put a 1 in labelImage everywhere this image is > 0, and 0 otherwise
    // NOTE: can't use use cv::Mat's > 0 operation, b/c that yields 0 and 255.
    // TODO: is "(this->get_CvMat()_ > 0)/255" actually faster?
    std::function<s32(const u8&)> lambda = [](u8 p) {
      return s32(p > 0 ? 1 : 0);
    };
    ApplyScalarFunction(lambda, labelImage);

    regionPoints.clear();
    
    // Fill the label_image with the blobs
    // 0  - background
    // 1  - unlabelled foreground
    // 2+ - labelled foreground
    
    int labelCount = 2; // starts at 2 because 0,1 are used already
    
    for(int y=0; y < labelImage.GetNumRows(); y++) {
      s32 *row = labelImage.GetRow(y); // (s32*)labelImage.ptr(y);
      for(int x=0; x < labelImage.GetNumCols(); x++) {
        if(row[x] != 1) {
          continue;
        }
        
        cv::Rect rect;
        cv::floodFill(labelImage.get_CvMat_(), cv::Point(x,y), labelCount, &rect, 0, 0, 4);
        
        std::vector<Point2<s32> > blob;
        
        for(int i=rect.y; i < (rect.y+rect.height); i++) {
          int *row2 = (int*)labelImage.GetRow(i);
          for(int j=rect.x; j < (rect.x+rect.width); j++) {
            if(row2[j] != labelCount) {
              continue;
            }
            
            blob.emplace_back(j,i);
          }
        }
        
        regionPoints.emplace_back(blob);
        
        ++labelCount;
      }
    }
    
    return labelCount-2;
    
  } // GetConnectedComponents()
  
  
  
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

} // namespace Vision
} // namespace Anki
