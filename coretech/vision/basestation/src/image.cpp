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

#include "anki/vision/basestation/image.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/highgui/highgui.hpp"
#endif


namespace Anki {
namespace Vision {
  
  Image::Image(s32 nrows, s32 ncols)
# if ANKICORETECH_USE_OPENCV
  : _cvMat(nrows,ncols)
# endif
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols, u8* data)
# if ANKICORETECH_USE_OPENCV
  : _cvMat(nrows,ncols,data)
# endif
  {
    
  }
  
  void Image::Display(const char *windowName, bool pause) const
  {
#   if ANKICORETECH_USE_OPENCV
    cv::imshow(windowName, _cvMat);
    if(pause) {
      cv::waitKey();
    }
#   endif
  }

  
} // namespace Vision
} // namespace Anki
