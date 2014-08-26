/**
File: trainDecisionTree.h
Author: Peter Barnum
Created: 2014-08-22

Train a decision tree, using multiple threads. Should work either locally or on an EC2 instance. Requires a heap.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _TRAIN_DECISION_TREE_H_
#define _TRAIN_DECISION_TREE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/tools/threads/threadSafeUtilities.h"

#include <vector>

namespace Anki
{
  namespace Embedded
  {
    typedef struct {
      bool values[256];
    } GrayvalueBool;

    typedef struct DecisionTreeNode
    {
      s32 depth;
      f32 infoGain;
      s32 whichProbe;
      u8 grayvalueThreshold;
      f32 x;
      f32 y;
      s32 leftChildIndex; //< Right child index is leftChildIndex + 1

      DecisionTreeNode(
        s32 depth,
        f32 infoGain,
        s32 whichProbe,
        u8 grayvalueThreshold,
        f32 x,
        f32 y,
        s32 leftChildIndex)  //< Right child index is leftChildIndex + 1. If leftChildIndex <= -1000000, this is a leaf, with the label as the negative of leftChildIndex.
        : depth(depth), infoGain(infoGain), whichProbe(whichProbe), grayvalueThreshold(grayvalueThreshold), x(x), y(y), leftChildIndex(leftChildIndex)
      {
      }
    } DecisionTreeNode;

    typedef struct ComputeInfoGainParameters
    {
      // Thread-independent input
      const FixedLengthList<const FixedLengthList<u8> > &probeValues; // For each probe location and for each image, what is the grayvalue?
      const FixedLengthList<s32> &labels; // For each image, what is it's ground truth label
      const FixedLengthList<u8> &grayvalueThresholdsToUse; //< If not empty, use these grayvalue thresholds instead of computing them
      const s32 minGrayvalueDistance; //< For a given probe location, how close can two grayvalue thresholds be?

      // Thread-specific input
      const std::vector<s32> &remaining; //< The indexes of the remaining images
      const std::vector<s32> probesLocationsToCheck; //< Which of the probe locations is this thread responsible for?

      // Thread-specific input/output
      std::vector<GrayvalueBool> probesUsed; //< Which probes locations and grayvalues have been used?

      // Thread-specific scratch
      s32 * restrict pNumLessThan; //< Must be allocated before calling the thread, and manually freed after the thread is complete
      s32 * restrict pNumGreaterThan; //< Must be allocated before calling the thread, and manually freed after the thread is complete

      // Thread-specific Output
      f32 bestEntropy; //< What is the best entropy that has been computed?
      s32 bestProbeIndex; //< What is the probe index corresponding to the best entropy?
      s32 bestGrayvalueThreshold; //< What is the grayvalue threshold corresponding to the best entropy?

      ComputeInfoGainParameters(
        const FixedLengthList<const FixedLengthList<u8> > &probeValues,
        const FixedLengthList<s32> &labels,
        const FixedLengthList<u8> &grayvalueThresholdsToUse,
        const s32 minGrayvalueDistance,
        const std::vector<s32> &remaining,
        std::vector<s32> probesLocationsToCheck,
        std::vector<GrayvalueBool> probesUsed,
        s32 * restrict pNumLessThan,
        s32 * restrict pNumGreaterThan,
        f32 bestEntropy,
        s32 bestProbeIndex,
        s32 bestGrayvalueThreshold)
        : probeValues(probeValues), labels(labels), grayvalueThresholdsToUse(grayvalueThresholdsToUse), minGrayvalueDistance(minGrayvalueDistance), remaining(remaining), probesLocationsToCheck(probesLocationsToCheck), probesUsed(probesUsed), pNumLessThan(pNumLessThan), pNumGreaterThan(pNumGreaterThan), bestEntropy(bestEntropy), bestProbeIndex(bestProbeIndex), bestGrayvalueThreshold(bestGrayvalueThreshold)
      {
      }
    } ComputeInfoGainParameters;

    ThreadResult ComputeInfoGain(void *computeInfoGainParameters);

    s32 FindMaxLabel(const FixedLengthList<s32> &labels, const std::vector<s32> &remaining);

    Result BuildTree(
      const std::vector<GrayvalueBool> &probesUsed, //< numProbes x 256
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
      );
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _TRAIN_DECISION_TREE_H_
