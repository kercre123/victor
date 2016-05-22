/**
File: cascadeClassifier.cpp
Author: Peter Barnum
Created: 2014-04-11

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

// Deprecate?
#if 0

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "anki/vision/robot/classifier.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/hostIntrinsics_m4.h"

#include "anki/vision/robot/imageProcessing.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/objdetect/cascadedetect.hpp"
#endif

#if !defined(__EDG__)
#include <time.h>
#endif

//#define EXACTLY_MATCH_OPENCV

#define USE_ARM_ACCELERATION

#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif

#define CALC_SUM_(p0, p1, p2, p3, offset) \
  ((p0)[offset] - (p1)[offset] - (p2)[offset] + (p3)[offset])

#define ANKI_SUM_PTRS( p0, p1, p2, p3, sum, rect, step ) \
  /* (x, y) */                                           \
  (p0) = sum + (rect).left  + (step) * (rect).top,       \
  /* (x + w, y) */                                       \
  (p1) = sum + (rect).right + (step) * (rect).top,       \
  /* (x, y + h) */                                       \
  (p2) = sum + (rect).left  + (step) * (rect).bottom,    \
  /* (x + w, y + h) */                                   \
  (p3) = sum + (rect).right + (step) * (rect).bottom

namespace Anki
{
  namespace Embedded
  {
    namespace Classifier
    {
      class SimilarRects
      {
      public:
        SimilarRects(f32 _eps) : eps(_eps) {}

        inline bool operator()(const Rectangle<s32> &r1, const Rectangle<s32> &r2) const
        {
          const s32 width1 = r1.right - r1.left;
          const s32 width2 = r2.right - r2.left;

          const s32 height1 = r1.bottom - r1.top;
          const s32 height2 = r2.bottom - r2.top;

          f32 delta = eps * (MIN(width1, width2) + MIN(height1, height2)) * 0.5f;

          return
            ABS(r1.left - r2.left) <= delta &&
            ABS(r1.top - r2.top) <= delta &&
            ABS(r1.right - r2.right) <= delta &&
            ABS(r1.bottom - r2.bottom) <= delta;
        }

        f32 eps;
      }; // class SimilarRects

      void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps, MemoryStack scratch)
      {
        GroupRectangles(rectList, groupThreshold, eps, NULL, NULL, scratch);
      } // void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps)

