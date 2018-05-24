

#include "coretech/vision/robot/nearestNeighborLibrary.h"
#include "coretech/vision/robot/fiducialMarkers.h"

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "coretech/common/robot/errorHandling.h"

#include <array>

#define DRAW_DEBUG_IMAGES 0 // Requires VisionComponent to be in Synchronous mode

namespace Anki {
namespace Embedded {
  
  NearestNeighborLibrary::NearestNeighborLibrary()
  : _probeXOffsets(NULL)
  , _probeYOffsets(NULL)
  {
    
  }
  
  NearestNeighborLibrary::NearestNeighborLibrary(const std::string& dataPath,
                                                 const s32 numDataPoints, const s32 dataDim,
                                                 const s16* probeCenters_X, const s16* probeCenters_Y,
                                                 const s16* probePoints_X, const s16* probePoints_Y,
                                                 const s32 numProbePoints, const s32 numFractionalBits)
  : _probeValues(1, dataDim)
  , _data(numDataPoints, dataDim)
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(1, numDataPoints)
  , _probeXCenters(probeCenters_X)
  , _probeYCenters(probeCenters_Y)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  , _numFractionalBits(numFractionalBits)
  {
    const std::string nnLibPath(dataPath + "/nnLibrary");
   
    AnkiConditionalErrorAndReturn(dataPath.empty() == false,
                                  "NearestNeighborLibrary.Constructor.EmptyDataPath", "");
    
    std::string dataFile = nnLibPath + "/nnLibrary.bin";
    
    FILE* fp = fopen(dataFile.c_str(), "rb");
    AnkiConditionalErrorAndReturn(fp, "NearestNeighborLibrary.Constructor.MissingFile",
                                  "Unable to find NN library data file '%s'.", dataFile.c_str());
    (void)fread(_data.data, numDataPoints*dataDim, sizeof(u8), fp);
    fclose(fp);
    
    dataFile = nnLibPath + "/nnLibrary_labels.bin";
    AnkiConditionalErrorAndReturn(fp, "NearestNeighborLibrary.Constructor.MissingFile",
                                  "Unable to find NN library labels file '%s'.", dataFile.c_str());
    fp = fopen(dataFile.c_str(), "rb");
    (void)fread(_labels.data, numDataPoints, sizeof(u16), fp);
    fclose(fp);

  }
  
  NearestNeighborLibrary::NearestNeighborLibrary(const u8* data,
                                                 const u8* weights,
                                                 const u16* labels,
                                                 const s32 numDataPoints, const s32 dataDim,
                                                 const s16* probeCenters_X, const s16* probeCenters_Y,
                                                 const s16* probePoints_X, const s16* probePoints_Y,
                                                 const s32 numProbePoints, const s32 numFractionalBits)
  : _probeValues(1, dataDim)
  , _data(numDataPoints, dataDim)
  , _numDataPoints(numDataPoints)
  , _dataDimension(dataDim)
  , _labels(1, numDataPoints, const_cast<u16*>(labels))
  , _probeXCenters(probeCenters_X)
  , _probeYCenters(probeCenters_Y)
  , _probeXOffsets(probePoints_X)
  , _probeYOffsets(probePoints_Y)
  , _numProbeOffsets(numProbePoints)
  , _numFractionalBits(numFractionalBits)
  {
    const cv::Mat_<u8> temp(numDataPoints, dataDim, const_cast<u8*>(data));
    temp.copyTo(_data);
  }
  
  Result NearestNeighborLibrary::GetNearestNeighbor(const Array<u8> &image, const Array<f32> &homography, const s32 distThreshold, s32 &label, s32 &closestDistance)
  {
    label = -1;
    s32 closestIndex = -1;
    s32 secondClosestIndex = -1;
  
    Result lastResult = VisionMarker::GetProbeValues(image, homography,
                                                     false, //USE_ILLUMINATION_NORMALIZATION,
                                                     _probeValues);
    
    AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                       "NearestNeighborLibrary.GetNearestNeighbor.GetProbesFailed",
                                       "");
    
    // Start with closest distance set to the threshold so if nothing is
    // below that threshold, closestIndex will never get set
    closestDistance = distThreshold;
    s32 secondClosestDistance = std::numeric_limits<s32>::max();
    
    cv::normalize(_probeValues, _probeValues, 255, 0, CV_MINMAX);
    
