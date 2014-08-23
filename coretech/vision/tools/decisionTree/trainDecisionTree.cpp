/**
File: trainDecisionTree.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "trainDecisionTree.h"

#include <vector>
#include <queue>

using namespace std;

namespace Anki
{
  namespace Embedded
  {
    typedef struct DecisionTreeWorkItem
    {
      s32 treeIndex;
      const vector<s32> &remaining; // remaining: [1x552000 int32];
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
      const f32 leafNodeFraction,
      const s32 leafNodeNumItems,
      const s32 minGrayvalueDistance,
      const FixedLengthList<u8> &grayvalueThresholdsToUse,
      vector<DecisionTreeNode> &decisionTree)
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

      DecisionTreeNode rootNode(0, 0, 0, 0, 0, 0, -1);

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

      //
      // Keep popping items from the work queue, until the queue is empty.
      //

      // TODO: add ability to launch threads when the work items are far down in the tree

      while(workQueue.size() > 0) {
        const DecisionTreeWorkItem workItem = workQueue.front();
        workQueue.pop();

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

        const ThreadResult result = ComputeInfoGain(reinterpret_cast<void*>(&newParameters));

        // TODO: take the output, and use it to build more of the tree, and add more things to the work queue
      } // while(workQueue.size() > 0)

      free(pNumLessThan); pNumLessThan = NULL;
      free(pNumGreaterThan); pNumGreaterThan = NULL;

      return RESULT_OK;
    } // BuildTree()
  } // namespace Embedded
} // namespace Anki
