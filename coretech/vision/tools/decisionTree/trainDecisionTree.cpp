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

typedef struct
{
  s32 value;
  s32 count;
} UniqueCount;

typedef struct TrainingFailure
{
  s32 nodeIndex;
  vector<UniqueCount> labels;

  TrainingFailure()
    : nodeIndex(-1)
  {
  }
} TrainingFailure;

typedef struct DecisionTreeWorkItem
{
  s32 treeIndex;
  vector<s32> remaining; // remaining: [1x552000 int32];
  vector<U8Bool> featuresUsed;

  DecisionTreeWorkItem(s32 treeIndex, vector<s32> &remaining, const vector<U8Bool> &featuresUsed)
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
  const s32 u8MinDistance; //< How close can two grayvalues be to be a threshold? 100 is a good value.
  const FixedLengthList<u8> &u8ThresholdsToUse; //< If not empty, this is the list of grayvalue thresholds to use
  const s32 maxSecondaryThreads; //< If we are building the start of the tree, use one primary thread, and maxSecondaryThreads threads to compute the information fain
  const std::vector<s32*> &pNumLessThans; //< One allocated buffer for each maxSecondaryThreads. Must be allocated before calling the thread, and manually freed after the thread is complete
  const std::vector<s32*> &pNumGreaterThans; //< One allocated buffer for each maxSecondaryThreads. Must be allocated before calling the thread, and manually freed after the thread is complete
  const s32 maxLabel; //< What is the max label in labels?
  ThreadSafeQueue<DecisionTreeWorkItem> &workQueue; //< What work needs to be done? Starts with just the root node.
  ThreadSafeVector<TrainingFailure> &trainingFailures; //< Which nodes could not be trained?
  ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree; //< The output decision tree
  ThreadSafeCounter<s32> &threadCount; //< Counter for the number of running threads

  BuildTreeThreadParameters(
    const vector<U8Bool> &featuresUsed,
    const FixedLengthList<const char *> &labelNames,
    const FixedLengthList<s32> &labels,
    const FixedLengthList<const FixedLengthList<u8> > &featureValues,
    const f32 leafNodeFraction,
    const s32 leafNodeNumItems,
    const s32 u8MinDistance,
    const FixedLengthList<u8> &u8ThresholdsToUse,
    const s32 maxSecondaryThreads,
    const std::vector<s32*> &pNumLessThans,
    const std::vector<s32*> &pNumGreaterThans,
    const s32 maxLabel,
    ThreadSafeQueue<DecisionTreeWorkItem> &workQueue,
    ThreadSafeVector<TrainingFailure> &trainingFailures,
    ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree,
    ThreadSafeCounter<s32> &threadCount)
    : featuresUsed(featuresUsed), labelNames(labelNames), labels(labels), featureValues(featureValues), leafNodeFraction(leafNodeFraction), leafNodeNumItems(leafNodeNumItems), u8MinDistance(u8MinDistance), u8ThresholdsToUse(u8ThresholdsToUse), maxSecondaryThreads(maxSecondaryThreads), pNumLessThans(pNumLessThans), pNumGreaterThans(pNumGreaterThans), maxLabel(maxLabel), workQueue(workQueue), trainingFailures(trainingFailures), decisionTree(decisionTree), threadCount(threadCount)
  {
  }
} BuildTreeThreadParameters;

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
  // Keep popping items from the work queue, until the queue is empty.
  //

  // TODO: add ability to launch threads when the work items are far down in the tree

  while(buildTreeParams->workQueue.Size() > 0) {
    const f64 time0 = GetTimeF64();

    DecisionTreeWorkItem workItem = buildTreeParams->workQueue.Front();
    buildTreeParams->workQueue.Pop();

    AnkiAssert(workItem.remaining.size() > 0);

    // WARNING: no other thread should write to this node, but this allows non-thread-safe access
    DecisionTreeNode &curNode = buildTreeParams->decisionTree.get_buffer()[workItem.treeIndex];

    //
    // First, check if this node is a leaf
    //

    s32 minLabel;
    s32 maxLabel;
    maxAndMin<s32>(buildTreeParams->labels.Pointer(0), workItem.remaining.data(), workItem.remaining.size(), minLabel, maxLabel);

    vector<UniqueCount> uniqueLabelCounts = findUnique(buildTreeParams->labels.Pointer(0), workItem.remaining.data(), workItem.remaining.size(), minLabel, maxLabel);

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

      const U8Bool * restrict pFeaturesUsed = workItem.featuresUsed.data();
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

    if((mostCommonLabel_count >= saturate_cast<s32>(buildTreeParams->leafNodeFraction * workItem.remaining.size())) || (static_cast<s32>(workItem.remaining.size()) < buildTreeParams->leafNodeNumItems)) {
      // Are more than leafNodeFraction percent of the remaining labels the same? If so, we're done.

      curNode.leftChildIndex = -mostCommonLabel - 1000000;

      // Comment or uncomment the next line as desired
      printf("LeafNode for label = %d, or \"%s\" at depth %d\n", mostCommonLabel, buildTreeParams->labelNames[mostCommonLabel], curNode.depth);

      continue;
    } else if(!unusedFeaturesFound) {
      // Have we used all features? If so, we're done.

      curNode.leftChildIndex = -mostCommonLabel - 1000000;

      // Comment or uncomment the next line as desired
      printf("MaxDepth LeafNode for label = %d, or \"%s\" at depth %d\n", mostCommonLabel, buildTreeParams->labelNames[mostCommonLabel], curNode.depth);

      continue;
    }

    //
    // If this isn't a leaf, find the best split
    //
    s32 numSecondaryThreadsToUse = buildTreeParams->maxSecondaryThreads;

    // TODO: add back?
    //if(workItem.remaining.size() < 5) {
    //  numSecondaryThreadsToUse = 1;
    //}

    if(numSecondaryThreadsToUse == 1) {
      const s32 minFeatureToCheck = 0;
      const s32 maxFeatureToCheck = numFeatures - 1;

      ComputeInfoGainParameters newParameters(
        buildTreeParams->featureValues, buildTreeParams->labels, buildTreeParams->u8ThresholdsToUse, buildTreeParams->u8MinDistance,
        workItem.remaining,
        minFeatureToCheck,
        maxFeatureToCheck,
        workItem.featuresUsed,
        buildTreeParams->pNumLessThans[0],
        buildTreeParams->pNumGreaterThans[0]);

      const ThreadResult result = ComputeInfoGain(reinterpret_cast<void*>(&newParameters));

      curNode.bestEntropy = newParameters.bestEntropy;
      curNode.whichFeature = newParameters.bestFeatureIndex;
      curNode.u8Threshold = newParameters.bestU8Threshold;
    } else {
      const s32 numFeaturesPerThread = (numFeatures + numSecondaryThreadsToUse - 1) / numSecondaryThreadsToUse;

      for(s32 iThread=0; iThread<numSecondaryThreadsToUse; iThread++) {
        computeInfoGainParams[iThread] = new ComputeInfoGainParameters(
          buildTreeParams->featureValues, buildTreeParams->labels, buildTreeParams->u8ThresholdsToUse, buildTreeParams->u8MinDistance,
          workItem.remaining,
          iThread * numFeaturesPerThread,
          MIN(numFeatures - 1, (iThread+1) * numFeaturesPerThread - 1),
          workItem.featuresUsed,
          buildTreeParams->pNumLessThans[iThread],
          buildTreeParams->pNumGreaterThans[iThread]);

        threadHandles[iThread] = CreateSimpleThread(ComputeInfoGain, computeInfoGainParams[iThread]);
      }

      for(s32 iThread=0; iThread<numSecondaryThreadsToUse; iThread++) {
        // Wait for the threads to complete and combine the results

        WaitForSimpleThread(threadHandles[iThread]);

        if(computeInfoGainParams[iThread]->bestEntropy < curNode.bestEntropy) {
          curNode.bestEntropy = computeInfoGainParams[iThread]->bestEntropy;
          curNode.whichFeature = computeInfoGainParams[iThread]->bestFeatureIndex;
          curNode.u8Threshold = computeInfoGainParams[iThread]->bestU8Threshold;
        }

        delete(computeInfoGainParams[iThread]);
      }
    }

    // If bestEntropy is too large, it means we could not split the data
    if(curNode.bestEntropy > 100000.0 || curNode.whichFeature < 0) {
      // TODO: there are probably very few labels, so this is an inefficent way to compute which are unique

      curNode.leftChildIndex = -1;

      TrainingFailure failure;

      failure.nodeIndex = workItem.treeIndex;
      failure.labels = uniqueLabelCounts;

      buildTreeParams->trainingFailures.Push_back(failure);

      printf("Could not split LeafNode for labels = {");

      const s32 numLabels = failure.labels.size();
      for(s32 i=0; i<numLabels; i++) {
        printf("(%d, %s), ", failure.labels[i].value, buildTreeParams->labelNames[failure.labels[i].value]);
      }

      printf("} at depth %d\n", curNode.depth);

      // TODO: show images?

      continue;
    } // if(newParameters.bestEntropy > 100000.0)

    // Lock
    buildTreeParams->decisionTree.Lock();

    buildTreeParams->decisionTree.get_buffer().set_size(buildTreeParams->decisionTree.get_buffer().get_size() + 2);

    const s32 leftNodeIndex = buildTreeParams->decisionTree.get_buffer().get_size() - 2;
    const s32 rightNodeIndex = buildTreeParams->decisionTree.get_buffer().get_size() - 1;

    buildTreeParams->decisionTree.Unlock();
    // Unlock

    buildTreeParams->decisionTree.get_buffer()[leftNodeIndex] = DecisionTreeNode(curNode.depth + 1, FLT_MAX, -1, 0, -1);;
    buildTreeParams->decisionTree.get_buffer()[rightNodeIndex] = DecisionTreeNode(curNode.depth + 1, FLT_MAX, -1, 0, -1);

    curNode.leftChildIndex = leftNodeIndex;

    const s32 numRemaining = workItem.remaining.size();

    //
    // Split the current remaining into left and right subsets
    //

    vector<s32> leftRemaining;
    vector<s32> rightRemaining;

    leftRemaining.resize(numRemaining);
    rightRemaining.resize(numRemaining);

    const u8 * restrict pFeatureValues = buildTreeParams->featureValues[curNode.whichFeature].Pointer(0);

    const s32 * restrict pRemaining = workItem.remaining.data();
    s32 * restrict pLeftRemaining = leftRemaining.data();
    s32 * restrict pRightRemaining = rightRemaining.data();

    s32 cLeft = 0, cRight = 0;
    for(s32 iRemain=0; iRemain<numRemaining; iRemain++) {
      const s32 curRemaining = pRemaining[iRemain];
      const u8 curFeatureValue = pFeatureValues[curRemaining];

      if(curFeatureValue < curNode.u8Threshold) {
        pLeftRemaining[cLeft] = curRemaining;
        cLeft++;
      } else {
        pRightRemaining[cRight] = curRemaining;
        cRight++;
      }
    }

    AnkiAssert(cLeft > 0 && cLeft <= numRemaining && cRight > 0 && cRight <= numRemaining );

    leftRemaining.resize(cLeft);
    rightRemaining.resize(cRight);

    // Mask out the grayvalues that are near to the chosen threshold
    const s32 minGray = MAX(0,   curNode.u8Threshold - buildTreeParams->u8MinDistance);
    const s32 maxGray = MIN(255, curNode.u8Threshold + buildTreeParams->u8MinDistance);
    for(s32 iGray=minGray; iGray<=maxGray; iGray++) {
      U8Bool &pFeaturesUsed = workItem.featuresUsed[curNode.whichFeature];
      pFeaturesUsed.values[iGray] = true;
    }

    buildTreeParams->workQueue.Push(DecisionTreeWorkItem(leftNodeIndex, leftRemaining, workItem.featuresUsed));
    buildTreeParams->workQueue.Push(DecisionTreeWorkItem(rightNodeIndex, rightRemaining, workItem.featuresUsed));

    const f64 time1 = GetTimeF64();

    printf("Computed split %d and %d, with best entropy of %f, in %f seconds\n", static_cast<s32>(leftRemaining.size()), static_cast<s32>(rightRemaining.size()), curNode.bestEntropy, time1-time0);
  } // while(workQueue.size() > 0)

  return 0;
} // ThreadResult BuildTreeThread(void *voidParameters)

