/**
File: decisionTree.h
Author: Peter Barnum
Created: 2014-03-03

The main prototype for an embedded decision tree. It's essentially an interface, but without the expicit overhead.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_DECISION_TREE_H_
#define _ANKICORETECHEMBEDDED_COMMON_DECISION_TREE_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    class DecisionTree
    {
    public:
      DecisionTree();

      // treeDataLength is the number of bytes in the treeData buffer
      // Warning: The input treeData buffer is not copied, and must be globally available
      DecisionTree(const void * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth, const u16 * restrict leafLabels, const s32 numLeafLabels);

      // TODO: These could be virtual methods, but I'm avoiding such things on the embedded side

      // Query the tree using the input image
      //Result Classify(const Array<u8> &image);

      bool IsValid() const;

      s32 get_numFractionalBits() const;
      s32 get_numMaxDepth() const;

    protected:
      const void * restrict treeData;
      s32 treeDataLength;
      s32 treeDataNumFractionalBits;
      s32 treeMaxDepth;
      
      const u16 * restrict leafLabels;
      s32 numLeafLabels;
      
    }; // class DecisionTree
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_DECISION_TREE_H_
