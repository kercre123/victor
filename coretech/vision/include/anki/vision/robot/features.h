/**
File: features.h
Author: Peter Barnum
Created: 2014-09-30
Utilities for detecting sparse features, and tracking them. Based off OpenCV

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef _ANKICORETECHEMBEDDED_VISION_FEATURES_H_
#define _ANKICORETECHEMBEDDED_VISION_FEATURES_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/geometry.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/integralImage.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Features
    {
      //Result goodFeaturesToTrack(
      //  const Array<u8> &image,
      //  FixedLengthList<f32> &corners,
      //  const s32 maxCorners,
      //  const f32 qualityLevel,
      //  const s32 blockSize = 3,
      //  const bool useHarrisDetector = false,
      //  const f32 k = 0.04);

      // Compute the Harris corner metric, per-pixel
      // Similar output to the OpenCV cv::cornerHarris(), except not scaled (divide the outputs by the max, and they are similar)
      Result CornerHarris(
        const Array<u8> &src,
        Array<f32> &dst,
        const s32 blockSize,
        const f32 k,
        MemoryStack scratch);
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FEATURES_H_
