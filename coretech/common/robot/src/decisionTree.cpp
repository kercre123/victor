/**
File: decisionTree.cpp
Author: Peter Barnum
Created: 2014-03-03

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/decisionTree.h"

namespace Anki
{
  namespace Embedded
  {
    DecisionTree::DecisionTree()
      : treeData(NULL), treeDataLength(-1), treeDataNumFractionalBits(-1), treeMaxDepth(-1)
    {
    }

    DecisionTree::DecisionTree(const u8 * restrict treeData, const s32 treeDataLength, const s32 treeDataNumFractionalBits, const s32 treeMaxDepth)
      : treeData(NULL), treeDataLength(-1), treeDataNumFractionalBits(-1), treeMaxDepth(-1)
    {
      AnkiConditionalErrorAndReturn(treeData != NULL, "DecisionTree::DecisionTree", "treeData is NULL");
      AnkiConditionalErrorAndReturn(treeDataLength > 0, "DecisionTree::DecisionTree", "treeDataLength <= 0");
      AnkiConditionalErrorAndReturn(treeDataNumFractionalBits >= 0 && treeDataNumFractionalBits <= 32, "DecisionTree::DecisionTree", "0 <= treeDataNumFractionalBits <= 32");

      this->treeData = treeData;
      this->treeDataLength = treeDataLength;
      this->treeDataNumFractionalBits = treeDataNumFractionalBits;
      this->treeMaxDepth = treeMaxDepth;
    }

    bool DecisionTree::IsValid() const
    {
      if(!treeData)
        return false;

      return true;
    }
  } // namespace Embedded
} // namespace Anki
