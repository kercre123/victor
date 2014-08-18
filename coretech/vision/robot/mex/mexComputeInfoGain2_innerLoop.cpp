// Example:
// [bestEntropy, bestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel, 1)

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

// Threading includes and defines
#ifdef _MSC_VER
#include <tchar.h>
#include <strsafe.h>
#include <windows.h>
#define SimpleMutex HANDLE
typedef HANDLE ThreadHandle;
#define ThreadResult DWORD WINAPI
#else // #ifdef _MSC_VER
#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#define SimpleMutex pthread_mutex_t
typedef pthread_t ThreadHandle;
#define ThreadResult void*
#endif // #ifdef _MSC_VER ... #else

using namespace Anki::Embedded;

#if defined(_MSC_VER)
const f32 log2DivisorF32 = 1.0f / log(2.0f);
static inline f32 log2(const f32 x)
{
  return log(x) * log2DivisorF32;
}

const f64 log2DivisorF64 = 1.0 / log(2.0);
static inline f64 log2(const f64 x)
{
  return log(x) * log2DivisorF64;
}
#endif

#define PRECISION f64

typedef struct
{
  // Thread-independent inputs
  s32 maxLabel;
  const u8 * restrict pGrayvalueThresholds;
  s32 numItems;
  const s32 * restrict pCurLabels;
  const u8 * restrict pCurProbeValues;

  // Thread-specific inputs
  s32 minGrayvalueThresholdIndex;
  s32 maxGrayvalueThresholdIndex;
  s32 * restrict pNumLessThan; //< Must be allocated before calling the thread, and freed after the thread is complete
  s32 * restrict pNumGreaterThan; //< Must be allocated before calling the thread, and freed after the thread is complete

  // Outputs
  PRECISION bestEntropy;
  u8 bestGrayvalueThreshold;
} MainLoopParameters;

