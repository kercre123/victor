

#include "anki/vision/robot/nearestNeighborLibrary.h"
#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/errorHandling.h"


#include <array>

#define USE_EARLY_EXIT_DISTANCE_COMPUTATION 1
#define USE_WEIGHTS 1

namespace Anki {
namespace Embedded {
  
  NearestNeighborLibrary::NearestNeighborLibrary()
  : _labels(NULL)
  , _probeXOffsets(NULL)
  , _probeYOffsets(NULL)
  , _useHoG(false)
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
# if USE_WEIGHTS
  , _weights(numDataPoints, dataDim, const_cast<u8*>(weights))
  , _totalWeight(numDataPoints, 1)
# endif
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(labels)
  , _probeXCenters(probeCenters_X)
  , _probeYCenters(probeCenters_Y)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  , _numFractionalBits(numFractionalBits)
  , _useHoG(false)
  , _probeValues(1, _dataDimension)
  {
#   if USE_WEIGHTS
    // Sum all the weights for each example in the library along the columns:
    cv::reduce(_weights, _totalWeight, 1, CV_REDUCE_SUM);
#   endif
  }
  
  
  NearestNeighborLibrary::NearestNeighborLibrary(const u8* HoGdata,
                                                 const u16* labels,
                                                 const s32 numDataPoints, const s32 dataDim,
                                                 const s16* probeCenters_X, const s16* probeCenters_Y,
                                                 const s16* probePoints_X, const s16* probePoints_Y,
                                                 const s32 numProbePoints, const s32 numFractionalBits,
                                                 const s32 numHogScales, const s32 numHogOrientations)
  : _data(numDataPoints, dataDim, const_cast<u8*>(HoGdata))
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(labels)
  , _probeXCenters(probeCenters_X)
  , _probeYCenters(probeCenters_Y)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  , _numFractionalBits(numFractionalBits)
  , _useHoG(true)
  , _numHogScales(numHogScales)
  , _numHogOrientations(numHogOrientations)
  , _probeValues(1, _dataDimension)
  , _probeHoG(16, _numHogScales*_numHogOrientations)
  , _probeHoG_F32(16, _numHogScales*_numHogOrientations)
  {

  }
  

  Result NearestNeighborLibrary::GetNearestNeighbor(const Array<u8> &image,
                                                    const Array<f32> &homography,
                                                    const s32 distThreshold,
                                                    s32 &label, s32 &closestDistance)

  {
    // Set these return values up front, in case of failure
#   if USE_EARLY_EXIT_DISTANCE_COMPUTATION
    closestDistance = distThreshold;
#   else
    closestDistance = std::numeric_limits<s32>::max();
#   endif
    
    label = -1;
    
    Result lastResult = VisionMarker::GetProbeValues(image, homography, _probeValues);
    
    AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                       "NearestNeighborLibrary.GetNearestNeighbor",
                                       "GetProbeValues() failed.\n");
    
    // Visualize probe values
    //cv::Mat_<u8> temp(32,32,_probeValues.data);
    //cv::imshow("ProbeValues", temp);
    //cv::waitKey(10);
    
    
    if(_useHoG) {
      GetProbeHoG();
      assert(_probeHoG.isContinuous());
      
      // Visualize HoG values
      //cv::Mat_<u8> temp(16,32,_probeHoG.data);
      //cv::imshow("ProbeHoG", _probeHoG);
      //cv::waitKey();
      
    }
    const u8* restrict pProbeData = (_useHoG ? _probeHoG[0] : _probeValues[0]);
    

    s32 closestIndex = -1;
    
    if(_useHoG) {
      closestDistance *= _probeHoG.rows*_probeHoG.cols;
    }
    