      void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps, FixedLengthList<s32>* weights, FixedLengthList<f32>* levelWeights, MemoryStack scratch)
      {
        if( groupThreshold <= 0 || rectList.get_size()==0 )
        {
          if( weights )
          {
            s32 sz = rectList.get_size();
            weights->set_size(sz);
            for(s32 i = 0; i < sz; i++ )
              (*weights)[i] = 1;
          }
          return;
        }

        FixedLengthList<int> labels(rectList.get_size(), scratch, Flags::Buffer(true, false, true));

        const int nclasses = Partition(rectList, labels, SimilarRects(eps), scratch);

        FixedLengthList<Rectangle<s32> > rrects(nclasses, scratch, Flags::Buffer(true, false, true));

        FixedLengthList<int> rweights(nclasses, scratch, Flags::Buffer(true, false, true));

        FixedLengthList<int> rejectLevels(nclasses, scratch, Flags::Buffer(true, false, true));

        FixedLengthList<f32> rejectWeights(nclasses, scratch, Flags::Buffer(false, false, true));
        rejectWeights.Set(FLT_MAX);

        const int nlabels = (int)labels.get_size();

        for(s32 i = 0; i < nlabels; i++ )
        {
          const int cls = labels[i];
          rrects[cls].left += rectList[i].left;
          rrects[cls].right += rectList[i].right;
          rrects[cls].top += rectList[i].top;
          rrects[cls].bottom += rectList[i].bottom;
          rweights[cls]++;
        }

        if ( levelWeights && weights && weights->get_size()!=0 && levelWeights->get_size()!=0 )
        {
          for(s32 i = 0; i < nlabels; i++ )
          {
            const int cls = labels[i];

            if( (*weights)[i] > rejectLevels[cls] )
            {
              rejectLevels[cls] = (*weights)[i];
              rejectWeights[cls] = (*levelWeights)[i];
            }
            else if( ( (*weights)[i] == rejectLevels[cls] ) && ( (*levelWeights)[i] > rejectWeights[cls] ) )
              rejectWeights[cls] = (*levelWeights)[i];
          }
        }

        for(s32 i = 0; i < nclasses; i++ )
        {
          const Rectangle<s32> r = rrects[i];
          const f32 s = 1.f/rweights[i];

          rrects[i] = Rectangle<s32>(
            Round<s32>(r.left*s),
            Round<s32>(r.right*s),
            Round<s32>(r.top*s),
            Round<s32>(r.bottom*s));
        }

        rectList.Clear();

        if( weights )
          weights->Clear();

        if( levelWeights )
          levelWeights->Clear();

        for(s32 i = 0; i < nclasses; i++ )
        {
          const Rectangle<s32> r1 = rrects[i];
          const int n1 = levelWeights ? rejectLevels[i] : rweights[i];
          const f32 w1 = rejectWeights[i];

          if( n1 <= groupThreshold )
            continue;

          s32 j;

          // filter out small face rectangles inside large rectangles
          for(j = 0; j < nclasses; j++ )
          {
            const int n2 = rweights[j];

            if( j == i || n2 <= groupThreshold )
              continue;

            const Rectangle<s32> r2 = rrects[j];

            //const s32 width1 = r1.right - r1.left;
            const s32 width2 = r2.right - r2.left;

            //const s32 height1 = r1.bottom - r1.top;
            const s32 height2 = r2.bottom - r2.top;

            int dx = Round<s32>( width2 * eps );
            int dy = Round<s32>( height2 * eps );

            if( i != j &&
              r1.left >= r2.left - dx &&
              r1.top >= r2.top - dy &&
              r1.right <= r2.right + dx &&
              r1.bottom <= r2.bottom + dy &&
              (n2 > MAX(3, n1) || n1 < 3) )
              break;
          }

          if( j == nclasses )
          {
            rectList.PushBack(r1);

            if( weights )
              weights->PushBack(n1);

            if( levelWeights )
              levelWeights->PushBack(w1);
          }
        }
      } // void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps, FixedLengthList<s32>* weights, FixedLengthList<f32>* levelWeights)

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
        AnkiConditionalErrorAndReturn(AreValid(stages, classifiers, nodes, leaves, subsets),
          "CascadeClassifier::CascadeClassifier", "Invalid objects");

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

