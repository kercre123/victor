/**
File: decisionTree_vision.h
Author: Peter Barnum
Created: 2014-03-03

The decision tree class queries an input binary image, based on a learned tree.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_DECISION_TREE_H_
#define _ANKICORETECHEMBEDDED_VISION_DECISION_TREE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/decisionTree.h"
#include "anki/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    class FiducialMarkerDecisionTree : public DecisionTree
    {
    public:
      typedef struct {
        s16 probeXCenter;
        s16 probeYCenter;
        u16 leftChildIndex;
        u16 label; //< If the high bit of the label is 1, then this node is a leaf
      } Node;

      FiducialMarkerDecisionTree();

      // treeDataLength is the number of bytes in the treeData buffer
      // Warning: The input treeData buffer is not copied, and must be globally available
      FiducialMarkerDecisionTree(const u8 * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth, const s16 * restrict probeXOffsets, const s16 * restrict probeYOffsets, const s32 numProbeOffsets);

      // Transform the image coordinates via transformation, then query the tree
      // If the sum of all probes if greater than grayvalueThreshold, then that point is considered white
      Result Classify(const Array<u8> &image, const Array<f32> &homography, u32 grayvalueThreshold, s32 &label) const;

    protected:
      const s16 * restrict probeXOffsets;
      const s16 * restrict probeYOffsets;
      const s32 numProbeOffsets;
    }; // class DecisionTree
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DECISION_TREE_H_
