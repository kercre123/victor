// Example:
//

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

#include "anki/tools/threads/threadSafeUtilities.h"

using namespace Anki::Embedded;


static void runTree(
                    const u8  * restrict pFeatureValues,
                    const s32 * restrict pWhichFeatures,
                    const u8  * restrict pU8Thresholds,
                    const s32 * restrict pLeftChildIndexs,
                    const s32 * restrict pSamplePositions,
                    s32 * restrict pLabelIds,
                    const s32 numFeatures,
                    const s32 numImages,
                    const s32 treeLength)
{
  for(s32 iImage=0; iImage<numImages; iImage++) {
    const u8 * restrict pFeatureValuesCur = pFeatureValues + iImage * numFeatures;
    
    s32 curNodeIndex = 0;
    
    while(pLeftChildIndexs[curNodeIndex] > 0) {
      const s32 curFeature = pWhichFeatures[curNodeIndex];
      const s32 imageIndex = pSamplePositions[curFeature];
      const u8 imageValue = pFeatureValuesCur[imageIndex];
      const u8 curU8Threshold = pU8Thresholds[curNodeIndex];
      const s32 curLeftChildIndex = pLeftChildIndexs[curNodeIndex];
    
      if(imageValue < curU8Threshold) {
        curNodeIndex = curLeftChildIndex;
      } else {
        curNodeIndex = curLeftChildIndex + 1;
      }
    } // while(pLeftChildIndexs[curNodeIndex] >= 0)
    
    const s32 curLeftChildIndex = pLeftChildIndexs[curNodeIndex];
    
    AnkiAssert(curLeftChildIndex < 0);
    
    if(curLeftChildIndex == -1) {
      pLabelIds[iImage] = -1;
    } else {
      pLabelIds[iImage] = (-curLeftChildIndex) - 1000000 + 1;
    }
  } // for(s32 iImage=0; iImage<numImages; iImage++)
} // void runTree()

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 6 && nlhs == 1, "mexDecisionTree2_testOnTrainingData_innerLoop", "Call this function as follows: labelIds = mexDecisionTree2_testOnTrainingData_innerLoop(featureValues, cTree_whichFeatures, cTree_u8Thresholds, cTree_leftChildIndexs, samplePositions, numThreads);");

  const s32 numFeatures = mxGetM(prhs[0]);
  const s32 numImages = mxGetN(prhs[0]);
  const s32 treeLength = mxGetM(prhs[1]);
  
  AnkiConditionalErrorAndReturn(mxGetN(prhs[1]) == 1 && mxGetN(prhs[2]) == 1 && mxGetN(prhs[3]) == 1, "mexDecisionTree2_testOnTrainingData_innerLoop", "Incorrect input size");
  
  AnkiConditionalErrorAndReturn(
                                treeLength == mxGetM(prhs[1]) &&
                                treeLength == mxGetM(prhs[2]) &&
                                treeLength == mxGetM(prhs[3]) &&
                                numFeatures == mxGetM(prhs[4]), "mexDecisionTree2_testOnTrainingData_innerLoop", "Incorrect input size");
  
  AnkiConditionalErrorAndReturn(mxGetM(prhs[0]) > 0, "mexDecisionTree2_testOnTrainingData_innerLoop", "Incorrect input size");

  // NOTE: Theoretically, this could be reasonable, and not due to a transpose mistake
  AnkiConditionalErrorAndReturn(numFeatures < numImages, "mexDecisionTree2_testOnTrainingData_innerLoop", "Possible transpose mistake");
  
  AnkiConditionalErrorAndReturn(
                                mxGetClassID(prhs[0])==mxUINT8_CLASS &&
                                mxGetClassID(prhs[1])==mxINT32_CLASS &&
                                mxGetClassID(prhs[2])==mxUINT8_CLASS &&
                                mxGetClassID(prhs[3])==mxINT32_CLASS &&
                                mxGetClassID(prhs[4])==mxINT32_CLASS, "mexDecisionTree2_testOnTrainingData_innerLoop", "Incorrect input types");
  
  const u8  * restrict pFeatureValues = reinterpret_cast<u8 *>( mxGetData(prhs[0]) );
  const s32 * restrict pWhichFeatures = reinterpret_cast<s32 *>( mxGetData(prhs[1]) );
  const u8  * restrict pU8Thresholds = reinterpret_cast<u8 *>( mxGetData(prhs[2]) );
  const s32 * restrict pLeftChildIndexs = reinterpret_cast<s32 *>( mxGetData(prhs[3]) );
  const s32 * restrict pSamplePositions = reinterpret_cast<s32 *>( mxGetData(prhs[4]) );
  const s32 numThreads = saturate_cast<s32>(mxGetScalar(prhs[5]));

  const mwSize outputDims[2] = {static_cast<mwSize>(numImages), 1};
  plhs[0] = mxCreateNumericArray(2, outputDims, mxINT32_CLASS, mxREAL);
  s32 * const pLabelIds = (s32 *) mxGetData(plhs[0]);

  runTree(
    pFeatureValues,
    pWhichFeatures,
    pU8Thresholds,
    pLeftChildIndexs,
    pSamplePositions,
    pLabelIds,
    numFeatures,
    numImages,
    treeLength);
  
} // mexFunction()


