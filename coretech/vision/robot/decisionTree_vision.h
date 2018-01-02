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

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/decisionTree.h"
#include "coretech/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    class FiducialMarkerDecisionTree : public DecisionTree
    {
    public:
      typedef struct {
        s16 probeXCenter; // or leafLabelStart if this is a leaf node
        s16 probeYCenter; // or leafLabelEnd if this is a leaf node
        u16 leftChildIndex;
        u16 label; //< If the high bit of the label is 1, then this node is a leaf
      } Node;
      
      FiducialMarkerDecisionTree();

      // treeDataLength is the number of bytes in the treeData buffer
      // Warning: The input treeData buffer is not copied, and must be globally available
      FiducialMarkerDecisionTree(const void * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth, const s16 * restrict probeXOffsets, const s16 * restrict probeYOffsets, const s32 numProbeOffsets, const u16 * restrict leafLabels, const s32 numLeafLabels);

      // Transform the image coordinates via transformation, then query the tree
      // If the mean of all probes if greater than grayvalueThreshold, then that point is considered white
      Result Classify(const Array<u8> &image, const Array<f32> &homography,
                      const u8 meanGrayvalueThreshold, s32 &label) const;
      
      Result Verify(const Array<u8> &image, const Array<f32> &homography,
                    const u8 meanGrayvalueThreshold, const s32 label,
                    bool& isVerified) const;

      bool IsValid() const;

    protected:
      const s16 * restrict probeXOffsets;
      const s16 * restrict probeYOffsets;
      s32 numProbeOffsets;
      
      Result FindLeaf(const Array<u8> &image, const Array<f32> &homography,
                      const u8 meanGrayvalueThreshold, s32& nodeIndex) const;
      
    }; // class DecisionTree
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DECISION_TREE_H_
