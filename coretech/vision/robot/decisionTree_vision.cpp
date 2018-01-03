/**
File: decisionTree_vision.cpp
Author: Peter Barnum
Created: 2014-03-03

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/decisionTree_vision.h"

namespace Anki
{
  namespace Embedded
  {
    FiducialMarkerDecisionTree::FiducialMarkerDecisionTree()
      : DecisionTree(), probeXOffsets(NULL), probeYOffsets(NULL), numProbeOffsets(-1)
    {
    }

    FiducialMarkerDecisionTree::FiducialMarkerDecisionTree(const void * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth, const s16 * restrict probeXOffsets, const s16 * restrict probeYOffsets, const s32 numProbeOffsets, const u16 * restrict leafLabels, const s32 numLeafLabels)
      : DecisionTree(treeData, treeDataLength, treeDataNumFractionalBits, treeMaxDepth, leafLabels, numLeafLabels), probeXOffsets(NULL), probeYOffsets(NULL), numProbeOffsets(-1)
    {
      AnkiConditionalErrorAndReturn(probeXOffsets != NULL && probeYOffsets != NULL,
        "FiducialMarkerDecisionTree::FiducialMarkerDecisionTree",
        "probes are NULL");

      AnkiConditionalErrorAndReturn(numProbeOffsets > 0,
        "FiducialMarkerDecisionTree::FiducialMarkerDecisionTree",
        "numProbeOffsets > 0");

      this->probeXOffsets = probeXOffsets;
      this->probeYOffsets = probeYOffsets;
      this->numProbeOffsets = numProbeOffsets;
    }

    Result FiducialMarkerDecisionTree::FindLeaf(const Array<u8> &image, const Array<f32> &homography,
      const u8 meanGrayvalueThreshold, s32& nodeIndex) const
    {
      AnkiConditionalErrorAndReturnValue(AreValid(*this, image, homography),
        RESULT_FAIL_INVALID_OBJECT, "FiducialMarkerDecisionTree::Classify", "Invalid objects");

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

      const u32 sumGrayvalueThreshold = meanGrayvalueThreshold * numProbeOffsets;

      f32 fixedPointDivider;

      fixedPointDivider = 1.0f / static_cast<f32>(1 << this->treeDataNumFractionalBits);

      f32 probeXOffsetsF32[9];
      f32 probeYOffsetsF32[9];

      for(s32 iProbe=0; iProbe<numProbeOffsets; iProbe++) {
        probeXOffsetsF32[iProbe] = static_cast<f32>(this->probeXOffsets[iProbe]) * fixedPointDivider;
        probeYOffsetsF32[iProbe] = static_cast<f32>(this->probeYOffsets[iProbe]) * fixedPointDivider;
      }

      const Node * restrict pTreeData = reinterpret_cast<const Node*>(this->treeData);

      nodeIndex = 0;
      for(s32 iDepth=0; iDepth<=treeMaxDepth; iDepth++) {
        const bool isLeaf = static_cast<bool>(pTreeData[nodeIndex].label & (1<<15));

        if(isLeaf) {
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

          const s32 warpedX = Round<s32>(warpedXf);
          const s32 warpedY = Round<s32>(warpedYf);

          // 2. Sample the image

          // This should only fail if there's a bug in the quad extraction
          AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < image.get_size(0) && warpedX < image.get_size(1)); // Verify y against image height and x against width

          const u8 imageValue = *image.Pointer(warpedY, warpedX);

          accumulator += imageValue;
        } // for(s32 iProbe=0; iProbe<numProbeOffsets; iProbe++)

        // If the point is black, go to the left child
        // If the point is white, go to the right child
        if(accumulator <= sumGrayvalueThreshold) {
          nodeIndex = pTreeData[nodeIndex].leftChildIndex;
        } else {
          nodeIndex = pTreeData[nodeIndex].leftChildIndex + 1;
        }
      } // for(s32 iDepth=0; iDepth<treeMaxDepth; iDepth++)

      AnkiError("FiducialMarkerDecisionTree::Classify", "Decision tree ran out of bounds");

      // If this is reached, we ran out of tree to query
      return RESULT_FAIL;
    } // FiducialMarkerDecisionTree::FindLeaf()

    Result FiducialMarkerDecisionTree::Classify(const Array<u8> &image, const Array<f32> &homography,
      const u8 meanGrayvalueThreshold, s32 &label) const
    {
      Result lastResult = RESULT_FAIL;

      s32 leafNodeIndex = -1;
      if((lastResult = FindLeaf(image, homography, meanGrayvalueThreshold, leafNodeIndex)) != RESULT_OK)
      {
        return lastResult;
      }

      const Node * restrict pTreeData = reinterpret_cast<const Node*>(this->treeData);

      // The X/Y center members of a node are actually the start/end indexes of
      // the labels list for leaf nodes.  If they are equal, there is a single
      // label at this leaf and it is simply stored in the label field.
      const s16 labelIndexStart = pTreeData[leafNodeIndex].probeXCenter;
      const s16 labelIndexEnd   = pTreeData[leafNodeIndex].probeYCenter;

      if(labelIndexStart == labelIndexEnd) {
        label = pTreeData[leafNodeIndex].label & 0x7FFF;
        lastResult = RESULT_OK;
      } else {
        // We should not be calling classify on a tree that has leaves with
        // multiple labels! Those are for verifying!
        AnkiError("FiducialMarkerDecisionTree::Classify",
          "Classification tree should not have leaves with multiple labels.");
        lastResult = RESULT_FAIL;
      }

      return lastResult;
    } // FiducialMarkerDecisionTree::Classify()

    Result FiducialMarkerDecisionTree::Verify(const Array<u8> &image, const Array<f32> &homography,
      const u8 meanGrayvalueThreshold, const s32 label,
      bool &isVerified) const
    {
      Result lastResult = RESULT_OK;

      s32 leafNodeIndex = -1;
      if((lastResult = FindLeaf(image, homography, meanGrayvalueThreshold, leafNodeIndex)) != RESULT_OK)
      {
        return lastResult;
      }

      const Node * restrict pTreeData = reinterpret_cast<const Node*>(this->treeData);

      // The X/Y center members of a node are actually the start/end indexes of
      // the labels list for leaf nodes.  If they are equal, there is a single
      // label at this leaf and it is simply stored in the label field.
      const s16 labelIndexStart = pTreeData[leafNodeIndex].probeXCenter;
      const s16 labelIndexEnd   = pTreeData[leafNodeIndex].probeYCenter;

      if(labelIndexStart == labelIndexEnd) {
        // simple case: the leaf node's label field _is_ the single label
        const u16 verifyLabel = pTreeData[leafNodeIndex].label & 0x7FFF;
        isVerified = label == verifyLabel;
      }
      else {
        // Check to see if the given label matches _any_ of the labels
        // at this leaf node
        isVerified = false;
        AnkiAssert(this->leafLabels != NULL);
        AnkiAssert(labelIndexStart >= 0 && labelIndexEnd <= this->numLeafLabels);
        const u16  * restrict pLeafLabels = this->leafLabels;
        for(s32 labelIndex = labelIndexStart; labelIndex < labelIndexEnd; ++labelIndex) {
          if(label == pLeafLabels[labelIndex]) {
            isVerified = true;
            break;
          }
        }
      }

      return lastResult;
    } // FiducialMarkerDecisionTree::Verify()

    bool FiducialMarkerDecisionTree::IsValid() const
    {
      if(DecisionTree::IsValid() == false)
        return false;

      if(!probeXOffsets)
        return false;

      if(!probeYOffsets)
        return false;

      return true;
    }
  } // namespace Embedded
} // namespace Anki
