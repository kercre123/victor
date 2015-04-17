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

namespace Anki {
namespace Embedded {
  
  template<typename T> class Array<T>;
  
  class NearestNeighborLibrary
  {
  public:
    NearestNeighborLibrary();
    NearestNeighborLibrary(const u8* data, const s32 numDataPoints, const s32 dataDim,
                           const s16* probePoints_X, const s16* probePoints_Y,
                           const s32 numProbePoints);
    
    Result GetProbeValues(const Array<u8> &image,
                          const Array<f32> &homography,
                          Array<u8> &probeValues) const;

    
  protected:
    
    const u8  * restrict libraryData; // numDataPoints rows x dataDimension cols
    s32 numDataPoints;
    s32 dataDimension;
    
    const s16 * restrict probeXOffsets;
    const s16 * restrict probeYOffsets;
    s32 numProbeOffsets;
    
  }; // class NearestNeighborLibrary
  
  
} // namespace Embedded
} // namespace Anki

#endif // ANKI_CORETECHEMBEDDED_VISION_NEAREST_NEIGHBOR_LIBRARY_H