    for(s32 iExample=0; iExample<_numDataPoints; ++iExample)
    {
      const u8* currentExample = _data[iExample];

      s32 currentDistance = 0;
      s32 iProbe = 0;

      if(_useHoG) {
        
        // Visualize current example
        //cv::Mat_<u8> temp(16, _numHogScales*_numHogOrientations,
        //                  const_cast<u8*>(currentExample));
        //cv::imshow("CurrentExample", temp);
        //cv::waitKey();
        
        while(iProbe < _dataDimension && currentDistance < closestDistance) {
          const s32 diff = static_cast<s32>(pProbeData[iProbe]) - static_cast<s32>(currentExample[iProbe]);
          currentDistance += std::abs(diff);
          ++iProbe;
        }
        
        if(currentDistance < closestDistance) {
          closestIndex = iExample;
          closestDistance = currentDistance;
        }
        
      } else {
        
#       if USE_WEIGHTS
        const u8* currentWeight  = _weights[iExample];
        const s32 currentTotalWeight = _totalWeight(iExample, 0);
#       endif
        
        // Visualize current example
        //cv::Mat_<u8> temp(32, 32, const_cast<u8*>(currentExample));
        //cv::namedWindow("CurrentExample", CV_WINDOW_KEEPRATIO);
        //cv::imshow("CurrentExample", temp);
        //cv::waitKey();
        
#       if USE_EARLY_EXIT_DISTANCE_COMPUTATION
        
#         if USE_WEIGHTS
          {
            // The distance threshold for this example depends on the total weight
            const s32 currentDistThreshold = currentTotalWeight * closestDistance;
            
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
          }
#         else // don't use weights
          {
            const s32 currentDistThreshold = _dataDimension * closestDistance;
            while(iProbe < _dataDimension && currentDistance < currentDistThreshold)
            {
              const s32 diff = static_cast<s32>(pProbeData[iProbe]) - static_cast<s32>(currentExample[iProbe]);
              currentDistance += std::abs(diff);
              ++iProbe;
            }
            
            if(currentDistance < currentDistThreshold) {
              currentDistance /= _dataDimension;
              if(currentDistance < closestDistance) {
                 closestIndex = iExample;
                closestDistance = currentDistance;
              }
            }
          }
#         endif // USE_WEIGHTS
        
        
#       else // don't use early exit distance computation
        
#         if USE_WEIGHTS
          {
            for(s32 iProbe=0; iProbe < _dataDimension; ++iProbe) {
              const s32 diff = static_cast<s32>(pProbeData[iProbe]) - static_cast<s32>(currentExample[iProbe]);
              currentDistance += static_cast<s32>(currentWeight[iProbe]) * std::abs(diff);
            }
            
            currentDistance /= currentTotalWeight;
            
            if(currentDistance < closestDistance) {
              closestDistance = currentDistance;
              if(currentDistance < distThreshold) {
                closestDistance = currentDistance;
                closestIndex = iExample;
              }
            }
          }
#         else // don't use weights
          {
            for(s32 iProbe=0; iProbe < _dataDimension; ++iProbe) {
              const s32 diff = static_cast<s32>(pProbeData[iProbe]) - static_cast<s32>(currentExample[iProbe]);
              currentDistance += std::abs(diff);
            }
            
            currentDistance /= _dataDimension;
            
            //printf("dist[%d] = %d\n", iExample, currentDistance);
            
            if(currentDistance < closestDistance) {
              closestDistance = currentDistance;
              if(currentDistance < distThreshold) {
                closestDistance = currentDistance;
                closestIndex = iExample;
              }
            }
          }
#         endif // USE_WEIGHTS
        
#       endif // USE_EARLY_EXIT_DISTANCE_COMPUTATION
        
      } // if(_useHoG)
      
    } // for each example
    
    if(closestIndex != -1) {
      label = _labels[closestIndex];
      if(_useHoG) {
        closestDistance /= _probeHoG.rows * _probeHoG.cols;
      }
    }
    
