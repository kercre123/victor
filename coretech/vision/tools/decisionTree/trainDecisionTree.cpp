/**
File: trainDecisionTree.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "trainDecisionTree.h"
#include "anki/tools/threads/threadSafeQueue.h"

#include <vector>
#include <queue>

using namespace std;

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

template<typename Type> void maxAndMin(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, Type &minValue, Type &maxValue);

//template<typename Type> vector<Type> findUnique(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const Type minValue, const Type maxValue);

vector<UniqueCount> findUnique(const s32 * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const s32 minValue, const s32 maxValue);

template<typename Type> void maxAndMin(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, Type &minValue, Type &maxValue)
{
  minValue = pArray[0];
  maxValue = pArray[0];

  for(s32 i=1; i<numRemaining; i++) {
    minValue = MIN(minValue, pArray[pRemaining[i]]);
    maxValue = MAX(maxValue, pArray[pRemaining[i]]);
  }
}

//template<typename Type> vector<Type> findUnique(const Type * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const Type minValue, const Type maxValue)
//{
//  const s32 countsLength = maxValue - minValue + 1;
//
//  s32 * counts = reinterpret_cast<s32*>( malloc(countsLength*sizeof(s32)) );
//
//  memset(counts, 0, countsLength*sizeof(s32));
//
//  for(s32 i=0; i<numRemaining; i++) {
//    const Type curValue = pArray[pRemaining[i]] - minValue;
//    counts[curValue]++;
//  }
//
//  s32 numUnique = 0;
//
//  for(s32 i=0; i<countsLength; i++) {
//    if(counts[i] > 0)
//      numUnique++;
//  }
//
//  vector<Type> uniques;
//  uniques.resize(numUnique);
//
//  s32 cUnique = 0;
//  for(s32 i=0; i<countsLength; i++) {
//    if(counts[i] > 0) {
//      uniques[cUnique] = static_cast<Type>(i + minValue);
//      cUnique++;
//    }
//  }
//
//  free(counts);
//
//  return uniques;
//}

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

namespace Anki
{
  namespace Embedded
  {
    typedef struct DecisionTreeWorkItem
    {
      s32 treeIndex;
      const vector<s32> remaining; // remaining: [1x552000 int32];
      vector<GrayvalueBool> probesUsed;

      DecisionTreeWorkItem(s32 treeIndex, const vector<s32> &remaining, const vector<GrayvalueBool> &probesUsed)
        : treeIndex(treeIndex), remaining(remaining), probesUsed(probesUsed)
      {
      }
    } DecisionTreeWorkItem;

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

    Result BuildTree(
      const vector<GrayvalueBool> &probesUsed, //< numProbes x 256
      const FixedLengthList<const char *> &labelNames, //< Lookup table between index and string name
      const FixedLengthList<s32> &labels, //< The label for every item to train on (very large)
      const FixedLengthList<const FixedLengthList<u8> > &probeValues, //< For every probe, the value for the probe every item to train on (outer is small, inner is very large)
      const FixedLengthList<f32> &probeLocationsXGrid, //< Lookup table between probe index and probe x location
      const FixedLengthList<f32> &probeLocationsYGrid, //< Lookup table between probe index and probe y location
      const f32 leafNodeFraction, //< What percent of the items in a node have to be the same for it to be considered a leaf node? From [0.0, 1.0], where 1.0 is a good value.
      const s32 leafNodeNumItems, //< If the number of items in a node is equal or below this, it is a leaf. 1 is a good value.
      const s32 minGrayvalueDistance, //< How close can two grayvalues be to be a threshold? 100 is a good value.
      const FixedLengthList<u8> &grayvalueThresholdsToUse, //< If not empty, this is the list of grayvalue thresholds to use
      std::vector<DecisionTreeNode> &decisionTree //< The output decision tree
      )
    {
      //
      // Check if inputs are valid
      //

      AnkiConditionalErrorAndReturnValue(AreValid(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, grayvalueThresholdsToUse),
        RESULT_FAIL_INVALID_OBJECT, "BuildTree", "Invalid inputs");

      const s32 numProbes = probesUsed.size();
      const s32 numItems = labels.get_size();

      AnkiConditionalErrorAndReturnValue(
        probeValues.get_size() == numProbes &&
        probeLocationsXGrid.get_size() == numProbes &&
        probeLocationsYGrid.get_size() == numProbes,
        RESULT_FAIL_INVALID_SIZE, "BuildTree", "Incorrect input size");

      for(s32 i=0; i<numProbes; i++) {
        AnkiConditionalErrorAndReturnValue(
          probeValues[i].get_size() == numItems,
          RESULT_FAIL_INVALID_SIZE, "BuildTree", "Incorrect input size");
      }

      //
      // Setup the decision tree and work queue with the root node
      //

      DecisionTreeNode rootNode(0, FLT_MAX, -1, 0, 0, 0, -1);

      decisionTree.emplace_back(rootNode);

      vector<s32> remaining;
      remaining.resize(numItems);

      s32 * restrict pRemaining = remaining.data();
      for(s32 i=0; i<numItems; i++) {
        pRemaining[i] = i;
      }

      DecisionTreeWorkItem firstWorkItem(0, remaining, probesUsed);

      queue<DecisionTreeWorkItem> workQueue;
      workQueue.emplace(firstWorkItem);

      const s32 maxLabel = FindMaxLabel(labels, remaining);

      s32 * restrict pNumLessThan = reinterpret_cast<s32*>( malloc((maxLabel+1)*sizeof(s32)) );
      s32 * restrict pNumGreaterThan = reinterpret_cast<s32*>( malloc((maxLabel+1)*sizeof(s32)) );

      std::vector<s32> probesLocationsToCheck;
      probesLocationsToCheck.resize(numProbes);
      s32 * restrict pProbesLocationsToCheck = probesLocationsToCheck.data();
      for(s32 i=0; i<numProbes; i++) {
        pProbesLocationsToCheck[i] = i;
      }

      ThreadSafeQueue<TrainingFailure> trainingFailues = ThreadSafeQueue<TrainingFailure>();

      //
      // Keep popping items from the work queue, until the queue is empty.
      //

      const f32 * restrict pProbeLocationsXGrid = probeLocationsXGrid.Pointer(0);
      const f32 * restrict pProbeLocationsYGrid = probeLocationsYGrid.Pointer(0);

      // TODO: add ability to launch threads when the work items are far down in the tree

      while(workQueue.size() > 0) {
        const DecisionTreeWorkItem workItem = workQueue.front();
        workQueue.pop();

        // vector<UniqueCount> findUnique(const s32 * restrict pArray, const s32 * restrict pRemaining, const s32 numRemaining, const s32 minValue, const s32 maxValue)

        AnkiAssert(workItem.remaining.size() > 0);

        //
        // First, check if this node is a leaf
        //

        s32 minLabel;
        s32 maxLabel;
        maxAndMin<s32>(labels.Pointer(0), workItem.remaining.data(), workItem.remaining.size(), minLabel, maxLabel);

        vector<UniqueCount> uniqueLabelCounts = findUnique(labels.Pointer(0), workItem.remaining.data(), workItem.remaining.size(), minLabel, maxLabel);

        s32 maxUniqueCount = 0;
        s32 maxUniqueCountId = -1;

        {
          const s32 numuniqueLabelCounts = uniqueLabelCounts.size();
          const UniqueCount * restrict puniqueLabelCounts = uniqueLabelCounts.data();

          for(s32 i=0; i<numuniqueLabelCounts; i++) {
            if(puniqueLabelCounts[i].count > maxUniqueCount) {
              maxUniqueCount = puniqueLabelCounts[i].count;
              maxUniqueCountId = puniqueLabelCounts[i].value;
            }
          }
        }

        bool unusedProbeLocationFound = false;

        {
          const s32 numGrayvalueThresholdsToUse = grayvalueThresholdsToUse.get_size();

          const GrayvalueBool * restrict pProbesUsed = workItem.probesUsed.data();
          for(s32 iProbe=0; iProbe<numProbes; iProbe++) {
            if(unusedProbeLocationFound) {
              break;
            }

            if(numGrayvalueThresholdsToUse > 0) {
              const u8 * restrict pGrayvalueThresholdsToUse = grayvalueThresholdsToUse.Pointer(0);

              for(s32 iGray=0; iGray<numGrayvalueThresholdsToUse; iGray++) {
                if(pProbesUsed[iProbe].values[pGrayvalueThresholdsToUse[iGray]] == 0) {
                  unusedProbeLocationFound = true;
                  break;
                }
              }
            } else {
              for(s32 iGray=0; iGray<256; iGray++) {
                if(pProbesUsed[iProbe].values[iGray] == 0) {
                  unusedProbeLocationFound = true;
                  break;
                }
              }
            }
          }
        }

        if((maxUniqueCount >= saturate_cast<s32>(leafNodeFraction * workItem.remaining.size())) || (static_cast<s32>(workItem.remaining.size()) < leafNodeNumItems)) {
          // Are more than leafNodeFraction percent of the remaining labels the same? If so, we're done.
          decisionTree[workItem.treeIndex].leftChildIndex = -maxUniqueCountId - 1000000;

          // Comment or uncomment the next line as desired
          printf("LeafNode for label = %d, or \"%s\" at depth %d\n", maxUniqueCountId, labelNames[maxUniqueCountId], decisionTree[workItem.treeIndex].depth);

          continue;
        } else if(!unusedProbeLocationFound) {
          // Have we used all probes? If so, we're done.

          decisionTree[workItem.treeIndex].leftChildIndex = -maxUniqueCountId - 1000000;

          // Comment or uncomment the next line as desired
          printf("MaxDepth LeafNode for label = %d, or \"%s\" at depth %d\n", maxUniqueCountId, labelNames[maxUniqueCountId], decisionTree[workItem.treeIndex].depth);

          continue;
        }

        //
        // If this isn't a leaf, find the best split
        //

        ComputeInfoGainParameters newParameters(
          probeValues, labels, grayvalueThresholdsToUse,
          minGrayvalueDistance,
          workItem.remaining,
          probesLocationsToCheck,
          workItem.probesUsed,
          pNumLessThan,
          pNumGreaterThan,
          0,
          0,
          0);

        //const f64 t0 = GetTimeF64();

        const ThreadResult result = ComputeInfoGain(reinterpret_cast<void*>(&newParameters));

        //const f64 t1 = GetTimeF64();

        //printf("Time %f\n", t1-t0);

        decisionTree[workItem.treeIndex].x = pProbeLocationsXGrid[newParameters.bestProbeIndex];
        decisionTree[workItem.treeIndex].y = pProbeLocationsYGrid[newParameters.bestProbeIndex];
        decisionTree[workItem.treeIndex].infoGain = newParameters.bestEntropy;
        decisionTree[workItem.treeIndex].whichProbe = newParameters.bestProbeIndex;
        decisionTree[workItem.treeIndex].grayvalueThreshold = newParameters.bestGrayvalueThreshold;

        // If bestEntropy is too large, it means we could not split the data
        if(newParameters.bestEntropy > 100000.0) {
          // TODO: there are probably very few labels, so this is an inefficent way to compute which are unique

          decisionTree[workItem.treeIndex].leftChildIndex = -1;

          TrainingFailure failure;

          failure.nodeIndex = workItem.treeIndex;
          failure.labels = uniqueLabelCounts;

          trainingFailues.Push(failure);

          printf("Could not split LeafNode for labels = {");

          const s32 numLabels = failure.labels.size();
          for(s32 i=0; i<numLabels; i++) {
            printf("(%d, %s), ", failure.labels[i], labelNames[failure.labels[i].value]);
          }

          printf("} at depth %d\n", decisionTree[workItem.treeIndex].depth);

          // TODO: show images?

          continue;
        } // if(newParameters.bestEntropy > 100000.0)

        decisionTree[workItem.treeIndex].leftChildIndex = decisionTree.size();

        DecisionTreeNode leftNode = DecisionTreeNode(decisionTree[workItem.treeIndex].depth + 1, FLT_MAX, -1, 0, 0, 0, -1);
        DecisionTreeNode rightNode = DecisionTreeNode(decisionTree[workItem.treeIndex].depth + 1, FLT_MAX, -1, 0, 0, 0, -1);

        decisionTree.emplace_back(leftNode);
        decisionTree.emplace_back(rightNode);

        const s32 numRemaining = workItem.remaining.size();

        //
        // Split the current remaining into left and right subsets
        //

        vector<s32> leftRemaining;
        vector<s32> rightRemaining;

        leftRemaining.resize(numRemaining);
        rightRemaining.resize(numRemaining);

        const u8 * restrict pProbeValues = probeValues[decisionTree[workItem.treeIndex].whichProbe].Pointer(0);

        const s32 * restrict pRemaining = workItem.remaining.data();
        s32 * restrict pLeftRemaining = leftRemaining.data();
        s32 * restrict pRightRemaining = rightRemaining.data();

        s32 cLeft = 0, cRight = 0;
        for(s32 iRemain=0; iRemain<numRemaining; iRemain++) {
          const s32 curRemaining = pRemaining[iRemain];
          const u8 curProbeValue = pProbeValues[curRemaining];

          if(curProbeValue < decisionTree[workItem.treeIndex].grayvalueThreshold) {
            pLeftRemaining[cLeft] = curRemaining;
            cLeft++;
          } else {
            pRightRemaining[cRight] = curRemaining;
            cRight++;
          }
        }

        AnkiAssert(cLeft > 0 && cRight > 0);

        leftRemaining.resize(cLeft);
        rightRemaining.resize(cRight);

        vector<GrayvalueBool> probesUsed;

        workQueue.push(DecisionTreeWorkItem(decisionTree.size() - 2, leftRemaining, newParameters.probesUsed));
        workQueue.push(DecisionTreeWorkItem(decisionTree.size() - 1, rightRemaining, newParameters.probesUsed));
      } // while(workQueue.size() > 0)

      free(pNumLessThan); pNumLessThan = NULL;
      free(pNumGreaterThan); pNumGreaterThan = NULL;

      return RESULT_OK;
    } // BuildTree()
  } // namespace Embedded
} // namespace Anki
