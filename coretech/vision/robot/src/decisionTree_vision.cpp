/**
File: decisionTree_vision.cpp
Author: Peter Barnum
Created: 2014-03-03

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/decisionTree_vision.h"

namespace Anki
{
  namespace Embedded
  {
    FiducialMarkerDecisionTree::FiducialMarkerDecisionTree()
      : DecisionTree()
    {
    }

    FiducialMarkerDecisionTree::FiducialMarkerDecisionTree(const u8 * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits)
      : DecisionTree(treeData, treeDataLength, treeDataNumFractionalBits)
    {
    }

    // Transform the image coordinates via transformation, then query the tree
    Result FiducialMarkerDecisionTree::Classify(const Array<u8> &image, const Transformations::PlanarTransformation_f32 &transformation)
    {
      // TODO: implement
      return RESULT_FAIL;
    }
  } // namespace Embedded
} // namespace Anki
