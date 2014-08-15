#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/fiducialDetection.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>
#include <cmath>
#include <math.h>

using namespace Anki::Embedded;

// Example:
// [bestEntropy, bestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel)

#if defined(_MSC_VER)
const f32 log2DivisorF32 = 1.0f / log(2.0f);
static inline f32 log2(const f32 x)
{
  return x * log2DivisorF32;
}

const f64 log2DivisorF64 = 1.0 / log(2.0);
static inline f64 log2(const f64 x)
{
  return x * log2DivisorF64;
}
#endif

#define PRECISION f64

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);
  
  AnkiConditionalErrorAndReturn(nrhs == 4 && nlhs == 2, "mexComputeInfoGain2_innerLoop", "Call this function as following: [bestEntropy, bestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel);");
  
  const s32 * restrict pCurLabels = reinterpret_cast<s32 *>( mxGetData(prhs[0]) );
  const u8 * restrict pCurProbeValues = reinterpret_cast<u8 *>( mxGetData(prhs[1]) );
  const u8 * restrict pGrayvalueThresholds = reinterpret_cast<u8 *>( mxGetData(prhs[2]) );
  const s32 maxLabel = saturate_cast<s32>(mxGetScalar(prhs[3]));
  
  AnkiConditionalErrorAndReturn(mxGetN(prhs[0]) == 1 && mxGetN(prhs[1]) == 1 && mxGetN(prhs[2]) == 1, "mexComputeInfoGain2_innerLoop", "Incorrect input size");
  
  AnkiConditionalErrorAndReturn(mxGetM(prhs[0]) == mxGetM(prhs[1]), "mexComputeInfoGain2_innerLoop", "Incorrect input size");
  
  const s32 numItems = mxGetM(prhs[0]);
  const s32 numGrayvalueThresholds = mxGetM(prhs[2]);
  
  s32 * restrict pNumLessThan = reinterpret_cast<s32*>(mxMalloc((maxLabel+1)*sizeof(s32)));
  s32 * restrict pNumGreaterThan = reinterpret_cast<s32*>(mxMalloc((maxLabel+1)*sizeof(s32)));
  
  PRECISION bestEntropy = FLT_MAX;
  u8 bestGrayvalueThreshold = 0;
  
  for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++) {
    const u8 curGrayvalueThreshold = pGrayvalueThresholds[iGrayvalueThreshold];
    
    memset(pNumLessThan, 0, (maxLabel+1)*sizeof(s32));
    memset(pNumGreaterThan, 0, (maxLabel+1)*sizeof(s32));
    
    s32 totalNumLessThan = 0;
    s32 totalNumGreaterThan = 0;
    
    //labelsLessThan = curLabels(curProbeValues < grayvalueThresholds(iGrayvalueThreshold));
    //labelsGreaterThan = curLabels(curProbeValues >= grayvalueThresholds(iGrayvalueThreshold));
    // [valuesLessThan, countsLessThan] = count_unique(labelsLessThan);
    // [valuesGreaterThan, countsGreaterThan] = count_unique(labelsGreaterThan);
    //totalNumLessThan = sum(countsLessThan);
    //totalNumGreaterThan = sum(countsGreaterThan);
    for(s32 iItem=0; iItem<numItems; iItem++) {
      const s32 curLabel = pCurLabels[iItem];
      
      if(pCurProbeValues[iItem] < curGrayvalueThreshold) {
        totalNumLessThan++;
        pNumLessThan[curLabel]++;
      } else {
        totalNumGreaterThan++;
        pNumGreaterThan[curLabel]++;
      }
    }
    
//    mexPrintf("%d %d\n", totalNumLessThan, totalNumGreaterThan);
    
    //if isempty(labelsLessThan) || isempty(labelsGreaterThan)
    //  continue
    //  end
    if(totalNumLessThan == 0 || totalNumGreaterThan == 0) {
      continue;
    }
    
    //allValuesLessThan = zeros(maxLabel, 1);
    //allValuesGreaterThan = zeros(maxLabel, 1);
    
    //allValuesLessThan(valuesLessThan) = countsLessThan;
    //allValuesGreaterThan(valuesGreaterThan) = countsGreaterThan;
    
    PRECISION entropyLessThan = 0;
    PRECISION entropyGreaterThan = 0;
    
    const PRECISION inverseTotalNumLessThan = static_cast<PRECISION>(1) / static_cast<PRECISION>(totalNumLessThan);
    const PRECISION inverseTotalNumGreaterThan = static_cast<PRECISION>(1) / static_cast<PRECISION>(totalNumGreaterThan);
    
    for(s32 iLabel=0; iLabel<=maxLabel; iLabel++) {
      //probabilitiesLessThan = allValuesLessThan / sum(allValuesLessThan);
      //entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
      if(pNumLessThan[iLabel] > 0) {
        const PRECISION probability = static_cast<PRECISION>(pNumLessThan[iLabel]) * inverseTotalNumLessThan;
        entropyLessThan -= probability * log2(probability);
      }
      
      //probabilitiesGreaterThan = allValuesGreaterThan / sum(allValuesGreaterThan);
      //entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
      if(pNumGreaterThan[iLabel] > 0) {
        const PRECISION probability = static_cast<PRECISION>(pNumGreaterThan[iLabel]) * inverseTotalNumGreaterThan;
        entropyGreaterThan -= probability * log2(probability);
      }
    }
    
    const PRECISION percent_lessThan    = static_cast<PRECISION>(totalNumLessThan)    / static_cast<PRECISION>(totalNumLessThan + totalNumGreaterThan);
    const PRECISION percent_greaterThan = static_cast<PRECISION>(totalNumGreaterThan) / static_cast<PRECISION>(totalNumLessThan + totalNumGreaterThan);
    
    const PRECISION weightedAverageEntropy = percent_lessThan * entropyLessThan + percent_greaterThan * entropyGreaterThan;
    
    if(weightedAverageEntropy < bestEntropy) {
      bestEntropy = weightedAverageEntropy;
      bestGrayvalueThreshold = curGrayvalueThreshold;
    }
  } // for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++)
  
  {
    const mwSize outputDims[2] = {1, 1};
    plhs[0] = mxCreateNumericArray(2, outputDims, mxSINGLE_CLASS, mxREAL);
    f32 * const matlabMatrixStartPointer = (f32 *) mxGetData(plhs[0]);
    matlabMatrixStartPointer[0] = static_cast<f32>(bestEntropy);
  }
  
  {
    const mwSize outputDims[2] = {1, 1};
    plhs[1] = mxCreateNumericArray(2, outputDims, mxUINT8_CLASS, mxREAL);
    u8 * const matlabMatrixStartPointer = (u8 *) mxGetData(plhs[1]);
    matlabMatrixStartPointer[0] = bestGrayvalueThreshold;
  }
} // mexFunction()