        this->isValid = true;
      } // CascadeClassifier::CascadeClassifier()

      bool CascadeClassifier::IsValid() const
      {
        if(!isValid)
          return false;

        if(!AreValid(data.stages, data.classifiers, data.nodes, data.leaves, data.subsets))
          return false;

        if(data.stages.get_size() != data.stages.get_maximumSize())
          return false;

        if(data.classifiers.get_size() != data.classifiers.get_maximumSize())
          return false;

        if(data.nodes.get_size() != data.nodes.get_maximumSize())
          return false;

        if(data.leaves.get_size() != data.leaves.get_maximumSize())
          return false;

        if(data.subsets.get_size() != data.subsets.get_maximumSize())
          return false;

        return true;
      } // bool CascadeClassifier::IsValid() const

      bool CascadeClassifier_LBP::IsValid() const
      {
        if(!CascadeClassifier::IsValid())
          return false;

        if(features.get_size() != features.get_maximumSize())
          return false;

        return true;
      } // bool CascadeClassifier::IsValid() const

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      CascadeClassifier_LBP::CascadeClassifier_LBP(const char * filename, MemoryStack &memory)
      {
        this->isValid = false;

        cv::CascadeClassifier opencvCascade;

        const bool loadSucceeded = opencvCascade.load(filename);

        AnkiConditionalErrorAndReturn(loadSucceeded,
          "CascadeClassifier_LBP::CascadeClassifier_LBP", "Could not load %s", filename);

        this->data.isStumpBased = opencvCascade.data.isStumpBased;
        this->data.stageType = opencvCascade.data.stageType;
        this->data.featureType = opencvCascade.data.featureType;
        this->data.ncategories = opencvCascade.data.ncategories;
        this->data.origWinHeight = opencvCascade.data.origWinSize.height;
        this->data.origWinWidth = opencvCascade.data.origWinSize.width;

        this->data.stages = FixedLengthList<CascadeClassifier::Stage>(static_cast<s32>(opencvCascade.data.stages.size()), memory);
        this->data.stages.set_size(static_cast<s32>(opencvCascade.data.stages.size()));

        this->data.classifiers = FixedLengthList<CascadeClassifier::DTree>(static_cast<s32>(opencvCascade.data.classifiers.size()), memory);
        this->data.classifiers.set_size(static_cast<s32>(opencvCascade.data.classifiers.size()));

        this->data.nodes = FixedLengthList<CascadeClassifier::DTreeNode>(static_cast<s32>(opencvCascade.data.nodes.size()), memory);
        this->data.nodes.set_size(static_cast<s32>(opencvCascade.data.nodes.size()));

        this->data.leaves = FixedLengthList<f32>(static_cast<s32>(opencvCascade.data.leaves.size()), memory);
        this->data.leaves.set_size(static_cast<s32>(opencvCascade.data.leaves.size()));

        this->data.subsets = FixedLengthList<s32>(static_cast<s32>(opencvCascade.data.subsets.size()), memory);
        this->data.subsets.set_size(static_cast<s32>(opencvCascade.data.subsets.size()));

        const cv::LBPEvaluator* evaluator = static_cast<cv::LBPEvaluator*>(opencvCascade.featureEvaluator.obj);

        this->features = FixedLengthList<LBPFeature>(static_cast<s32>(evaluator->features->size()), memory);
        this->features.set_size(static_cast<s32>(evaluator->features->size()));

        AnkiConditionalErrorAndReturn(AreValid(this->data.stages, this->data.classifiers, this->data.nodes, this->data.leaves, this->data.subsets, this->features),
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

        for(s32 i=0; i<this->features.get_size(); i++) {
          const s32 x = (*evaluator->features)[i].rect.x;
          const s32 y = (*evaluator->features)[i].rect.y;
          const s32 width = (*evaluator->features)[i].rect.width;
          const s32 height = (*evaluator->features)[i].rect.height;

          this->features[i].rect.left  = x;
          this->features[i].rect.right = x + width;

          this->features[i].rect.top    = y;
          this->features[i].rect.bottom = y + height;
        }

        this->isValid = true;
      } // CascadeClassifier_LBP::CascadeClassifier(const char * filename, MemoryStack &memory)

      Result CascadeClassifier_LBP::SaveAsHeader(const char * filename, const char * objectName)
      {
        FILE *file;

        file = fopen(filename, "w");

        AnkiConditionalErrorAndReturnValue(file,
          RESULT_FAIL_IO, "CascadeClassifier::SaveAsHeader", "Could not open file %s", filename);

        const time_t curTime = time(NULL);
        const struct tm curLocalTime = *localtime(&curTime);

        const s32 maxStringLength = 1024;
        char headerDefineString[maxStringLength];
        snprintf(headerDefineString, maxStringLength, "_CASCADECLASSIFIER_%s_H_", objectName);

        fprintf(file,
          "// Autogenerated by cascadeClassifier.cpp on %04d-%02d-%02d at %02d:%02d:%02d\n\n", curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday, curLocalTime.tm_hour, curLocalTime.tm_min, curLocalTime.tm_sec);

        fprintf(file,
          "#ifndef %s\n"
          "#define %s\n\n"
          "#include \"anki/common/robot/config.h\"\n"
          "#include \"anki/vision/robot/classifier.h\"\n\n", headerDefineString, headerDefineString);

        if(this->data.isStumpBased) {
          fprintf(file,
            "const bool %s_isStumpBased = true;\n", objectName);
        } else {
          fprintf(file,
            "const bool %s_isStumpBased = false;\n", objectName);
        }

        fprintf(file,
          "const s32 %s_stageType = %d;\n"
          "const s32 %s_featureType = %d;\n"
          "const s32 %s_ncategories = %d;\n"
          "const s32 %s_origWinHeight = %d;\n"
          "const s32 %s_origWinWidth = %d;\n\n",
          objectName, this->data.stageType,
          objectName, this->data.featureType,
          objectName, this->data.ncategories,
          objectName, this->data.origWinHeight,
          objectName, this->data.origWinWidth);

        fprintf(file,
          "#define %s_stages_length %d\n"
          "#define %s_classifiers_length %d\n"
          "#define %s_nodes_length %d\n"
          "#define %s_leaves_length %d\n"
          "#define %s_subsets_length %d\n"
          "#define %s_featureRectangles_length %d\n\n",
          objectName, this->data.stages.get_size(),
          objectName, this->data.classifiers.get_size(),
          objectName, this->data.nodes.get_size(),
          objectName, this->data.leaves.get_size(),
          objectName, this->data.subsets.get_size(),
          objectName, this->features.get_size());

        // FixedLengthList<Stage> stages;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const Anki::Embedded::Classifier::CascadeClassifier::Stage %s_stages_data[%s_stages_length + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.stages.get_size(); i++) {
          fprintf(file,
            "{%d,%d,%ff},", this->data.stages[i].first, this->data.stages[i].ntrees, this->data.stages[i].threshold);
        }

        fprintf(file,
          "}; // %s_stages_data\n\n\n",
          objectName);

        // FixedLengthList<DTree> classifiers;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const Anki::Embedded::Classifier::CascadeClassifier::DTree %s_classifiers_data[%s_classifiers_length + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.classifiers.get_size(); i++) {
          fprintf(file,
            "%d,", this->data.classifiers[i].nodeCount);
        }

        fprintf(file,
          "}; // %s_classifiers_data\n\n\n",
          objectName);

        // FixedLengthList<DTreeNode> nodes;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const Anki::Embedded::Classifier::CascadeClassifier::DTreeNode %s_nodes_data[%s_nodes_length + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.nodes.get_size(); i++) {
          fprintf(file,
            "{%d,%ff,%d,%d},", this->data.nodes[i].featureIdx, this->data.nodes[i].threshold, this->data.nodes[i].left, this->data.nodes[i].right);
        }

        fprintf(file,
          "}; // %s_nodes_data\n\n\n",
          objectName);

        // FixedLengthList<f32> leaves;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const f32 %s_leaves_data[%s_leaves_length + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.leaves.get_size(); i++) {
          fprintf(file,
            "%ff,", this->data.leaves[i]);
        }

        fprintf(file,
          "}; // %s_leaves_data\n\n\n",
          objectName);

        // FixedLengthList<s32> subsets;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const s32 %s_subsets_data[%s_subsets_length + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.subsets.get_size(); i++) {
          fprintf(file,
            "%d,", this->data.subsets[i]);
        }

        fprintf(file,
          "}; // %s_subsets_data\n\n\n",
          objectName);

        // FixedLengthList<LBPFeature> features;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const s32 %s_featureRectangles_data[%s_featureRectangles_length*4 + MEMORY_ALIGNMENT_RAW]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->features.get_size(); i++) {
          fprintf(file,
            "%d,%d,%d,%d,", this->features[i].rect.left, this->features[i].rect.right, this->features[i].rect.top, this->features[i].rect.bottom);
        }

        fprintf(file,
          "}; // %s_featureRectangles_data\n\n\n",
          objectName);

        fprintf(file,
          "#endif // %s\n", headerDefineString);

        fclose(file);

        return RESULT_OK;
      } // Result CascadeClassifier_LBP::SaveAsHeader(const char * filename, const char * objectName)