    return RESULT_OK;
  } // GetNearestNeighbor()
  
  
  Result NearestNeighborLibrary::GetProbeHoG()
  {
    static const s32 gridSize = static_cast<s32>(sqrt(static_cast<f64>(_dataDimension)));
    static cv::Mat_<u8> whichHist(gridSize, gridSize);
    static bool whichHistInitialized = false;
    if(!whichHistInitialized) {
      AnkiAssert(gridSize*gridSize == _dataDimension);
      
      for(s32 y=1; y<=gridSize; ++y) {
        u8* whichHist_y = whichHist[y-1];
        const s32 yi = std::ceil(4*static_cast<f32>(y)/static_cast<f32>(gridSize));
        for(s32 x=1; x<=gridSize; ++x) {
          const s32 xi = std::ceil(4*static_cast<f32>(x)/static_cast<f32>(gridSize));
          const s32 bin = yi + (xi-1)*4;
          AnkiAssert(bin > 0 && bin <= 16);
          whichHist_y[x-1] = bin - 1;
        }
      }
      
      whichHistInitialized = true;
      
      // Visualize whichHist
      //cv::imshow("WhichHist", whichHist*16);
      //cv::waitKey();
    }
    
    const f32 oneOver255 = 1.f / 255.f;
    
    _probeHoG_F32 = 0.f;
    
    for(s32 iScale=0; iScale<_numHogScales; ++iScale) {
      const s32 scale = 1 << iScale;
      
      std::array<f32,16> histSums;
      histSums.fill(0);
      
      for(s32 i=0; i<_probeValues.rows; ++i)
      {
        const u8* probeValues_i = _probeValues[i];
        const u8* probeValues_iUp = _probeValues[std::max(0, i-scale)];
        const u8* probeValues_iDown = _probeValues[std::min(_probeValues.rows-1, i+scale)];
        const u8* whichHist_i = whichHist[i];
        
        for(s32 j=0; j<_probeValues.cols; ++j)
        {
          const s32 jLeft  = std::max(0, j-scale);
          const s32 jRight = std::min(_probeValues.cols-1, j+scale);
          
          const f32 Ix = static_cast<f32>(probeValues_i[jRight] - probeValues_i[jLeft]) * oneOver255;
          const f32 Iy = static_cast<f32>(probeValues_iDown[j] - probeValues_iUp[j]) * oneOver255;
          
          const f32 mag = sqrtf(Ix*Ix + Iy*Iy);
          f32 orient = std::atan2(Iy, Ix);
          
          if(std::abs(-M_PI - orient) < 1e-6f) {
            orient = M_PI;
          }
          
          // From (-pi, pi] to (0, 2pi] and then to (0, 1] and finally to (0, numBins] and 1:numBins
          const f32 scaledOrient = (orient + M_PI)/(2*M_PI) * _numHogOrientations;
          const s32 binnedOrient_R = std::ceil(scaledOrient);
          const f32 weight_L = binnedOrient_R - scaledOrient;
          AnkiAssert(weight_L >= 0.f && weight_L <= 1.f);
          s32 binnedOrient_L = binnedOrient_R - 1;
          if(binnedOrient_L == 0) {
            binnedOrient_L = _numHogOrientations;
          }
          const f32 weight_R = 1.f - weight_L;
          
          const s32 row   = whichHist_i[j];
          const s32 col_L = binnedOrient_L + iScale*_numHogOrientations - 1;
          const s32 col_R = binnedOrient_R + iScale*_numHogOrientations - 1;
          
          AnkiAssert(col_L >= 0 && col_L < _probeHoG.cols);
          AnkiAssert(col_R >= 0 && col_R < _probeHoG.cols);
          
          const f32 leftVal  = mag*weight_L;
          const f32 rightVal = mag*weight_R;
          _probeHoG_F32(row,col_L) += leftVal;
          _probeHoG_F32(row,col_R) += rightVal;
          
          histSums[row] += leftVal + rightVal;
        } // for j
      } // for i
      
      // Normalize each histogram at this scale to sum to one
      for(s32 row=0; row<_probeHoG_F32.rows; ++row) {
        cv::Mat_<f32> H = _probeHoG_F32(cv::Range(row,row+1),
                                        cv::Range(iScale*_numHogOrientations,
                                                  (iScale+1)*_numHogOrientations));
        H /= histSums[row];
      }
      
    } // for scale
    
    _probeHoG_F32.convertTo(_probeHoG, CV_8UC1, 255);
    
    return RESULT_OK;
    
  } // GetProbeHoG()
  
} // namespace Embedded
} // namesapce Anki