namespace Anki
{
  namespace Embedded
  {
    s32 FindMaxLabel(const FixedLengthList<s32> &labels, const vector<s32> &remaining)
    {
      s32 maxLabel = s32_MIN;

      const s32 numRemaining = remaining.size();
      const s32 * restrict pLabels = labels.Pointer(0);
      const s32 * restrict pRemaining = remaining.data();

      for(s32 i=0; i<numRemaining; i++) {
        maxLabel = MAX(maxLabel, pLabels[pRemaining[i]]);
      }

      return maxLabel;
    }

    ThreadResult BenchmarkingThread(void * voidBenchmarkingParams)
    {
      BenchmarkingParameters * restrict benchmarkingParams = reinterpret_cast<BenchmarkingParameters*>(voidBenchmarkingParams);

      AnkiAssert(benchmarkingParams->sampleEveryNSeconds >= 0.1);

      // First call returns zero
      GetCpuUsage();

      f64 lastTime = GetTimeF64();

      while(*(benchmarkingParams->isRunning)) {
        // Busy-wait to sleep
        while(*(benchmarkingParams->isRunning) && (GetTimeF64() - lastTime) < benchmarkingParams->sampleEveryNSeconds) {
          usleep(10);
        }

        lastTime = GetTimeF64();

        benchmarkingParams->cpuUsage.push_back(GetCpuUsage());
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
      const s32 u8MinDistance, //< How close can two grayvalues be to be a threshold? 100 is a good value.
      const FixedLengthList<u8> &u8ThresholdsToUse, //< If not empty, this is the list of grayvalue thresholds to use
      const s32 maxThreads, //< Max threads to use (should be at least the number of cores)
      ThreadSafeFixedLengthList<DecisionTreeNode> &decisionTree //< The output decision tree
      )
    {
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
        u8MinDistance >= 0 &&
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

      vector<s32> remaining;
      remaining.resize(numItems);

      s32 * restrict pRemaining = remaining.data();
      for(s32 i=0; i<numItems; i++) {
        pRemaining[i] = i;
      }

      const s32 maxLabel = FindMaxLabel(labels, remaining);

      std::vector<std::vector<s32*> > pNumLessThans;
      std::vector<std::vector<s32*> > pNumGreaterThans;

      pNumLessThans.resize(maxThreads);
      pNumGreaterThans.resize(maxThreads);

      for(s32 i=0; i<maxThreads; i++) {
        pNumLessThans[i].resize(maxThreads);
        pNumGreaterThans[i].resize(maxThreads);

        for(s32 j=0; j<maxThreads; j++) {
          pNumLessThans[i][j] = reinterpret_cast<s32*>(malloc((maxLabel+1)*sizeof(s32)));
          pNumGreaterThans[i][j] = reinterpret_cast<s32*>(malloc((maxLabel+1)*sizeof(s32)));

          AnkiConditionalErrorAndReturnValue(
            pNumLessThans[i][j] && pNumGreaterThans[i][j],
            RESULT_FAIL_OUT_OF_MEMORY, "BuildTree", "Out of memory");
        }
      }

      ThreadSafeQueue<DecisionTreeWorkItem> workQueue = ThreadSafeQueue<DecisionTreeWorkItem>();
      workQueue.Emplace(DecisionTreeWorkItem(0, remaining, featuresUsed));

      ThreadSafeVector<TrainingFailure> trainingFailures = ThreadSafeVector<TrainingFailure>();

      decisionTree.get_buffer().set_size(1);
      decisionTree.get_buffer()[0] = DecisionTreeNode(0, FLT_MAX, -1, 0, -1);

      ThreadSafeCounter<s32> threadCount(0, maxThreads);

      vector<BuildTreeThreadParameters*> newParameters;
      newParameters.resize(maxThreads);

      vector<ThreadHandle> threadHandles;
      threadHandles.resize(maxThreads);

      //
      // Launch one primary thread for each maxThreads
      //

      for(s32 iThread=0; iThread<maxThreads; iThread++) {
        newParameters[iThread] = new BuildTreeThreadParameters(
          featuresUsed,
          labelNames,
          labels,
          featureValues,
          leafNodeFraction,
          leafNodeNumItems,
          u8MinDistance,
          u8ThresholdsToUse,
          maxThreads,
          pNumLessThans[iThread],
          pNumGreaterThans[iThread],
          maxLabel,
          workQueue,
          trainingFailures,
          decisionTree,
          threadCount);

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
          free(pNumLessThans[i][j]);
          free(pNumGreaterThans[i][j]);
        }
      }

      return RESULT_OK;
    } // Result BuildTree()
  } // namespace Embedded
} // namespace Anki
