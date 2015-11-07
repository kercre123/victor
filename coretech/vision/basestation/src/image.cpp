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

#include "anki/vision/basestation/image_impl.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#endif

// Uncomment this to test cropping the 4:3 image to 16:9 format by simply
// adding block bars to the top and bottom 12.5% of the image
//#define TEST_16_9_ASPECT_RATIO

namespace Anki {
namespace Vision {
  
  
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
  
  template<typename T>
  void ImageBase<T>::Display(const char *windowName, bool pause) const
  {
#   if ANKICORETECH_USE_OPENCV
    cv::imshow(windowName, this->get_CvMat_());
    if(pause) {
      cv::waitKey();
    }
#   endif
  }

  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage,
                                    std::vector<std::vector< Point2<s32> > >& regionPoints) const
  {
    // Until we start using OpenCV 3, which has an actual connected components implementation,
    // this is adapted from here:
    //    http://nghiaho.com/uploads/code/opencv_connected_component/blob.cpp
    //
    
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
  
  
  void Image::Resize(f32 scaleFactor)
  {
    cv::resize(this->get_CvMat_(), this->get_CvMat_(), cv::Size(), scaleFactor, scaleFactor, CV_INTER_LINEAR);
  }
  
  void Image::Resize(s32 desiredRows, s32 desiredCols)
  {
    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      const cv::Size desiredSize(desiredCols, desiredRows);
      cv::resize(this->get_CvMat_(), this->get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
    }
  }
  
  void Image::Resize(Image& resizedImage) const
  {
    if(resizedImage.IsEmpty()) {
      printf("Image::Resize - Output image should already be the desired size.\n");
    } else {
      const cv::Size desiredSize(resizedImage.GetNumCols(), resizedImage.GetNumRows());
      cv::resize(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
    }
  }
  
  
  
#if 0
#pragma mark --- ImageRGBA ---
#endif
  
  ImageRGBA::ImageRGBA()
  : ImageBase<u32>()
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols)
  : ImageBase<u32>(nrows, ncols)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u32* data)
  : ImageBase<u32>(nrows, ncols, data)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u8* data24)
  : ImageRGBA(nrows,ncols)
  {
    s32 index=0;
    u32* data32 = GetDataPointer();
    for(s32 i=0; i<nrows*ncols; ++i, index+=3) {
      data32[i] = ((static_cast<u32>(data24[index])  <<24) +
                   (static_cast<u32>(data24[index+1])<<16) +
                   (static_cast<u32>(data24[index+2])<<8));
    }
  }
  
  Image ImageRGBA::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    cv::cvtColor(this->get_CvMat_(), grayImage.get_CvMat_(), CV_RGBA2GRAY);
    return grayImage;
  }
  
#if 0 
#pragma mark --- ImageRGB ---
#endif 
  
  ImageRGB::ImageRGB()
  : ImageBase<RGBPixel>()
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols)
  : ImageBase<RGBPixel>(nrows, ncols)
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols, u8* data)
  : ImageBase<RGBPixel>(nrows, ncols, reinterpret_cast<RGBPixel*>(data))
  {
    
  }
  
  Image ImageRGB::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    cv::cvtColor(this->get_CvMat_(), grayImage.get_CvMat_(), CV_RGB2GRAY);
    return grayImage;
  }
/*
# if ANKICORETECH_USE_OPENCV
  cv::Mat ImageRGB::get_CvMat_()
  {
    return cv::Mat(GetNumRows(), GetNumCols(), CV_8UC3, GetDataPointer());
  }
  
  const cv::Mat ImageRGB::get_CvMat_() const
  {
    return cv::Mat(GetNumRows(), GetNumCols(), CV_8UC3, const_cast<RGBPixel*>(GetDataPointer()));
  }
# endif
*/
} // namespace Vision
} // namespace Anki
