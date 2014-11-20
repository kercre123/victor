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

#ifndef ANKI_CORETECH_VISION_BASESTATION_IMAGE_H
#define ANKI_CORETECH_VISION_BASESTATION_IMAGE_H

#include "anki/common/types.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif


namespace Anki {
namespace Vision {

  class Image
//#if ANKICORETECH_USE_OPENCV
//  : private cv::Mat // private inheritance from cv::Mat
//#endif
  {
  public:
    
    // Allocate a new image
    Image(s32 nrows, s32 ncols);
    
    // Wrap image "header" around given data pointer: no allocation.
    Image(s32 nrows, s32 ncols, u8* data);
    
    bool IsEmpty() const;
    
    const u8* GetDataPointer() const;
    
    void SetTimestamp(TimeStamp_t ts);
    TimeStamp_t GetTimestamp() const;
    
    void Display(const char *windowName, bool pause = false) const;
    
  protected:
    TimeStamp_t     _timeStamp;
    
#   if ANKICORETECH_USE_OPENCV
    cv::Mat_<u8> _cvMat;
#   endif
    
  };
  
  
  //
  // Inlined implemenations
  //
  
  inline const u8* Image::GetDataPointer() const {
#   if ANKICORETECH_USE_OPENCV
    return _cvMat.ptr();
#   endif
  }
  
  inline bool Image::IsEmpty() const
  {
#   if ANKICORETECH_USE_OPENCV
    return _cvMat.empty();
#   endif
  }
  
  inline void Image::SetTimestamp(TimeStamp_t ts) {
    _timeStamp = ts;
  }
  
  inline TimeStamp_t Image::GetTimestamp() const {
    return _timeStamp;
  }
  

} // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_IMAGE_H
