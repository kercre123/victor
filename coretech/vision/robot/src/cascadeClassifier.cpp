/**
File: cascadeClassifier.cpp
Author: Peter Barnum
Created: 2014-04-11

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/classifier.h"
#include "anki/common/robot/matlabInterface.h"

#define USE_ARM_ACCELERATION

#if defined(__EDG__)
#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif
#else
#undef USE_ARM_ACCELERATION
#endif

#if defined(USE_ARM_ACCELERATION)
#ifdef USING_CHIP_SIMULATOR
#include <ARMCM4.h>
#else
#include <stm32f4xx.h>
#endif
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace Classifier
    {
      CascadeClassifier::CascadeClassifier()
        : isValid(false)
      {
      }

      CascadeClassifier::CascadeClassifier(
        const bool isStumpBased,
        const s32 stageType,
        const s32 featureType,
        const s32 ncategories,
        const s32 origWinHeight,
        const s32 origWinWidth,
        const FixedLengthList<CascadeClassifier::Stage> &stages,
        const FixedLengthList<CascadeClassifier::DTree> &classifiers,
        const FixedLengthList<CascadeClassifier::DTreeNode> &nodes,
        const FixedLengthList<f32> &leaves,
        const FixedLengthList<s32> &subsets)
        : isValid(false)
      {
        AnkiConditionalErrorAndReturn(stages.IsValid() && classifiers.IsValid() && nodes.IsValid() && leaves.IsValid() && subsets.IsValid(),
          "CascadeClassifier::CascadeClassifier", "Some inputs are not valid");

        this->data.isStumpBased = isStumpBased;
        this->data.stageType = stageType;
        this->data.featureType = featureType;
        this->data.ncategories = ncategories;
        this->data.origWinHeight = origWinHeight;
        this->data.origWinWidth = origWinWidth;

        this->data.stages = stages;
        this->data.classifiers = classifiers;
        this->data.nodes = nodes;
        this->data.leaves = leaves;
        this->data.subsets = subsets;

        this->IsValid = true;
      }

      bool CascadeClassifier::IsValid() const
      {
        if(!isValid)
          return false;

        if(!data.stages.IsValid())
          return false;

        if(!data.classifiers.IsValid())
          return false;

        if(!data.nodes.IsValid())
          return false;

        if(!data.leaves.IsValid())
          return false;

        if(!data.subsets.IsValid())
          return false;

        return true;
      }

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
      CascadeClassifier::CascadeClassifier(const char * filename, MemoryStack &memory)
        : isValid(false)
      {
        cv::CascadeClassifier opencvCascade;

        const bool loadSucceeded = opencvCascade.load(filename);

        AnkiConditionalErrorAndReturn(loadSucceeded,
          "CascadeClassifier::CascadeClassifier", "Could not load %s", filename);

        this->data.isStumpBased = opencvCascade.data.isStumpBased;
        this->data.stageType = opencvCascade.data.stageType;
        this->data.featureType = opencvCascade.data.featureType;
        this->data.ncategories = opencvCascade.data.ncategories;
        this->data.origWinHeight = opencvCascade.data.origWinSize.height;
        this->data.origWinWidth = opencvCascade.data.origWinSize.width;

        this->data.stages = FixedLengthList<CascadeClassifier::Stage>(static_cast<s32>(opencvCascade.data.stages.size()), memory);
        this->data.classifiers = FixedLengthList<CascadeClassifier::DTree>(static_cast<s32>(opencvCascade.data.classifiers.size()), memory);
        this->data.nodes = FixedLengthList<CascadeClassifier::DTreeNode>(static_cast<s32>(opencvCascade.data.nodes.size()), memory);

        this->data.leaves = FixedLengthList<f32>(static_cast<s32>(opencvCascade.data.leaves.size()), memory);
        this->data.subsets = FixedLengthList<s32>(static_cast<s32>(opencvCascade.data.subsets.size()), memory);

        AnkiConditionalErrorAndReturn(this->data.stages.IsValid() && this->data.classifiers.IsValid() && this->data.nodes.IsValid() && this->data.leaves.IsValid() && this->data.subsets.IsValid(),
          "CascadeClassifier::CascadeClassifier", "Out of memory copying %s", filename);

        for(s32 i=0; i<this->data.stages.get_size(); i++) {
          this->data.stages[i].first = opencvCascade.data.stages[i].first;
          this->data.stages[i].ntrees = opencvCascade.data.stages[i].ntrees;
          this->data.stages[i].threshold = opencvCascade.data.stages[i].threshold;
        }

        for(s32 i=0; i<this->data.classifiers.get_size(); i++) {
          this->data.classifiers[i].nodeCount = opencvCascade.data.classifiers[i].nodeCount;
        }

        for(s32 i=0; i<this->data.nodes.get_size(); i++) {
          this->data.nodes[i].featureIdx = opencvCascade.data.nodes[i].featureIdx;
          this->data.nodes[i].threshold = opencvCascade.data.nodes[i].threshold;
          this->data.nodes[i].left = opencvCascade.data.nodes[i].left;
          this->data.nodes[i].right = opencvCascade.data.nodes[i].right;
        }

        for(s32 i=0; i<this->data.leaves.get_size(); i++) {
          this->data.leaves[i] = opencvCascade.data.leaves[i];
        }

        for(s32 i=0; i<this->data.subsets.get_size(); i++) {
          this->data.subsets[i] = opencvCascade.data.subsets[i];
        }

        this->IsValid = true;
      }

      Result CascadeClassifier::SaveAsHeader(const char * filename, const char * objectName)
      {
      }
#endif
    } // namespace Classifier
  } // namespace Embedded
} // namespace Anki
