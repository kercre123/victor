
#include "anki/vision/robot/nearestNeighborLibrary.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/errorHandling.h"

namespace Anki {
namespace Embedded {
  
  NearestNeighborLibrary::NearestNeighborLibrary()
  : _labels(NULL)
  , _probeXOffsets(NULL)
  , _probeYOffsets(NULL)
  {
    
  }
  
  
  NearestNeighborLibrary::NearestNeighborLibrary(const u8* data,
                                                 const u8* weights,
                                                 const u16* labels,
                                                 const s32 numDataPoints, const s32 dataDim,
                                                 const s16* probeCenters_X, const s16* probeCenters_Y,
                                                 const s16* probePoints_X, const s16* probePoints_Y,
                                                 const s32 numProbePoints, const s32 numFractionalBits)
  : _data(numDataPoints, dataDim, const_cast<u8*>(data))
  , _weights(numDataPoints, dataDim, const_cast<u8*>(weights))
  , _totalWeight(numDataPoints, 1)
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(labels)
  , _probeXCenters(probeCenters_X)
  , _probeYCenters(probeCenters_Y)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  , _numFractionalBits(numFractionalBits)
  , _probeValues(1, _dataDimension)
  {
    // Sum all the weights for each example in the library along the columns:
    cv::reduce(_weights, _totalWeight, 1, CV_REDUCE_SUM);
  }
  

  Result NearestNeighborLibrary::GetNearestNeighbor(const Array<u8> &image,
                                                    const Array<f32> &homography,
                                                    const s32 distThreshold,
                                                    s32 &label, s32 &closestDistance)
  {
    GetProbeValues(image, homography);
    
    // Visualize probe values
    //cv::Mat_<u8> temp(32,32,_probeValues.data);
    //cv::imshow("ProbeValues", temp);

    const u8* restrict pProbeData = _probeValues[0];
    
    closestDistance = distThreshold;
    s32 closestIndex = -1;
    for(s32 iExample=0; iExample<_numDataPoints; ++iExample)
    {
      const u8* currentExample = _data[iExample];
      const u8* currentWeight  = _weights[iExample];
      const s32 currentTotalWeight = _totalWeight(iExample, 0);
      
      // Visualize current example
      //cv::Mat_<u8> temp(32,32,const_cast<u8*>(currentExample));
      //cv::imshow("CurrentExample", temp);
      //cv::waitKey();
      
      // The distance threshold for this example depends on the total weight
      const s32 currentDistThreshold = currentTotalWeight * closestDistance;
      s32 currentDistance = 0;
      s32 iProbe = 0;
      while(iProbe < _dataDimension && currentDistance < currentDistThreshold)
      {
        const s32 diff = static_cast<s32>(pProbeData[iProbe]) - static_cast<s32>(currentExample[iProbe]);
        currentDistance += static_cast<s32>(currentWeight[iProbe]) * std::abs(diff);
        ++iProbe;
      }
      
      if(currentDistance < currentDistThreshold) {
        currentDistance /= currentTotalWeight;
        if(currentDistance < closestDistance) {
          closestIndex = iExample;
          closestDistance = currentDistance;
        }
      }
    } // for each example
    
    if(closestIndex != -1) {
      label = _labels[closestIndex];
    } else {
      label = -1;
    }
    
    return RESULT_OK;
  }
  
  Result NearestNeighborLibrary::GetProbeValues(const Array<u8> &image,
                                                const Array<f32> &homography)
  {
    const s32 imageHeight = image.get_size(0);
    const s32 imageWidth = image.get_size(1);
    
    AnkiConditionalErrorAndReturnValue(AreValid(image, homography),
                                       RESULT_FAIL_INVALID_OBJECT, "VisionMarker::GetProbeValues", "Invalid objects");
    
    // This function is optimized for 9 or fewer probes, but this is not a big issue
    AnkiAssert(_numProbeOffsets <= 9);
    
    const s32 numProbeOffsets = _numProbeOffsets;
    const s32 numProbes = _dataDimension;
    
    const f32 h00 = homography[0][0];
    const f32 h10 = homography[1][0];
    const f32 h20 = homography[2][0];
    const f32 h01 = homography[0][1];
    const f32 h11 = homography[1][1];
    const f32 h21 = homography[2][1];
    const f32 h02 = homography[0][2];
    const f32 h12 = homography[1][2];
    const f32 h22 = homography[2][2];
    
    const f32 fixedPointDivider = 1.0f / static_cast<f32>(1 << _numFractionalBits);
    
    u8* restrict pProbeData = _probeValues[0];
    
    f32 probeXOffsetsF32[9];
    f32 probeYOffsetsF32[9];
    
    for(s32 iOffset=0; iOffset<numProbeOffsets; iOffset++) {
      probeXOffsetsF32[iOffset] = static_cast<f32>(_probeXOffsets[iOffset]) * fixedPointDivider;
      probeYOffsetsF32[iOffset] = static_cast<f32>(_probeYOffsets[iOffset]) * fixedPointDivider;
    }
    
    u8 minValue = u8_MAX;
    u8 maxValue = 0;
    
    for(s32 iProbe=0; iProbe<numProbes; ++iProbe)
    {
      const f32 xCenter = static_cast<f32>(_probeXCenters[iProbe]) * fixedPointDivider;
      const f32 yCenter = static_cast<f32>(_probeYCenters[iProbe]) * fixedPointDivider;
      
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
      
      // Keep track of min/max for normalization below
      if(pProbeData[iProbe] < minValue) {
        minValue = pProbeData[iProbe];
      } else if(pProbeData[iProbe] > maxValue) {
        maxValue = pProbeData[iProbe];
      }
      
    } // for each probe
    
    // Normalization to scale between 0 and 255:
    const s32 divisor = static_cast<s32>(maxValue - minValue);
    for(s32 iProbe=0; iProbe<numProbes; ++iProbe) {
      pProbeData[iProbe] = (255*static_cast<s32>(pProbeData[iProbe] - minValue)) / divisor;
    }
    
    return RESULT_OK;
  } // VisionMarker::GetProbeValues()
  
} // namespace Embedded
} // namesapce Anki