    cv::Mat_<u8> diffImage, bestDiffImage, secondBestDiffImage;
    for(s32 iExample=0; iExample<_numDataPoints; ++iExample)
    {
      cv::absdiff(_probeValues, _data.row(iExample), diffImage);
      
      const s32 currentDistance = cv::sum(diffImage)[0] / _dataDimension;
      
      if(currentDistance < closestDistance)
      {
        secondClosestIndex = closestIndex;
        secondClosestDistance = closestDistance;
        closestDistance = currentDistance;
        closestIndex = iExample;
        if(DRAW_DEBUG_IMAGES)
        {
          std::swap(bestDiffImage, secondBestDiffImage);
          diffImage.reshape(0, VisionMarker::GRIDSIZE).copyTo(bestDiffImage);
        }
      }
      else if(currentDistance < secondClosestDistance)
      {
        secondClosestIndex = iExample;
        secondClosestDistance = currentDistance;
        if(DRAW_DEBUG_IMAGES)
        {
          diffImage.reshape(0, VisionMarker::GRIDSIZE).copyTo(secondBestDiffImage);
        }
      }
    }
  
    if(closestIndex != -1)
    {
      const s32 closestLabel = _labels.at<s16>(closestIndex);
      const s32 secondLabel = (secondClosestIndex >= 0 ? _labels.at<s16>(secondClosestIndex) : -1);
      
      s32 maskedDist = -1;
      if(secondClosestIndex == -1 || closestLabel == secondLabel) {
        // Top two labels the same, nothing more to check
        label = closestLabel;
      } else {
        // Find where the two top examples differ and check only those probes for
        // change
        const u8* restrict example1 = _data[closestIndex];
        const u8* restrict example2 = _data[secondClosestIndex];
        const u8* restrict probeData = _probeValues[0];
        s32 count = 0;
        maskedDist = 0;
        for(s32 i=0; i<_dataDimension; ++i)
        {
          if(std::abs((s32)(example1[i]) - (s32)(example2[i])) > distThreshold)
          {
            maskedDist += std::abs((s32)(probeData[i]) - (s32)(example1[i]));
            ++count;
          }
        }

        // Allow a bit more average variation since we're looking at a smaller
        // number of probes
        const f32 distThreshLeniency = 1.25f;
        
        if(count == 0)
        {
          maskedDist = 0;
        }
        else
        {
          maskedDist /= count;
        }
        
        if(maskedDist < distThreshLeniency*distThreshold) {
          label = closestLabel;
        }
      }
      
      if(DRAW_DEBUG_IMAGES)
      {
        const s32 dispSize = 256;
 
        cv::Mat probeDisp;
        cv::resize(_probeValues.reshape(0, VisionMarker::GRIDSIZE), probeDisp,
                   cv::Size(dispSize,dispSize));
        cv::imshow("Probes", probeDisp);
        
        cv::Mat closestExampleDisp;
        cv::Mat_<u8> tempClosestExample(VisionMarker::GRIDSIZE, VisionMarker::GRIDSIZE,
                                        _data[closestIndex]);
        cv::resize(tempClosestExample, closestExampleDisp, cv::Size(dispSize,dispSize));
        cv::cvtColor(closestExampleDisp, closestExampleDisp, CV_GRAY2BGR);
      
        cv::resize(bestDiffImage, bestDiffImage, cv::Size(dispSize,dispSize));
        cv::imshow("ClosestDiff", bestDiffImage);
        
        char closestStr[128];
        snprintf(closestStr, 127, "Dist: %d(%d), Index: %d, Label: %d",
                 closestDistance, maskedDist, closestIndex, closestLabel);
        cv::putText(closestExampleDisp, closestStr, cv::Point(0,closestExampleDisp.rows-2), CV_FONT_NORMAL, 0.4, cv::Scalar(0,0,255));
        cv::imshow("ClosestExample", closestExampleDisp);
        
        if(secondClosestIndex != -1)
        {
          cv::resize(secondBestDiffImage, secondBestDiffImage, cv::Size(dispSize,dispSize));
          cv::imshow("SecondDiff", secondBestDiffImage);
          
          cv::Mat secondClosestDisp;
          cv::Mat_<u8> tempSecondExample(VisionMarker::GRIDSIZE, VisionMarker::GRIDSIZE,
                                          _data[secondClosestIndex]);
          cv::resize(tempSecondExample, secondClosestDisp, cv::Size(dispSize,dispSize));
          cv::cvtColor(secondClosestDisp, secondClosestDisp, CV_GRAY2BGR);
          
          cv::Mat maskDisp;
          cv::absdiff(tempClosestExample, tempSecondExample, maskDisp);
          cv::resize(maskDisp>distThreshold, maskDisp, cv::Size(dispSize, dispSize), CV_INTER_NN);
          cv::imshow("ExampleDiffMask", maskDisp);
          
          char closestStr[128];
          snprintf(closestStr, 127, "Dist: %d, Index: %d, Label: %d",
                   secondClosestDistance, secondClosestIndex, secondLabel);
          cv::putText(secondClosestDisp, closestStr, cv::Point(0,secondClosestDisp.rows-2), CV_FONT_NORMAL, 0.4, cv::Scalar(0,0,255));
          cv::imshow("SecondExample", secondClosestDisp);
        }
        
        cv::waitKey(1);
      }
    }
    return lastResult;
  }
  
} // namespace Embedded
} // namespace Anki
