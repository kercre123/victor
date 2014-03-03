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
      FiducialMarkerDecisionTree();

      // treeDataLength is the number of bytes in the treeData buffer
      // Warning: The input treeData buffer is not copied, and must be globally available
      FiducialMarkerDecisionTree(const u8 * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits);

      // Transform the image coordinates via transformation, then query the tree
      Result Classify(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation);
    }; // class DecisionTree
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DECISION_TREE_H_
