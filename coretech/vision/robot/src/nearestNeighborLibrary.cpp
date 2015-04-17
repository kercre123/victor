
#include "anki/vision/robot/nearestNeighborLibrary.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"

namespace Anki {
namespace Embedded {
  
  NearestNeighborLibrary::NearestNeighborLibrary()
  : _data(NULL)
  , _labels(NULL)
  , _probeXOffsets(NULL)
  , _probeYOffsets(NULL)
  {
    
  }
  
  
  NearestNeighborLibrary::NearestNeighborLibrary(const u8* data,
                                                 const u16* labels,
                                                 const s32 numDataPoints, const s32 dataDim,
                                                 const s16* probePoints_X, const s16* probePoints_Y,
                                                 const s32 numProbePoints,
                                                 MemoryStack& memory)
  : _data(data)
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(labels)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  {
    _probeValues = FixedLengthList<u8>(_dataDimension, memory);

    
  }
  
  Result NearestNeighborLibrary::GetNearestNeighbor(const Array<u8> &image,
                                                    const Array<f32> &homography,
                                                    const s32 distThreshold,
                                                    s32 &label, s32 &distance) const
  {
    const s32 imageHeight = image.get_size(0);
    const s32 imageWidth = image.get_size(1);
    
    AnkiConditionalErrorAndReturnValue(AreValid(*this, image, homography, probeValues),
                                       RESULT_FAIL_INVALID_OBJECT, "VisionMarker::GetProbeValues", "Invalid objects");
    
    // This function is optimized for 9 or fewer probes, but this is not a big issue
    AnkiAssert(numProbeOffsets <= 9);
    
    const s32 numProbeOffsets = this->numProbeOffsets;
    const s32 numProbes = this->dataDimension;
    
    AnkiConditionalErrorAndReturnValue(probeValues.get_numElements()==numProbes, RESULT_FAIL_INVALID_SIZE,
                                       "VisionMarker::GetProbeValues",
                                       "Output probeValues array should have room for %d probes (it has %d).\n",
                                       numProbes, probeValues.get_numElements());
    
    const f32 h00 = homography[0][0];
    const f32 h10 = homography[1][0];
    const f32 h20 = homography[2][0];
    const f32 h01 = homography[0][1];
    const f32 h11 = homography[1][1];
    const f32 h21 = homography[2][1];
    const f32 h02 = homography[0][2];
    const f32 h12 = homography[1][2];
    const f32 h22 = homography[2][2];
    
    const f32 fixedPointDivider = 1.0f / static_cast<f32>(1 << this->nearestNeighborNumFractionalBits);
    
    const NearestNeighborExample * restrict pNearestNeighborLib = reinterpret_cast<const NearestNeighborExample*>(this->nearestNeighborLibrary);
    
    u8* restrict pProbeData = probeValues.Pointer(0, 0);
    
    f32 probeXOffsetsF32[9];
    f32 probeYOffsetsF32[9];
    
    for(s32 iOffset=0; iOffset<numProbeOffsets; iOffset++) {
      probeXOffsetsF32[iOffset] = static_cast<f32>(this->nearestNeighborLibrary.probeXOffsets[iOffset]) * fixedPointDivider;
      probeYOffsetsF32[iOffset] = static_cast<f32>(this->nearestNeighborLibrary.probeYOffsets[iOffset]) * fixedPointDivider;
    }
    
    for(s32 iProbe=0; iProbe<numProbes; ++iProbe)
    {
      const f32 xCenter = static_cast<f32>(pNearestNeighborLib[iProbe].probeXCenter) * fixedPointDivider;
      const f32 yCenter = static_cast<f32>(pNearestNeighborLib[iProbe].probeYCenter) * fixedPointDivider;
      
      u32 accumulator = 0;
      for(s32 iOffset=0; iOffset<numProbeOffsets; iOffset++) {
        // 1. Map each probe to its warped locations
        const f32 x = xCenter + probeXOffsetsF32[iOffset];
        const f32 y = yCenter + probeYOffsetsF32[iOffset];
        
        const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);
        
        const f32 warpedXf = (h00 * x + h01 *y + h02) * homogenousDivisor;
        const f32 warpedYf = (h10 * x + h11 *y + h12) * homogenousDivisor;
        
        const s32 warpedX = Round<s32>(warpedXf);
        const s32 warpedY = Round<s32>(warpedYf);
        
        // 2. Sample the image
        
        // This should only fail if there's a bug in the quad extraction
        AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < imageHeight && warpedX < imageWidth);
        
        const u8 imageValue = *image.Pointer(warpedY, warpedX);
        
        accumulator += imageValue;
      } // for each probe offset
      
      pProbeData[iProbe] = static_cast<u8>(accumulator / numProbeOffsets);
      
    } // for each probe
    
    return RESULT_OK;
  } // VisionMarker::GetProbeValues()
  
} // namespace Embedded
} // namesapce Anki