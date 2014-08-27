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
    } U8Bool;

    typedef struct DecisionTreeNode
    {
      s32 depth;
      f32 bestEntropy;
      s32 whichFeature;
      u8  u8Threshold;
      s32 leftChildIndex; //< Right child index is leftChildIndex + 1

      DecisionTreeNode(
        s32 depth,
        f32 bestEntropy,
        s32 whichFeature,
        u8  u8Threshold,
        s32 leftChildIndex)  //< Right child index is leftChildIndex + 1. If leftChildIndex <= -1000000, this is a leaf, with the label as the negative of leftChildIndex.
        : depth(depth), bestEntropy(bestEntropy), whichFeature(whichFeature), u8Threshold(u8Threshold), leftChildIndex(leftChildIndex)
      {
      }
    } DecisionTreeNode;

    typedef struct ComputeInfoGainParameters
    {
      // Thread-independent input
      const FixedLengthList<const FixedLengthList<u8> > &featureValues; // For each feature and for each image, what is the value?
      const FixedLengthList<s32> &labels; // What is the ground truth label for each image?
      const FixedLengthList<u8> &u8ThresholdsToUse; //< If not empty, use these u8 thresholds instead of computing them
      const s32 u8MinDistance; //< How close can the value for splitting on two values of one feature? Set to 255 to disallow splitting more than once per feature (e.g. multiple splits for one probe in the same location).

      // Thread-specific input
      const std::vector<s32> &remaining; //< The indexes of the remaining images
      const s32 minFeatureToCheck; //< Which of the features is this thread responsible for?
      const s32 maxFeatureToCheck; //< Which of the features is this thread responsible for?

      // Thread-specific input/output
      std::vector<U8Bool> &featuresUsed; //< Which features and u8 thresholds have been used? Note: All threads share one copy, because they shouldn't read or modify the same pieces.

      // Thread-specific scratch
      s32 * restrict pNumLessThan; //< Must be allocated before calling the thread, and manually freed after the thread is complete
      s32 * restrict pNumGreaterThan; //< Must be allocated before calling the thread, and manually freed after the thread is complete

      // Thread-specific Output
      f32 bestEntropy; //< What is the best entropy that has been computed?
      s32 bestFeatureIndex; //< What is the feature index corresponding to the best entropy?
      s32 bestU8Threshold; //< What is the grayvalue threshold corresponding to the best entropy?

      ComputeInfoGainParameters(
        const FixedLengthList<const FixedLengthList<u8> > &featureValues,
        const FixedLengthList<s32> &labels,
        const FixedLengthList<u8> &u8ThresholdsToUse,
        const s32 u8MinDistance,
        const std::vector<s32> &remaining,
        const s32 minFeatureToCheck,
        const s32 maxFeatureToCheck,
        std::vector<U8Bool> &featuresUsed,
        s32 * restrict pNumLessThan,
        s32 * restrict pNumGreaterThan)
        : featureValues(featureValues), labels(labels), u8ThresholdsToUse(u8ThresholdsToUse), u8MinDistance(u8MinDistance), remaining(remaining), minFeatureToCheck(minFeatureToCheck), maxFeatureToCheck(maxFeatureToCheck), featuresUsed(featuresUsed), pNumLessThan(pNumLessThan), pNumGreaterThan(pNumGreaterThan), bestEntropy(-1), bestFeatureIndex(-1), bestU8Threshold(0)
      {
      }
    } ComputeInfoGainParameters;

    ThreadResult ComputeInfoGain(void *computeInfoGainParameters);

    s32 FindMaxLabel(const FixedLengthList<s32> &labels, const std::vector<s32> &remaining);

    Result BuildTree(
      const std::vector<U8Bool> &featuresUsed, //< numFeatures x 256
      const FixedLengthList<const char *> &labelNames, //< Lookup table between index and string name
      const FixedLengthList<s32> &labels, //< The label for every item to train on (very large)
      const FixedLengthList<const FixedLengthList<u8> > &featureValues, //< For every feature, the value for the feature for every image (outer is small, inner is very large)
      const f32 leafNodeFraction, //< What percent of the items in a node have to be the same for it to be considered a leaf node? From [0.0, 1.0], where 1.0 is a good value.
      const s32 leafNodeNumItems, //< If the number of items in a node is equal or below this, it is a leaf. 1 is a good value.
      const s32 u8MinDistance, //< How close can two grayvalues be to be a threshold? 100 is a good value.
      const FixedLengthList<u8> &u8ThresholdsToUse, //< If not empty, this is the list of grayvalue thresholds to use
      const s32 maxPrimaryThreads, //< If we are building the end of the tree, use maxPrimaryThreads threads (one for each node), and on thread to compute the information fain
      const s32 maxSecondaryThreads, //< If we are building the start of the tree, use one primary thread, and maxSecondaryThreads threads to compute the information fain
      std::vector<DecisionTreeNode> &decisionTree //< The output decision tree
      );
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _TRAIN_DECISION_TREE_H_