#endif

      CascadeClassifier_LBP::CascadeClassifier_LBP()
        : CascadeClassifier()
      {
      }

      CascadeClassifier_LBP::CascadeClassifier_LBP(
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
        const FixedLengthList<s32> &subsets,
        const FixedLengthList<Rectangle<s32> > &featureRectangles, //< Rectangles will be copied into this object's FixedLengthList<LBPFeature> features (which is allocated from memory)
        MemoryStack &memory)
        : CascadeClassifier(isStumpBased, stageType, featureType, ncategories, origWinHeight, origWinWidth, stages, classifiers, nodes, leaves, subsets)
      {
        this->isValid = false;

        this->features = FixedLengthList<LBPFeature>(featureRectangles.get_size(), memory, Flags::Buffer(true, false, true));

        AnkiConditionalErrorAndReturn(AreValid(featureRectangles, this->features),
          "CascadeClassifier_LBP::CascadeClassifier_LBP", "Invalid objects");

        const s32 numFeatures = featureRectangles.get_size();

        const Rectangle<s32> * restrict pFeatureRectangles = featureRectangles.Pointer(0);
        LBPFeature * restrict pFeatures = features.Pointer(0);

        for(s32 i=0; i<numFeatures; i++) {
          pFeatures[i].rect = pFeatureRectangles[i];
        }

        this->isValid = true;
      }

      Result CascadeClassifier_LBP::DetectMultiScale(
        const Array<u8> &image,
        const f32 scaleFactor,
        const s32 minNeighbors,
        const s32 minObjectHeight,
        const s32 minObjectWidth,
        const s32 maxObjectHeight,
        const s32 maxObjectWidth,
        FixedLengthList<Rectangle<s32> > &objects,
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        const f32 GROUP_EPS = 0.2f;

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(AreValid(*this, image, objects) && (image.get_size(0) > 0),
          RESULT_FAIL_INVALID_OBJECT, "CascadeClassifier::DetectMultiScale", "Invalid objects");

        BeginBenchmark("CascadeClassifier_LBP::DetectMultiScale");

        objects.Clear();

        FixedLengthList<Rectangle<s32> > candidates(objects.get_maximumSize(), slowScratch);

        for(f32 factor = 1; ; factor *= scaleFactor) {
          PUSH_MEMORY_STACK(fastScratch);

          BeginBenchmark("CascadeClassifier_LBP::DetectMultiScale main loop");

          const s32 windowHeight = Round<s32>(this->data.origWinHeight*factor);
          const s32 windowWidth = Round<s32>(this->data.origWinWidth*factor);

          const s32 scaledImageHeight = Round<s32>(imageHeight / factor);
          const s32 scaledImageWidth = Round<s32>(imageWidth / factor);

          const s32 processingRectHeight = scaledImageHeight - this->data.origWinHeight;
          const s32 processingRectWidth = scaledImageWidth - this->data.origWinWidth;

          if( processingRectWidth <= 0 || processingRectHeight <= 0 )
            break;

          if( windowWidth > maxObjectWidth || windowHeight > maxObjectHeight )
            break;

          if( windowWidth < minObjectWidth || windowHeight < minObjectHeight )
            continue;

          Array<u8> scaledImage(scaledImageHeight, scaledImageWidth, fastScratch, Flags::Buffer(false, false, false));

          AnkiConditionalErrorAndReturnValue(AreValid(scaledImage),
            RESULT_FAIL_OUT_OF_MEMORY, "CascadeClassifier::DetectMultiScale", "Out of memory");

          BeginBenchmark("Resize");
          // OpenCV's resize grayvalues may be 1 or so different than Anki's
#ifdef EXACTLY_MATCH_OPENCV
          {
            const cv::Size scaledImageSize = cv::Size(scaledImageWidth,scaledImageHeight);

            Array<u8> tmpImHack = image;

            cv::Mat_<u8> tmpImHack_cvMat;
            tmpImHack.ArrayToCvMat(&tmpImHack_cvMat);

            cv::Mat_<u8> scaledImage_cvMat;
            scaledImage.ArrayToCvMat(&scaledImage_cvMat);

            cv::resize(tmpImHack_cvMat, scaledImage_cvMat, scaledImageSize, 0, 0, CV_INTER_LINEAR);
          }
#else
          //ImageProcessing::Resize(image, scaledImage);
          ImageProcessing::DownsampleBilinear(image, scaledImage, fastScratch);
          //scaledImage.Show("scaledImage", true);
#endif

          EndBenchmark("Resize");

          const s32 xyIncrement = factor > 2.0f ? 1 : 2;

          BeginBenchmark("DetectSingleScale");

          if(DetectSingleScale(scaledImage, processingRectHeight, processingRectWidth, xyIncrement, factor, candidates, fastScratch) != RESULT_OK) {
            break;
          }

          EndBenchmark("DetectSingleScale");

          EndBenchmark("CascadeClassifier_LBP::DetectMultiScale main loop");
        } // for(f32 factor = 1; ; factor *= scaleFactor)

        objects.set_size(candidates.get_size());
        memcpy(objects.Pointer(0), candidates.Pointer(0), candidates.get_array().get_stride());

        GroupRectangles(objects, minNeighbors, GROUP_EPS, fastScratch);

        //EndBenchmark("CascadeClassifier_LBP::DetectMultiScale");

        return RESULT_OK;
      }

      Result CascadeClassifier_LBP::DetectSingleScale(
        const Array<u8> &image,
        const s32 processingRectHeight,
        const s32 processingRectWidth,
        const s32 xyIncrement, //< Same as openCV yStep
        const f32 scaleFactor,
        FixedLengthList<Rectangle<s32> > &candidates,
        MemoryStack scratch)
      {
        BeginBenchmark("CascadeClassifier_LBP::DetectSingleScale");

        BeginBenchmark("CascadeClassifier_LBP::DetectSingleScale init");

        const s32 winHeight = Round<s32>(this->data.origWinHeight * scaleFactor);
        const s32 winWidth = Round<s32>(this->data.origWinWidth * scaleFactor);

        const s32 nfeatures = this->features.get_size();

        const s32 scrollingIntegralImage_numScrollRows = 16;
        const s32 scrollingIntegralImage_bufferHeight = MIN(image.get_size(0)+2, winHeight + scrollingIntegralImage_numScrollRows + 2);

        ScrollingIntegralImage_u8_s32 scrollingIntegralImage(scrollingIntegralImage_bufferHeight, image.get_size(1), 1, scratch);

        scrollingIntegralImage.ScrollDown(image, scrollingIntegralImage_bufferHeight, scratch);

        //const IntegralImage_u8_s32 integralImage(image, scratch);

        for(s32 fi = 0; fi < nfeatures; fi++) {
          this->features[fi].updatePtrs(scrollingIntegralImage);
        }

        EndBenchmark("CascadeClassifier_LBP::DetectSingleScale init");

        for(s32 y = 0; y < processingRectHeight; y += xyIncrement) {
          //BeginBenchmark("CascadeClassifier_LBP::DetectSingleScale scrollIntegralImage");

          if(scrollingIntegralImage.get_maxRow(this->data.origWinHeight+xyIncrement) < y) {
            const Result lastResult = scrollingIntegralImage.ScrollDown(image, scrollingIntegralImage_numScrollRows, scratch);

            if(lastResult != RESULT_OK)
              return lastResult;
          }

          //EndBenchmark("CascadeClassifier_LBP::DetectSingleScale scrollIntegralImage");

          //BeginBenchmark("CascadeClassifier_LBP::DetectSingleScale x loop");
          for(s32 x = 0; x < processingRectWidth; x += xyIncrement) {
            f32 gypWeight;

            const int result = this->PredictCategoricalStump(scrollingIntegralImage, Point<s16>(x,y), gypWeight);

            if( result > 0 ) {
              const s32 x0 = Round<s32>(x*scaleFactor);
              const s32 y0 = Round<s32>(y*scaleFactor);

              // TODO: should be width/height minus one?
              candidates.PushBack(Rectangle<s32>(
                x0, x0 + winWidth,
                y0, y0 + winHeight));
            }

            // TODO: should the xStep be twice the xyIncrement?
            if(result == 0)
              x += xyIncrement;
          }
          //EndBenchmark("CascadeClassifier_LBP::DetectSingleScale x loop");
        }

        EndBenchmark("CascadeClassifier_LBP::DetectSingleScale");

        return RESULT_OK;
      }

      s32 CascadeClassifier_LBP::PredictCategoricalStump(const ScrollingIntegralImage_u8_s32 &integralImage, const Point<s16> &location, f32& sum) const
      {
        const int nstages = (int)this->data.stages.get_size();
        int nodeOfs = 0, leafOfs = 0;
        const size_t subsetSize = (this->data.ncategories + 31)/32;
        const int* cascadeSubsets = &this->data.subsets[0];
        const f32* cascadeLeaves = &this->data.leaves[0];
        const CascadeClassifier::DTreeNode* cascadeNodes = &this->data.nodes[0];
        const CascadeClassifier::Stage* cascadeStages = &this->data.stages[0];

        const s32 offset = location.x + (location.y - integralImage.get_rowOffset() - 1) * (integralImage.get_stride() / sizeof(s32));

        for( int si = 0; si < nstages; si++ )
        {
          const CascadeClassifier::Stage& stage = cascadeStages[si];
          int wi, ntrees = stage.ntrees;

          sum = 0;

          for( wi = 0; wi < ntrees; wi++ )
          {
            const CascadeClassifier::DTreeNode& node = cascadeNodes[nodeOfs];

            const int c = this->features[node.featureIdx].calc(offset);

            const int* subset = &cascadeSubsets[nodeOfs*subsetSize];

            sum += cascadeLeaves[ subset[c>>5] & (1 << (c & 31)) ? leafOfs : leafOfs+1];

            nodeOfs++;
            leafOfs += 2;
          }

          if( sum < stage.threshold )
            return -si;
        }

        return 1;
      }

      void CascadeClassifier_LBP::LBPFeature::updatePtrs(const ScrollingIntegralImage_u8_s32 &sum)
      {
        const int* ptr = sum.Pointer(0,0);
        const s32 step = sum.get_stride() / sizeof(s32);
        Rectangle<s32> tr = rect;

        const s32 width = tr.right - tr.left;
        const s32 height = tr.bottom - tr.top;

        ANKI_SUM_PTRS( p[0], p[1], p[4], p[5], ptr, tr, step );
        tr.left += 2*width;
        tr.right += 2*width;

        ANKI_SUM_PTRS( p[2], p[3], p[6], p[7], ptr, tr, step );
        tr.top += 2*height;
        tr.bottom += 2*height;

        ANKI_SUM_PTRS( p[10], p[11], p[14], p[15], ptr, tr, step );
        tr.left -= 2*width;
        tr.right -= 2*width;

        ANKI_SUM_PTRS( p[8], p[9], p[12], p[13], ptr, tr, step );
      }

      inline int CascadeClassifier_LBP::LBPFeature::calc(const int _offset) const
      {
        int cval = CALC_SUM_( p[5], p[6], p[9], p[10], _offset );

        return (CALC_SUM_( p[0], p[1], p[4], p[5], _offset ) >= cval ? 128 : 0) |   // 0
          (CALC_SUM_( p[1], p[2], p[5], p[6], _offset ) >= cval ? 64 : 0) |    // 1
          (CALC_SUM_( p[2], p[3], p[6], p[7], _offset ) >= cval ? 32 : 0) |    // 2
          (CALC_SUM_( p[6], p[7], p[10], p[11], _offset ) >= cval ? 16 : 0) |  // 5
          (CALC_SUM_( p[10], p[11], p[14], p[15], _offset ) >= cval ? 8 : 0)|  // 8
          (CALC_SUM_( p[9], p[10], p[13], p[14], _offset ) >= cval ? 4 : 0)|   // 7
          (CALC_SUM_( p[8], p[9], p[12], p[13], _offset ) >= cval ? 2 : 0)|    // 6
          (CALC_SUM_( p[4], p[5], p[8], p[9], _offset ) >= cval ? 1 : 0);
      }

      void CascadeClassifier_LBP::LBPFeature::Print() const
      {
        return this->rect.Print();
      }
    } // namespace Classifier
  } // namespace Embedded
} // namespace Anki

#endif // #if 0
