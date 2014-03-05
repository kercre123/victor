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
      : DecisionTree(), probeXOffsets(NULL), probeYOffsets(NULL), numProbeOffsets(-1)
    {
    }

    FiducialMarkerDecisionTree::FiducialMarkerDecisionTree(const u8 * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth, const s16 * restrict probeXOffsets, const s16 * restrict probeYOffsets, const s32 numProbeOffsets)
      : DecisionTree(treeData, treeDataLength, treeDataNumFractionalBits, treeMaxDepth), probeXOffsets(probeXOffsets), probeYOffsets(probeYOffsets), numProbeOffsets(numProbeOffsets)
    {
    }

    Result FiducialMarkerDecisionTree::Classify(const Array<u8> &image, const Array<f32> &homography, u32 grayvalueThreshold, s32 &label) const
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid() && homography.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "FiducialMarkerDecisionTree::Classify", "Inputs are not valid");

      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "FiducialMarkerDecisionTree::Classify", "This object is not valid");

      // This function is optimized for 9 or fewer probes, but this is not a big issue
      AnkiAssert(numProbeOffsets <= 9);

      const s32 numProbeOffsets = this->numProbeOffsets;

      const f32 h00 = homography[0][0];
      const f32 h10 = homography[1][0];
      const f32 h20 = homography[2][0];
      const f32 h01 = homography[0][1];
      const f32 h11 = homography[1][1];
      const f32 h21 = homography[2][1];
      const f32 h02 = homography[0][2];
      const f32 h12 = homography[1][2];
      const f32 h22 = homography[2][2];

      f32 fixedPointDivider;

      if(this->treeDataNumFractionalBits == 0) {
        fixedPointDivider = 1.0f;
      } else {
        fixedPointDivider = 1.0f / static_cast<f32>(2 << (this->treeDataNumFractionalBits-1));
      }

      f32 probeXOffsetsF32[9];
      f32 probeYOffsetsF32[9];

      for(s32 iProbe=0; iProbe<numProbeOffsets; iProbe++) {
        probeXOffsetsF32[iProbe] = static_cast<f32>(this->probeXOffsets[iProbe]) * fixedPointDivider;
        probeYOffsetsF32[iProbe] = static_cast<f32>(this->probeYOffsets[iProbe]) * fixedPointDivider;
      }

      const Node * restrict pTreeData = reinterpret_cast<const Node*>(this->treeData);

      s32 nodeIndex = 0;
      for(s32 iDepth=0; iDepth<=treeMaxDepth; iDepth++) {
        const bool isLeaf = static_cast<bool>(pTreeData[nodeIndex].label & (1<<15));

        if(isLeaf) {
          label = pTreeData[nodeIndex].label & 0x7FFF;
          return RESULT_OK;
        }

        const f32 xCenter = static_cast<f32>(pTreeData[nodeIndex].probeXCenter) * fixedPointDivider;
        const f32 yCenter = static_cast<f32>(pTreeData[nodeIndex].probeYCenter) * fixedPointDivider;

        u32 accumulator = 0;
        for(s32 iProbe=0; iProbe<numProbeOffsets; iProbe++) {
          // 1. Map each probe to its warped locations
          const f32 x = xCenter + probeXOffsetsF32[iProbe];
          const f32 y = yCenter + probeYOffsetsF32[iProbe];

          const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);

          const f32 warpedXf = (h00 * x + h01 *y + h02) * homogenousDivisor;
          const f32 warpedYf = (h10 * x + h11 *y + h12) * homogenousDivisor;

          const s32 warpedX = RoundS32(warpedXf);
          const s32 warpedY = RoundS32(warpedYf);

          // 2. Sample the image

          // This should only fail if there's a bug in the quad extraction
          AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < imageHeight && warpedX < imageWidth);

          const u8 imageValue = *image.Pointer(warpedY, warpedX);

          accumulator += imageValue;
        } // for(s32 iProbe=0; iProbe<numProbeOffsets; iProbe++)

        // If the point is white, go to the left child
        // If the point is black, go to the right child
        if(accumulator > grayvalueThreshold) {
          nodeIndex = pTreeData[nodeIndex].leftChildIndex;
        } else {
          nodeIndex = pTreeData[nodeIndex].leftChildIndex + 1;
        }
      } // for(s32 iDepth=0; iDepth<treeMaxDepth; iDepth++)

      AnkiError("FiducialMarkerDecisionTree::Classify", "Decision tree ran out of bounds");

      // If this is reached, we ran out of tree to query
      return RESULT_FAIL;
    }
  } // namespace Embedded
} // namespace Anki
