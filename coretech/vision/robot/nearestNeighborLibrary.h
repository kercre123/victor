/**
 * File: nearestNeighborLibrary.h
 *
 * Author: Andrew Stein
 * Date:   4/17/2015
 *
 * Description: Defines a class for storing a nearest-neighbor library of vision
 *              markers.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_CORETECHEMBEDDED_VISION_NEAREST_NEIGHBOR_LIBRARY_H
#define ANKI_CORETECHEMBEDDED_VISION_NEAREST_NEIGHBOR_LIBRARY_H

#include "coretech/common/shared/types.h"
#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki {
  
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Embedded {
  
  class NearestNeighborLibrary
  {
  public:
    NearestNeighborLibrary();
    
    NearestNeighborLibrary(const std::string& dataPath,
                           const s32 numDataPoints, const s32 dataDim,
                           const s16* probeCenters_X, const s16* probeCenters_Y,
                           const s16* probePoints_X, const s16* probePoints_Y,
                           const s32 numProbePoints, const s32 numFractionalBits);
    
    NearestNeighborLibrary(const u8* data,
                           const u8* weights,
                           const u16* labels,
                           const s32 numDataPoints, const s32 dataDim,
                           const s16* probeCenters_X, const s16* probeCenters_Y,
                           const s16* probePoints_X, const s16* probePoints_Y,
                           const s32 numProbePoints, const s32 numFractionalBits);
      
    Result GetNearestNeighbor(const Array<u8> &image,
                              const Array<f32> &homography,
                              const s32 distThreshold,
                              s32 &label, s32 &distance);

    s32 GetNumFractionalBits() const { return _numFractionalBits; }
    
  protected:
    
    cv::Mat_<u8> _probeValues;
    cv::Mat_<u8> _data;    // numDataPoints rows x dataDimension cols
    
    s32 _numDataPoints;
    s32 _dataDimension;
    
    cv::Mat_<u16> _labels; // numDataPoints in length
    
    const s16 * restrict _probeXCenters;
    const s16 * restrict _probeYCenters;
    
    const s16 * restrict _probeXOffsets;
    const s16 * restrict _probeYOffsets;
    s32 _numProbeOffsets;
    
    s32 _numFractionalBits;
    
  }; // class NearestNeighborLibrary
  
  
} // namespace Embedded
} // namespace Anki

#endif // ANKI_CORETECHEMBEDDED_VISION_NEAREST_NEIGHBOR_LIBRARY_H
