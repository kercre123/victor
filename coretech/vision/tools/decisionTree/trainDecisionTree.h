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
#include "anki/tools/threads/threadSafeFixedLengthList.h"

#include <vector>

// Visual c++ is missing log2
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
      s32 leftChildIndex; //< Right child index is leftChildIndex + 1. If leftChildIndex <= -1000000, this is a leaf, with the label as the negative of leftChildIndex.
      s32 numLeft;
      s32 numRight;
      s32 numLeftAndRight; //< How many images are within u8MinDistanceFromThreshold from the threshold? numImages = (numLeft + numRight - numLeftAndRight)
      u8  u8Threshold;
      u8  meanDistanceFromThreshold;

      DecisionTreeNode(
        s32 depth,
        f32 bestEntropy,
        s32 whichFeature,
        s32 leftChildIndex,
        s32 numLeft,
        s32 numRight,
        s32 numLeftAndRight,
        u8  u8Threshold,
        u8  meanDistanceFromThreshold)
        : depth(depth), bestEntropy(bestEntropy), whichFeature(whichFeature), leftChildIndex(leftChildIndex), numLeft(numLeft), numRight(numRight), numLeftAndRight(numLeftAndRight), u8Threshold(u8Threshold), meanDistanceFromThreshold(meanDistanceFromThreshold)
      {
      }
    } DecisionTreeNode;

    // Class to hold a buffer. The buffer is allocated on construction, but must be manually freed.
    template<typename Type> class RawBuffer
    {
    public:
      Type * buffer;
      s32 size;

      RawBuffer();

      RawBuffer(s32 size);

      // Allocate memory and clone in
      // WARNING: Does not free memory before allocating
      void Clone(const RawBuffer &in);

      void Free();
    }; // RawBuffer

    typedef struct ComputeInfoGainParameters
    {
      // Thread-independent input
      const FixedLengthList<const FixedLengthList<u8> > &featureValues; // For each feature and for each image, what is the value?
      const FixedLengthList<s32> &labels; // What is the ground truth label for each image?
      const FixedLengthList<u8> &u8ThresholdsToUse; //< If not empty, use these u8 thresholds instead of computing them
      const s32 u8MinDistanceForSplits; //< How close can the value for splitting on two values of one feature? Set to 255 to disallow splitting more than once per feature (e.g. multiple splits for one probe in the same location).
      const s32 u8MinDistanceFromThreshold; //< If a feature value is closer than this to the threshold, pass it to both subpaths

      // Thread-specific input
      const RawBuffer<s32> &remaining; //< The indexes of the remaining images
      const s32 minFeatureToCheck; //< Which of the features is this thread responsible for?
      const s32 maxFeatureToCheck; //< Which of the features is this thread responsible for?

      // Thread-specific input/output
      RawBuffer<U8Bool> &featuresUsed; //< Which features and u8 thresholds have been used? Note: All threads share one copy, because they shouldn't read or modify the same pieces.

      // Thread-specific scratch
      s32 * restrict pNumLT; //< Must be allocated before calling the thread, and manually freed after the thread is complete
      s32 * restrict pNumGE; //< Must be allocated before calling the thread, and manually freed after the thread is complete

      // Thread-specific Output
      f32 bestEntropy; //< What is the best entropy that has been computed?
      s32 bestFeatureIndex; //< What is the feature index corresponding to the best entropy?
      s32 bestU8Threshold; //< What is the grayvalue threshold corresponding to the best entropy?
      s32 totalNumLT; //< How many images are below bestU8Threshold for feature bestFeatureIndex?
      s32 totalNumGE; //< How many images are equal or above bestU8Threshold for feature bestFeatureIndex?
      s32 totalNumBoth; //< How many images are within u8MinDistanceFromThreshold from the threshold? numImages = (totalNumLT + totalNumGE - totalNumBoth)
      u8 meanDistanceFromThreshold; //< It is better for features to be different from each other, so if there's a tie, the most different split will be chosen

      ComputeInfoGainParameters(
        const FixedLengthList<const FixedLengthList<u8> > &featureValues,
        const FixedLengthList<s32> &labels,
        const FixedLengthList<u8> &u8ThresholdsToUse,
        const s32 u8MinDistanceForSplits,
        const s32 u8MinDistanceFromThreshold,
        const RawBuffer<s32> &remaining,
        const s32 minFeatureToCheck,
        const s32 maxFeatureToCheck,
        RawBuffer<U8Bool> &featuresUsed,
        s32 * restrict pNumLT,
        s32 * restrict pNumGE)
        : featureValues(featureValues), labels(labels), u8ThresholdsToUse(u8ThresholdsToUse), u8MinDistanceForSplits(u8MinDistanceForSplits), u8MinDistanceFromThreshold(u8MinDistanceFromThreshold), remaining(remaining), minFeatureToCheck(minFeatureToCheck), maxFeatureToCheck(maxFeatureToCheck), featuresUsed(featuresUsed), pNumLT(pNumLT), pNumGE(pNumGE), bestEntropy(-1), bestFeatureIndex(-1), bestU8Threshold(0)
      {
      }
    } ComputeInfoGainParameters;

    ThreadResult ComputeInfoGain(void *computeInfoGainParameters);

    s32 FindMaxLabel(const FixedLengthList<s32> &labels, const RawBuffer<s32> &remaining);

    // Build a tree, based on input parameters. See ComputeInfoGainParameters for details on the parameters
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
      );
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _TRAIN_DECISION_TREE_H_
