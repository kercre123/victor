/**
File: trainDecisionTree.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "trainDecisionTree.h"
#include "anki/tools/threads/threadSafeCounter.h"
#include "anki/tools/threads/threadSafeFixedLengthList.h"
#include "anki/tools/threads/threadSafeQueue.h"
#include "anki/tools/threads/threadSafeVector.h"

#include <vector>
#include <queue>

using namespace std;
using namespace Anki;
using namespace Anki::Embedded;

const s32 MAX_THREADS = 128; // Max threads
const s32 MIN_IMAGES_PER_THREAD = 10; // If the number of images left is below this, one one thread will be used to compute the entropy
const s32 BUSY_WAIT_SLEEP_MICROSECONDS = 100000;

//#define PRINT_INTERMEDIATE
//#define PRINT_FAILURES

// A list of UniqueCounts holds the number of instances of each value, like an integer histogram
typedef struct
{
  s32 value;
  s32 count;
} UniqueCount;

// Hold an example of a training failure, where the tree couldn't split some examples with different labels
typedef struct TrainingFailure
{
  s32 nodeIndex;
  vector<UniqueCount> labels;

  TrainingFailure()
    : nodeIndex(-1)
  {
  }
} TrainingFailure;

template<typename Type> RawBuffer<Type>::RawBuffer()
  : buffer(NULL), size(0)
{
}

template<typename Type> RawBuffer<Type>::RawBuffer(s32 size)
{
  AnkiAssert(size > 0);

  this->buffer = reinterpret_cast<Type*>( malloc(size*sizeof(Type)) );
  this->size = size;

  AnkiAssert(this->buffer);
}

template<typename Type> void RawBuffer<Type>::Clone(const RawBuffer &in)
{
  AnkiAssert(in.size > 0);

  this->size = in.size;
  this->buffer = reinterpret_cast<Type*>( malloc(this->size*sizeof(Type)) );

  AnkiAssert(this->buffer);

  for(s32 i=0; i<this->size; i++) {
    this->buffer[i] = in.buffer[i];
  }
}

template<typename Type> void RawBuffer<Type>::Free()
{
  if(this->buffer) {
    free(this->buffer);
    this->buffer = NULL;
  }
}

typedef struct DecisionTreeWorkItem
{
  s32 treeIndex;
  RawBuffer<s32> remaining; // remaining: [1x552000 int32];
  RawBuffer<U8Bool> featuresUsed;

  DecisionTreeWorkItem(s32 treeIndex, const RawBuffer<s32> &remaining, const RawBuffer<U8Bool> &featuresUsed)
    : treeIndex(treeIndex), remaining(remaining), featuresUsed(featuresUsed)
  {
  }
} DecisionTreeWorkItem;

typedef struct BuildTreeThreadParameters
{
  const vector<U8Bool> &featuresUsed; //< numFeatures x 256
  const FixedLengthList<const char *> &labelNames; //< Lookup table between index and string name
  const FixedLengthList<s32> &labels; //< The label for every item to train on (very large)
  const FixedLengthList<const FixedLengthList<u8> > &featureValues; //< For every feature, the value for the feature every item to train on (outer is small, inner is very large)
  const f32 leafNodeFraction; //< What percent of the items in a node have to be the same for it to be considered a leaf node? From [0.0, 1.0], where 1.0 is a good value.
  const s32 leafNodeNumItems; //< If the number of items in a node is equal or below this, it is a leaf. 1 is a good value.
  const s32 u8MinDistanceForSplits; //< How close can two grayvalues be to be a threshold? 100 is a good value.
  const s32 u8MinDistanceFromThreshold; //< If a feature value is closer than this to the threshold, pass it to both subpaths
  const FixedLengthList<u8> &u8ThresholdsToUse; //< If not empty, this is the list of grayvalue thresholds to use
  const s32 maxSecondaryThreads; //< If we are building the start of the tree, use one primary thread, and maxSecondaryThreads threads to compute the information fain
  const std::vector<s32*> &pNumLTs; //< One allocated buffer for each maxSecondaryThreads. Must be allocated before calling the thread, and manually freed after the thread is complete
  const std::vector<s32*> &pNumGEs; //< One allocated buffer for each maxSecondaryThreads. Must be allocated before calling the thread, and manually freed after the thread is complete
  const s32 maxLabel; //< What is the max label in labels?
  const s32 threadId; //< What is the id of the thread?
  const s32 maxNodesToProcess; //< How many nodes should this thread process? Set to -1 to process until the queue is empty.
  ThreadSafeQueue<DecisionTreeWorkItem> &workQueue; //< What work needs to be done? Starts with just the root node.
  ThreadSafeVector<TrainingFailure> &trainingFailures; //< Which nodes could not be trained?
  ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree; //< The output decision tree
  ThreadSafeCounter<s32> &numCompleted; //< How many images have been assigned to a leaf node? If it is equal to labels.get_size(), then we're finished.

  BuildTreeThreadParameters(
    const vector<U8Bool> &featuresUsed,
    const FixedLengthList<const char *> &labelNames,
    const FixedLengthList<s32> &labels,
    const FixedLengthList<const FixedLengthList<u8> > &featureValues,
    const f32 leafNodeFraction,
    const s32 leafNodeNumItems,
    const s32 u8MinDistanceForSplits,
    const s32 u8MinDistanceFromThreshold,
    const FixedLengthList<u8> &u8ThresholdsToUse,
    const s32 maxSecondaryThreads,
    const std::vector<s32*> &pNumLTs,
    const std::vector<s32*> &pNumGEs,
    const s32 maxLabel,
    const s32 threadId,
    const s32 maxNodesToProcess,
    ThreadSafeQueue<DecisionTreeWorkItem> &workQueue,
    ThreadSafeVector<TrainingFailure> &trainingFailures,
    ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree,
    ThreadSafeCounter<s32> &numCompleted)
    : featuresUsed(featuresUsed), labelNames(labelNames), labels(labels), featureValues(featureValues), leafNodeFraction(leafNodeFraction), leafNodeNumItems(leafNodeNumItems), u8MinDistanceForSplits(u8MinDistanceForSplits), u8MinDistanceFromThreshold(u8MinDistanceFromThreshold), u8ThresholdsToUse(u8ThresholdsToUse), maxSecondaryThreads(maxSecondaryThreads), pNumLTs(pNumLTs), pNumGEs(pNumGEs), maxLabel(maxLabel), threadId(threadId), maxNodesToProcess(maxNodesToProcess), workQueue(workQueue), trainingFailures(trainingFailures), decisionTree(decisionTree), numCompleted(numCompleted)
  {
  }
} BuildTreeThreadParameters;

typedef struct BenchmarkingParameters
{
  volatile bool * isRunning;
  const f64 sampleEveryNSeconds;
  std::vector<f32> &cpuUsage;
  ThreadSafeCounter<s32> &numCompleted;
  const s32 numTotal;
  const ThreadSafeQueue<DecisionTreeWorkItem> &workQueue;

  BenchmarkingParameters(
    volatile bool * isRunning,
    const f64 sampleEveryNSeconds,
    std::vector<f32> &cpuUsage,
    ThreadSafeCounter<s32> &numCompleted,
    const s32 numTotal,
    const ThreadSafeQueue<DecisionTreeWorkItem> &workQueue)
    : isRunning(isRunning), sampleEveryNSeconds(sampleEveryNSeconds), cpuUsage(cpuUsage), numCompleted(numCompleted), numTotal(numTotal), workQueue(workQueue)
  {
  }
} BenchmarkingParameters;

template<typename Type> void maxAndMin(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, Type &minValue, Type &maxValue);

vector<UniqueCount> findUnique(const s32 * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const s32 minValue, const s32 maxValue);

ThreadResult BuildTreeThread(void *voidParameters);

template<typename Type> void maxAndMin(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, Type &minValue, Type &maxValue)
{
  minValue = pArray[pRemaining[0]];
  maxValue = pArray[pRemaining[0]];

  for(s32 i=1; i<numRemaining; i++) {
    minValue = MIN(minValue, pArray[pRemaining[i]]);
    maxValue = MAX(maxValue, pArray[pRemaining[i]]);
  }
}

vector<UniqueCount> findUnique(const s32 * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const s32 minValue, const s32 maxValue)
{
  const s32 countsLength = maxValue - minValue + 1;

  s32 * counts = reinterpret_cast<s32*>( malloc(countsLength*sizeof(s32)) );

  memset(counts, 0, countsLength*sizeof(s32));

  for(s32 i=0; i<numRemaining; i++) {
    const s32 curValue = pArray[pRemaining[i]] - minValue;
    counts[curValue]++;
  }

  s32 numUnique = 0;

  for(s32 i=0; i<countsLength; i++) {
    if(counts[i] > 0)
      numUnique++;
  }

  vector<UniqueCount> uniques;
  uniques.resize(numUnique);

  s32 cUnique = 0;
  for(s32 i=0; i<countsLength; i++) {
    if(counts[i] > 0) {
      uniques[cUnique].value = static_cast<s32>(i + minValue);
      uniques[cUnique].count = counts[i];
      cUnique++;
    }
  }

  free(counts);

  return uniques;
}

ThreadResult BuildTreeThread(void * voidBuildTreeParams)
{
  BuildTreeThreadParameters * restrict buildTreeParams = reinterpret_cast<BuildTreeThreadParameters*>(voidBuildTreeParams);

  //
  // Setup the decision tree with the root node
  //

  const s32 numFeatures = buildTreeParams->featuresUsed.size();

  ComputeInfoGainParameters *computeInfoGainParams[MAX_THREADS];
  ThreadHandle threadHandles[MAX_THREADS];

  //
  // Keep popping items from the work queue, until all images are assigned to a leaf node
  //

  s32 numNodesProcessed = 0;

  // NOTE: if some images are split more than once, there may be more than
  //       buildTreeParams->labels.get_size() images at leaves. This means that some threads may
  //       exit early.
  while(true) {
#ifdef PRINT_INTERMEDIATE
    const f64 time0 = GetTimeF64();
#endif

    // Wait for a work item and a thread to become available

    buildTreeParams->workQueue.Lock(); // Lock workQueue

    if(buildTreeParams->workQueue.Size_unsafe() == 0) {
      buildTreeParams->workQueue.Unlock(); //Unlock workQueue

      // If the queue is empty, and all images are at a leaf, exit the loop and the thread.
      // NOTE: numCompleted may be a low estimate
      if(buildTreeParams->numCompleted.Get() >= buildTreeParams->labels.get_size())
        break;

      usleep(BUSY_WAIT_SLEEP_MICROSECONDS);
      continue;
    }

    numNodesProcessed++;

    if((buildTreeParams->maxNodesToProcess > 0) && (numNodesProcessed > (buildTreeParams->maxNodesToProcess-1))) {
      buildTreeParams->workQueue.Unlock(); //Unlock workQueue
      break;
    }

    AnkiAssert(buildTreeParams->workQueue.Size_unsafe() > 0);

    DecisionTreeWorkItem workItem = buildTreeParams->workQueue.Front_unsafe();
    buildTreeParams->workQueue.Pop_unsafe();

    buildTreeParams->workQueue.Unlock(); //Unlock workQueue

    const s32 numThreadsToUse = MAX(1, MIN(buildTreeParams->maxSecondaryThreads, workItem.remaining.size / MIN_IMAGES_PER_THREAD));

    AnkiAssert(workItem.featuresUsed.size > 0 && workItem.featuresUsed.buffer);
    AnkiAssert(workItem.remaining.size > 0 && workItem.remaining.buffer);

#ifdef PRINT_INTERMEDIATE
    CoreTechPrint("Thread %d: %d secondary threads for %d items\n", buildTreeParams->threadId, numThreadsToUse, workItem.remaining.size);
#endif

    // WARNING: no other thread should write to this node, but this allows non-thread-safe access
    DecisionTreeNode &curNode = buildTreeParams->decisionTree.get_buffer()[workItem.treeIndex];

    //
    // First, check if this node is a leaf
    //

    s32 minLabel;
    s32 maxLabel;
    maxAndMin<s32>(buildTreeParams->labels.Pointer(0), workItem.remaining.buffer, workItem.remaining.size, minLabel, maxLabel);

    vector<UniqueCount> uniqueLabelCounts = findUnique(buildTreeParams->labels.Pointer(0), workItem.remaining.buffer, workItem.remaining.size, minLabel, maxLabel);

    s32 mostCommonLabel_count = 0;
    s32 mostCommonLabel = -1;

    // Find the most common label
    {
      const s32 numuniqueLabelCounts = uniqueLabelCounts.size();
      const UniqueCount * restrict pUniqueLabelCounts = uniqueLabelCounts.data();

      for(s32 i=0; i<numuniqueLabelCounts; i++) {
        if(pUniqueLabelCounts[i].count > mostCommonLabel_count) {
          mostCommonLabel_count = pUniqueLabelCounts[i].count;
          mostCommonLabel = pUniqueLabelCounts[i].value;
        }
      }
    }

    bool unusedFeaturesFound = false;

    // Check if all the features have been used
    {
      const s32 numGrayvalueThresholdsToUse = buildTreeParams->u8ThresholdsToUse.get_size();

      const U8Bool * restrict pFeaturesUsed = workItem.featuresUsed.buffer;
      for(s32 iFeature=0; iFeature<numFeatures; iFeature++) {
        if(unusedFeaturesFound) {
          break;
        }

        if(numGrayvalueThresholdsToUse > 0) {
          const u8 * restrict pGrayvalueThresholdsToUse = buildTreeParams->u8ThresholdsToUse.Pointer(0);

          for(s32 iGray=0; iGray<numGrayvalueThresholdsToUse; iGray++) {
            if(pFeaturesUsed[iFeature].values[pGrayvalueThresholdsToUse[iGray]] == 0) {
              unusedFeaturesFound = true;
              break;
            }
          }
        } else {
          for(s32 iGray=0; iGray<256; iGray++) {
            if(pFeaturesUsed[iFeature].values[iGray] == 0) {
              unusedFeaturesFound = true;
              break;
            }
          }
        }
      }
    }

    if((mostCommonLabel_count >= saturate_cast<s32>(buildTreeParams->leafNodeFraction * workItem.remaining.size)) || (static_cast<s32>(workItem.remaining.size) < buildTreeParams->leafNodeNumItems)) {
      // Are more than leafNodeFraction percent of the remaining labels the same? If so, we're done.

      curNode.leftChildIndex = -mostCommonLabel - 1000000;

      // Comment or uncomment the next line as desired
#ifdef PRINT_INTERMEDIATE
      Anki::CoreTechPrint("Thread %d: LeafNode for label = %d, or \"%s\" at depth %d\n", buildTreeParams->threadId, mostCommonLabel, buildTreeParams->labelNames[mostCommonLabel], curNode.depth);
#endif

      buildTreeParams->numCompleted.Increment(workItem.remaining.size);

      workItem.featuresUsed.Free();
      workItem.remaining.Free();

      continue;
    } else if(!unusedFeaturesFound) {
      // Have we used all features? If so, we're done.

      curNode.leftChildIndex = -mostCommonLabel - 1000000;

#ifdef PRINT_INTERMEDIATE
      Anki::CoreTechPrint("Thread %d: MaxDepth LeafNode for label = %d, or \"%s\" at depth %d\n", buildTreeParams->threadId, mostCommonLabel, buildTreeParams->labelNames[mostCommonLabel], curNode.depth);
#endif

      buildTreeParams->numCompleted.Increment(workItem.remaining.size);

      workItem.featuresUsed.Free();
      workItem.remaining.Free();

      continue;
    }

    //
    // If this isn't a leaf, find the best split
    //

    if(numThreadsToUse == 1) {
      const s32 minFeatureToCheck = 0;
      const s32 maxFeatureToCheck = numFeatures - 1;

      ComputeInfoGainParameters newParameters(
        buildTreeParams->featureValues, buildTreeParams->labels, buildTreeParams->u8ThresholdsToUse, buildTreeParams->u8MinDistanceForSplits, buildTreeParams->u8MinDistanceFromThreshold,
        workItem.remaining,
        minFeatureToCheck,
        maxFeatureToCheck,
        workItem.featuresUsed,
        buildTreeParams->pNumLTs[0],
        buildTreeParams->pNumGEs[0]);

      ComputeInfoGain(reinterpret_cast<void*>(&newParameters));

      curNode.bestEntropy = newParameters.bestEntropy;
      curNode.whichFeature = newParameters.bestFeatureIndex;
      curNode.u8Threshold = newParameters.bestU8Threshold;
      curNode.numLeft = newParameters.totalNumLT;
      curNode.numRight = newParameters.totalNumGE;
      curNode.numLeftAndRight = newParameters.totalNumBoth;
    } else { // if(numThreadsToUse == 1)
      const s32 numFeaturesPerThread = (numFeatures + numThreadsToUse - 1) / numThreadsToUse;

      for(s32 iThread=0; iThread<numThreadsToUse; iThread++) {
        computeInfoGainParams[iThread] = new ComputeInfoGainParameters(
          buildTreeParams->featureValues, buildTreeParams->labels, buildTreeParams->u8ThresholdsToUse, buildTreeParams->u8MinDistanceForSplits, buildTreeParams->u8MinDistanceFromThreshold,
          workItem.remaining,
          iThread * numFeaturesPerThread,
          MIN(numFeatures - 1, (iThread+1) * numFeaturesPerThread - 1),
          workItem.featuresUsed,
          buildTreeParams->pNumLTs[iThread],
          buildTreeParams->pNumGEs[iThread]);

        threadHandles[iThread] = CreateSimpleThread(ComputeInfoGain, computeInfoGainParams[iThread]);
      }

      for(s32 iThread=0; iThread<numThreadsToUse; iThread++) {
        // Wait for the threads to complete and combine the results

        WaitForSimpleThread(threadHandles[iThread]);

        // If the entropy is less, or the the entropy is LEQ and the mean distance is more
        if((computeInfoGainParams[iThread]->bestEntropy < curNode.bestEntropy) ||
          (computeInfoGainParams[iThread]->bestEntropy <= curNode.bestEntropy && computeInfoGainParams[iThread]->meanDistanceFromThreshold > curNode.meanDistanceFromThreshold)) {
            curNode.bestEntropy = computeInfoGainParams[iThread]->bestEntropy;
            curNode.whichFeature = computeInfoGainParams[iThread]->bestFeatureIndex;
            curNode.numLeft = computeInfoGainParams[iThread]->totalNumLT;
            curNode.numRight = computeInfoGainParams[iThread]->totalNumGE;
            curNode.numLeftAndRight = computeInfoGainParams[iThread]->totalNumBoth;
            curNode.u8Threshold = computeInfoGainParams[iThread]->bestU8Threshold;
            curNode.meanDistanceFromThreshold = computeInfoGainParams[iThread]->meanDistanceFromThreshold;
        }

        delete(computeInfoGainParams[iThread]);

#ifdef PRINT_INTERMEDIATE
        Anki::CoreTechPrint("Thread %d: Worker finished\n", buildTreeParams->threadId);
#endif
      }
    } // if(numThreadsToUse == 1) ... else

    // If bestEntropy is too large, it means we could not split the data
    if(curNode.bestEntropy > 100000.0 || curNode.whichFeature < 0) {
      // TODO: there are probably very few labels, so this is an inefficent way to compute which are unique

      curNode.leftChildIndex = -1;

      TrainingFailure failure;

      failure.nodeIndex = workItem.treeIndex;
      failure.labels = uniqueLabelCounts;

      buildTreeParams->trainingFailures.Push_back(failure);

#ifdef PRINT_FAILURES
      Anki::CoreTechPrint("Thread %d: Could not split LeafNode at depth %d for labels = {", buildTreeParams->threadId, curNode.depth);

      const s32 numLabels = failure.labels.size();
      for(s32 i=0; i<numLabels; i++) {
        Anki::CoreTechPrint("(%d, %s), ", failure.labels[i].value, buildTreeParams->labelNames[failure.labels[i].value]);
      }

      Anki::CoreTechPrint("}\n");
#endif

      // TODO: show images?

      buildTreeParams->numCompleted.Increment(workItem.remaining.size);

      workItem.featuresUsed.Free();
      workItem.remaining.Free();

      continue;
    } // if(newParameters.bestEntropy > 100000.0)

    buildTreeParams->decisionTree.Lock(); // Lock decisionTree

    buildTreeParams->decisionTree.get_buffer().set_size(buildTreeParams->decisionTree.get_buffer().get_size() + 2);

    const s32 leftNodeIndex = buildTreeParams->decisionTree.get_buffer().get_size() - 2;
    const s32 rightNodeIndex = buildTreeParams->decisionTree.get_buffer().get_size() - 1;

    buildTreeParams->decisionTree.Unlock(); // Unlock decisionTree

    buildTreeParams->decisionTree.get_buffer()[leftNodeIndex]  = DecisionTreeNode(curNode.depth + 1, FLT_MAX, -1, -1, -1, -1, -1, 0, 255);
    buildTreeParams->decisionTree.get_buffer()[rightNodeIndex] = DecisionTreeNode(curNode.depth + 1, FLT_MAX, -1, -1, -1, -1, -1, 0, 255);

    curNode.leftChildIndex = leftNodeIndex;

    //
    // Split the current remaining into left and right subsets
    //

    const s32 numRemaining = workItem.remaining.size;

    AnkiAssert(curNode.numLeft > 0 && curNode.numLeft < numRemaining && curNode.numRight > 0 && curNode.numRight < numRemaining);

    RawBuffer<s32> leftRemaining(curNode.numLeft);
    RawBuffer<s32> rightRemaining(curNode.numRight);

    const u8 * restrict pFeatureValues = buildTreeParams->featureValues[curNode.whichFeature].Pointer(0);

    const s32 * restrict pRemaining = workItem.remaining.buffer;
    s32 * restrict pLeftRemaining = leftRemaining.buffer;
    s32 * restrict pRightRemaining = rightRemaining.buffer;

    const s32 curGrayvalueThresholdS32 = curNode.u8Threshold;

    s32 cLeft = 0, cRight = 0;
    for(s32 iRemain=0; iRemain<numRemaining; iRemain++) {
      const s32 curRemaining = pRemaining[iRemain];
      const s32 curFeatureValue = pFeatureValues[curRemaining];

      // NOTE: One item may go both left and right

      if(curFeatureValue < (curGrayvalueThresholdS32 + buildTreeParams->u8MinDistanceFromThreshold)) {
        pLeftRemaining[cLeft] = curRemaining;
        cLeft++;
      }

      if(curFeatureValue >= (curGrayvalueThresholdS32 - buildTreeParams->u8MinDistanceFromThreshold)) {
        pRightRemaining[cRight] = curRemaining;
        cRight++;
      }
    }

    AnkiAssert(cLeft == curNode.numLeft);
    AnkiAssert(cRight == curNode.numRight);

    // Mask out the grayvalues that are near to the chosen threshold
    const s32 minGray = MAX(0,   curNode.u8Threshold - buildTreeParams->u8MinDistanceForSplits);
    const s32 maxGray = MIN(255, curNode.u8Threshold + buildTreeParams->u8MinDistanceForSplits);
    U8Bool &pFeaturesUsed = workItem.featuresUsed.buffer[curNode.whichFeature];
    for(s32 iGray=minGray; iGray<=maxGray; iGray++) {
      pFeaturesUsed.values[iGray] = true;
    }

#ifdef PRINT_INTERMEDIATE
    Anki::CoreTechPrint("Thread %d: Thread finished\n", buildTreeParams->threadId);
#endif

    RawBuffer<U8Bool> leftFeaturesUsed;
    RawBuffer<U8Bool> rightFeaturesUsed;

    leftFeaturesUsed.Clone(workItem.featuresUsed);
    rightFeaturesUsed.Clone(workItem.featuresUsed);

    DecisionTreeWorkItem leftWorkItem(leftNodeIndex, leftRemaining, leftFeaturesUsed);
    DecisionTreeWorkItem rightWorkItem(rightNodeIndex, rightRemaining, rightFeaturesUsed);

    AnkiAssert(leftWorkItem.remaining.buffer);
    AnkiAssert(leftWorkItem.featuresUsed.buffer);
    AnkiAssert(rightWorkItem.remaining.buffer);
    AnkiAssert(rightWorkItem.featuresUsed.buffer);

    buildTreeParams->workQueue.Lock(); // Lock workQueue
    buildTreeParams->workQueue.Push_unsafe(leftWorkItem);
    buildTreeParams->workQueue.Push_unsafe(rightWorkItem);
    buildTreeParams->workQueue.Unlock(); // Unlock workQueue

    workItem.featuresUsed.Free();
    workItem.remaining.Free();

#ifdef PRINT_INTERMEDIATE
    const f64 time1 = GetTimeF64();
    Anki::CoreTechPrint("Thread %d: Computed split %d and %d, with best entropy of %f, in %f seconds\n", buildTreeParams->threadId, static_cast<s32>(leftRemaining.size), static_cast<s32>(rightRemaining.size), curNode.bestEntropy, time1-time0);
#endif
  } // while(buildTreeParams->numCompleted.Get() < buildTreeParams->labels.get_size())

  return 0;
} // ThreadResult BuildTreeThread(void *voidParameters)

namespace Anki
{
  namespace Embedded
  {
    s32 FindMaxLabel(const FixedLengthList<s32> &labels, const RawBuffer<s32> &remaining)
    {
      s32 maxLabel = s32_MIN;

      const s32 numRemaining = remaining.size;
      const s32 * restrict pLabels = labels.Pointer(0);
      const s32 * restrict pRemaining = remaining.buffer;

      for(s32 i=0; i<numRemaining; i++) {
        maxLabel = MAX(maxLabel, pLabels[pRemaining[i]]);
      }

      return maxLabel;
    }

    ThreadResult BenchmarkingThread(void * voidBenchmarkingParams)
    {
      BenchmarkingParameters * restrict benchmarkingParams = reinterpret_cast<BenchmarkingParameters*>(voidBenchmarkingParams);

      AnkiAssert(benchmarkingParams->sampleEveryNSeconds >= 0.1);

      const f64 startTime = GetTimeF64();

      f64 lastTime = GetTimeF64();

      while(*(benchmarkingParams->isRunning)) {
        const f64 endTime = startTime + benchmarkingParams->sampleEveryNSeconds * static_cast<f64>(benchmarkingParams->cpuUsage.size());

        // Busy-wait to sleep
        while(*(benchmarkingParams->isRunning) && (GetTimeF64() < endTime)) {
          usleep(BUSY_WAIT_SLEEP_MICROSECONDS);
        }

        // This loop body will run every sampleEveryNSeconds seconds, and once right before quitting

        lastTime = GetTimeF64();

        const f32 curCpuUsage = GetCpuUsage();

        benchmarkingParams->cpuUsage.push_back(curCpuUsage);

        const s32 numComplete = benchmarkingParams->numCompleted.Get();

        CoreTechPrint("CPU %0.1f%% (queue is %d long). %d/%d = %0.2f%% completed in %0.2f seconds.\n",
          curCpuUsage,
          benchmarkingParams->workQueue.Size_unsafe(),
          numComplete,
          benchmarkingParams->numTotal,
          100.0 * static_cast<f64>(numComplete) / static_cast<f64>(benchmarkingParams->numTotal),
          lastTime - startTime);
      } // while(*isRunning)

      return 0;
    }

    Result BuildTree(
      const std::vector<U8Bool> &featuresUsed, //< numFeatures x 256
      const FixedLengthList<const char *> &labelNames, //< Lookup table between index and string name
      const FixedLengthList<s32> &labels, //< The label for every item to train on (very large)
      const FixedLengthList<const FixedLengthList<u8> > &featureValues, //< For every feature, the value for the feature for every image (outer is small, inner is very large)
      const f32 leafNodeFraction, //< What percent of the items in a node have to be the same for it to be considered a leaf node? From [0.0, 1.0], where 1.0 is a good value.
      const s32 leafNodeNumItems, //< If the number of items in a node is equal or below this, it is a leaf. 1 is a good value.
      const s32 u8MinDistanceForSplits, //< How close can two grayvalues be to be a threshold? 100 is a good value.
      const s32 u8MinDistanceFromThreshold, //< If a feature value is closer than this to the threshold, pass it to both subpaths
      const FixedLengthList<u8> &u8ThresholdsToUse, //< If not empty, this is the list of grayvalue thresholds to use
      const s32 maxThreads, //< Max threads to use (should be at least the number of cores)
      f64 benchmarkSampleEveryNSeconds, //< How often to sample CPU usage for benchmarking
      std::vector<f32> &cpuUsage, //< Sampled cpu percentages
      ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree //< The output decision tree
      )
    {
      const s32 numSingleThreadNodes = 1024;

      //
      // Check if inputs are valid
      //

      AnkiConditionalErrorAndReturnValue(AreValid(labelNames, labels, featureValues, u8ThresholdsToUse),
        RESULT_FAIL_INVALID_OBJECT, "BuildTree", "Invalid inputs");

      const s32 numFeatures = featuresUsed.size();
      const s32 numItems = labels.get_size();

      AnkiConditionalErrorAndReturnValue(
        leafNodeFraction > 0.01 && leafNodeFraction <= 1.0 &&
        leafNodeNumItems >= 1 &&
        u8MinDistanceForSplits >= 0 &&
        maxThreads <= MAX_THREADS,
        RESULT_FAIL_INVALID_PARAMETER, "BuildTree", "Invalid parameter");

      AnkiConditionalErrorAndReturnValue(
        featureValues.get_size() == numFeatures,
        RESULT_FAIL_INVALID_SIZE, "BuildTree", "Incorrect input size");

      for(s32 i=0; i<numFeatures; i++) {
        AnkiConditionalErrorAndReturnValue(
          featureValues[i].get_size() == numItems,
          RESULT_FAIL_INVALID_SIZE, "BuildTree", "Incorrect input size");
      }

      //
      // Allocate memory and initialize
      //

      RawBuffer<s32> remaining(numItems);

      s32 * restrict pRemaining = remaining.buffer;
      for(s32 i=0; i<numItems; i++) {
        pRemaining[i] = i;
      }

      const s32 maxLabel = FindMaxLabel(labels, remaining);

      std::vector<std::vector<s32*> > pNumLTs;
      std::vector<std::vector<s32*> > pNumGEs;

      pNumLTs.resize(maxThreads);
      pNumGEs.resize(maxThreads);

      for(s32 i=0; i<maxThreads; i++) {
        pNumLTs[i].resize(maxThreads);
        pNumGEs[i].resize(maxThreads);

        for(s32 j=0; j<maxThreads; j++) {
          pNumLTs[i][j] = reinterpret_cast<s32*>(malloc((maxLabel+1)*sizeof(s32)));
          pNumGEs[i][j] = reinterpret_cast<s32*>(malloc((maxLabel+1)*sizeof(s32)));

          AnkiConditionalErrorAndReturnValue(
            pNumLTs[i][j] && pNumGEs[i][j],
            RESULT_FAIL_OUT_OF_MEMORY, "BuildTree", "Out of memory");
        }
      }

      ThreadSafeQueue<DecisionTreeWorkItem> workQueue = ThreadSafeQueue<DecisionTreeWorkItem>();

      RawBuffer<U8Bool> featuresUsedRaw(featuresUsed.size());
      for(s32 i=0; i<static_cast<s32>(featuresUsed.size()); i++) {
        featuresUsedRaw.buffer[i] = featuresUsed[i];
      }

      workQueue.Lock();
      workQueue.Push_unsafe(DecisionTreeWorkItem(0, remaining, featuresUsedRaw));
      workQueue.Unlock();

      ThreadSafeVector<TrainingFailure> trainingFailures = ThreadSafeVector<TrainingFailure>();

      decisionTree.get_buffer().set_size(1);
      decisionTree.get_buffer()[0] = DecisionTreeNode(0, FLT_MAX, -1, -1, -1, -1, -1, 0, 255);

      ThreadSafeCounter<s32> numCompleted(0, s32_MAX);

      vector<BuildTreeThreadParameters*> newParameters;
      newParameters.resize(maxThreads);

      vector<ThreadHandle> threadHandles;
      threadHandles.resize(maxThreads);

      //
      // Start benchmarking
      //

      volatile bool isBenchmarkingRunning = true;

      BenchmarkingParameters benchmarkingParams(&isBenchmarkingRunning, benchmarkSampleEveryNSeconds, cpuUsage, numCompleted, labels.get_size(), workQueue);

      ThreadHandle benchmarkingThreadHandle = CreateSimpleThread(BenchmarkingThread, reinterpret_cast<void*>(&benchmarkingParams));

      CoreTechPrint("Starting single thread\n");

      //
      // First, launch one thread, to compute the first N nodes with all threads for computing entropy
      //

      {
        const s32 threadId = 0;

        BuildTreeThreadParameters oneThreadParameters(
          featuresUsed,
          labelNames,
          labels,
          featureValues,
          leafNodeFraction,
          leafNodeNumItems,
          u8MinDistanceForSplits,
          u8MinDistanceFromThreshold,
          u8ThresholdsToUse,
          maxThreads,
          pNumLTs[threadId],
          pNumGEs[threadId],
          maxLabel,
          threadId,
          numSingleThreadNodes,
          workQueue,
          trainingFailures,
          decisionTree,
          numCompleted);

        BuildTreeThread(reinterpret_cast<void*>(&oneThreadParameters));
      }

      CoreTechPrint("Single thread done, starting multi-threads.\n");

      //
      // Second, launch one primary thread for each maxThreads
      //

      for(s32 iThread=0; iThread<maxThreads; iThread++) {
        newParameters[iThread] = new BuildTreeThreadParameters(
          featuresUsed,
          labelNames,
          labels,
          featureValues,
          leafNodeFraction,
          leafNodeNumItems,
          u8MinDistanceForSplits,
          u8MinDistanceFromThreshold,
          u8ThresholdsToUse,
          1,
          pNumLTs[iThread],
          pNumGEs[iThread],
          maxLabel,
          iThread,
          -1,
          workQueue,
          trainingFailures,
          decisionTree,
          numCompleted);

        AnkiConditionalErrorAndReturnValue(
          newParameters[iThread],
          RESULT_FAIL_OUT_OF_MEMORY, "BuildTree", "Out of memory");

        threadHandles[iThread] = CreateSimpleThread(BuildTreeThread, reinterpret_cast<void*>(newParameters[iThread]));
      } // for(s32 iThread=0; iThread<maxThreads; iThread++)

      //
      // Wait for all threads to finish and clean up
      //

      for(s32 iThread=0; iThread<maxThreads; iThread++) {
        WaitForSimpleThread(threadHandles[iThread]);
      } // for(s32 i=0; i<maxThreads; i++)

      for(s32 i=0; i<maxThreads; i++) {
        delete(newParameters[i]);

        for(s32 j=0; j<maxThreads; j++) {
          free(pNumLTs[i][j]);
          free(pNumGEs[i][j]);
        }
      }

      isBenchmarkingRunning = false;

      WaitForSimpleThread(benchmarkingThreadHandle);

      return RESULT_OK;
    } // Result BuildTree()
  } // namespace Embedded
} // namespace Anki
