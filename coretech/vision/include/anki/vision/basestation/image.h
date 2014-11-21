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

#include "anki/common/basestation/array2d.h"
#include "anki/common/basestation/math/point.h"


namespace Anki {
namespace Vision {

  class Image : public Array2d<u8>
  {
  public:
    
    // Create an empty image
    Image();
    
    // Allocate a new image
    Image(s32 nrows, s32 ncols);
    
    // Wrap image "header" around given data pointer: no allocation.
    Image(s32 nrows, s32 ncols, u8* data);
    
    void SetTimestamp(TimeStamp_t ts);
    TimeStamp_t GetTimestamp() const;
    
    void Display(const char *windowName, bool pause = false) const;

    s32 GetConnectedComponents(Array2d<s32>& labelImage,
                               std::vector<std::vector< Point2<s32> > >& regionPoints) const;
    
  protected:
    TimeStamp_t     _timeStamp;
        
  };
  
  
  //
  // Inlined implemenations
  //
  inline void Image::SetTimestamp(TimeStamp_t ts) {
    _timeStamp = ts;
  }
  
  inline TimeStamp_t Image::GetTimestamp() const {
    return _timeStamp;
  }
  

} // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_IMAGE_H