static ThreadResult MainLoop(void *threadParameter)
{
  MainLoopParameters * restrict parameters = (MainLoopParameters*) threadParameter;

  parameters->bestEntropy = FLT_MAX;
  parameters->bestGrayvalueThreshold = 0;

  for(s32 iGrayvalueThreshold=parameters->minGrayvalueThresholdIndex; iGrayvalueThreshold<=parameters->maxGrayvalueThresholdIndex; iGrayvalueThreshold++) {
    const u8 curGrayvalueThreshold = parameters->pGrayvalueThresholds[iGrayvalueThreshold];

    memset(parameters->pNumLessThan, 0, (parameters->maxLabel+1)*sizeof(s32));
    memset(parameters->pNumGreaterThan, 0, (parameters->maxLabel+1)*sizeof(s32));

    s32 totalNumLessThan = 0;
    s32 totalNumGreaterThan = 0;

    //labelsLessThan = curLabels(curProbeValues < grayvalueThresholds(iGrayvalueThreshold));
    //labelsGreaterThan = curLabels(curProbeValues >= grayvalueThresholds(iGrayvalueThreshold));
    // [valuesLessThan, countsLessThan] = count_unique(labelsLessThan);
    // [valuesGreaterThan, countsGreaterThan] = count_unique(labelsGreaterThan);
    //totalNumLessThan = sum(countsLessThan);
    //totalNumGreaterThan = sum(countsGreaterThan);

    for(s32 iItem=0; iItem<parameters->numItems; iItem++) {
      const s32 curLabel = parameters->pCurLabels[iItem];

      if(parameters->pCurProbeValues[iItem] < curGrayvalueThreshold) {
        totalNumLessThan++;
        parameters->pNumLessThan[curLabel]++;
      } else {
        totalNumGreaterThan++;
        parameters->pNumGreaterThan[curLabel]++;
      }
    }

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

    for(s32 iLabel=0; iLabel<=parameters->maxLabel; iLabel++) {
      //probabilitiesLessThan = allValuesLessThan / sum(allValuesLessThan);
      //entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
      if(parameters->pNumLessThan[iLabel] > 0) {
        const PRECISION probability = static_cast<PRECISION>(parameters->pNumLessThan[iLabel]) * inverseTotalNumLessThan;
        entropyLessThan -= probability * log2(probability);
      }

      //probabilitiesGreaterThan = allValuesGreaterThan / sum(allValuesGreaterThan);
      //entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
      if(parameters->pNumGreaterThan[iLabel] > 0) {
        const PRECISION probability = static_cast<PRECISION>(parameters->pNumGreaterThan[iLabel]) * inverseTotalNumGreaterThan;
        entropyGreaterThan -= probability * log2(probability);
      }
    } // for(s32 iLabel=0; iLabel<=maxLabel; iLabel++)

    const PRECISION percent_lessThan    = static_cast<PRECISION>(totalNumLessThan)    / static_cast<PRECISION>(totalNumLessThan + totalNumGreaterThan);
    const PRECISION percent_greaterThan = static_cast<PRECISION>(totalNumGreaterThan) / static_cast<PRECISION>(totalNumLessThan + totalNumGreaterThan);

    const PRECISION weightedAverageEntropy = percent_lessThan * entropyLessThan + percent_greaterThan * entropyGreaterThan;

    if(weightedAverageEntropy < parameters->bestEntropy) {
      parameters->bestEntropy = weightedAverageEntropy;
      parameters->bestGrayvalueThreshold = curGrayvalueThreshold;
    }
  } // for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++)

  return 0;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  const s32 MAX_THREADS = 32;

  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 5 && nlhs == 2, "mexComputeInfoGain2_innerLoop", "Call this function as follows: [bestEntropy, bestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel, numThreads);");

  const s32 * restrict pCurLabels = reinterpret_cast<s32 *>( mxGetData(prhs[0]) );
  const u8 * restrict pCurProbeValues = reinterpret_cast<u8 *>( mxGetData(prhs[1]) );
  const u8 * restrict pGrayvalueThresholds = reinterpret_cast<u8 *>( mxGetData(prhs[2]) );
  const s32 maxLabel = saturate_cast<s32>(mxGetScalar(prhs[3]));
  s32 numThreads = saturate_cast<s32>(mxGetScalar(prhs[4]));

  AnkiConditionalErrorAndReturn(mxGetN(prhs[0]) == 1 && mxGetN(prhs[1]) == 1 && mxGetN(prhs[2]) == 1, "mexComputeInfoGain2_innerLoop", "Incorrect input size");

  AnkiConditionalErrorAndReturn(mxGetM(prhs[0]) == mxGetM(prhs[1]), "mexComputeInfoGain2_innerLoop", "Incorrect input size");
  
  AnkiConditionalErrorAndReturn(mxGetM(prhs[0]) > 0 && mxGetM(prhs[2]) > 0, "mexComputeInfoGain2_innerLoop", "Incorrect input size");

  const s32 numItems = mxGetM(prhs[0]);
  const s32 numGrayvalueThresholds = mxGetM(prhs[2]);

  const s32 numWorkItems = numItems * numGrayvalueThresholds;

  numThreads = MAX(1, MIN(numThreads, MAX_THREADS));

  // TODO: figure out a good threshold
  if(numWorkItems < 1000) {
    numThreads = 1;
  }

  numThreads = MIN(numThreads, numGrayvalueThresholds);

  PRECISION bestEntropy = FLT_MAX;
  u8 bestGrayvalueThreshold = 0;

  if(numThreads == 1) {
    MainLoopParameters parameters;

    parameters.maxLabel = maxLabel;
    parameters.pGrayvalueThresholds = pGrayvalueThresholds;
    parameters.numItems = numItems;
    parameters.pCurLabels = pCurLabels;
    parameters.pCurProbeValues = pCurProbeValues;

    parameters.minGrayvalueThresholdIndex = 0;
    parameters.maxGrayvalueThresholdIndex = numGrayvalueThresholds-1;

    parameters.pNumLessThan = reinterpret_cast<s32*>(mxMalloc((parameters.maxLabel+1)*sizeof(s32)));
    parameters.pNumGreaterThan = reinterpret_cast<s32*>(mxMalloc((parameters.maxLabel+1)*sizeof(s32)));

    MainLoop(&parameters);

    bestEntropy = parameters.bestEntropy;
    bestGrayvalueThreshold = parameters.bestGrayvalueThreshold;

    mxFree(parameters.pNumLessThan);
    mxFree(parameters.pNumGreaterThan);
  } else {
    MainLoopParameters parameters[MAX_THREADS];
    ThreadHandle threadHandles[MAX_THREADS];

    const s32 numGrayvalueThresholdsPerThread = (numGrayvalueThresholds + numThreads - 1) / numThreads;

    // Spawn the desired number of threads
    for(s32 iThread=0; iThread<numThreads; iThread++) {
      parameters[iThread].maxLabel = maxLabel;
      parameters[iThread].pGrayvalueThresholds = pGrayvalueThresholds;
      parameters[iThread].numItems = numItems;
      parameters[iThread].pCurLabels = pCurLabels;
      parameters[iThread].pCurProbeValues = pCurProbeValues;

      parameters[iThread].minGrayvalueThresholdIndex = iThread * numGrayvalueThresholdsPerThread;
      parameters[iThread].maxGrayvalueThresholdIndex = MIN(numGrayvalueThresholds - 1, (iThread+1) * numGrayvalueThresholdsPerThread - 1);

      parameters[iThread].pNumLessThan = reinterpret_cast<s32*>(mxMalloc((parameters[iThread].maxLabel+1)*sizeof(s32)));
      parameters[iThread].pNumGreaterThan = reinterpret_cast<s32*>(mxMalloc((parameters[iThread].maxLabel+1)*sizeof(s32)));

#ifdef _MSC_VER
      DWORD threadId = -1;
      threadHandles[iThread] = CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        MainLoop, // thread function name
        &parameters[iThread],    // argument to thread function
        0,           // use default creation flags
        &threadId);  // returns the thread identifier
#else
      pthread_attr_t connectionAttr;
      pthread_attr_init(&connectionAttr);

      pthread_create(&(threadHandles[iThread]), &connectionAttr, MainLoop, (void *)(&parameters[iThread]));
#endif
    }

    // Wait for the threads to complete and combine the results
    for(s32 iThread=0; iThread<numThreads; iThread++) {
#ifdef _MSC_VER
      WaitForSingleObject(threadHandles[iThread], INFINITE);
#else
      pthread_join(threadHandles[iThread], NULL);
#endif

      if(parameters[iThread].bestEntropy < bestEntropy) {
        bestEntropy = parameters[iThread].bestEntropy;
        bestGrayvalueThreshold = parameters[iThread].bestGrayvalueThreshold;
      }

      mxFree(parameters[iThread].pNumLessThan);
      mxFree(parameters[iThread].pNumGreaterThan);
    }
  } // if(numThreads == 1) ... else

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
