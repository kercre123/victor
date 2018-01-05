/**
File: classifier.h
Author: Peter Barnum
Created: 2014-04-11

Classify a query with a classifier. For example, detect faces in an image, using a cascade classifier.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

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

#ifndef _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_
#define _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/geometry.h"
#include "coretech/common/robot/fixedLengthList.h"

#include "coretech/vision/robot/integralImage.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Classifier
    {
      void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps, MemoryStack scratch);
      void GroupRectangles(FixedLengthList<Rectangle<s32> >& rectList, const s32 groupThreshold, const f32 eps, FixedLengthList<s32>* weights, FixedLengthList<f32>* levelWeights, MemoryStack scratch);

      // Based off OpenCV 2.4.8 CascadeClassifier
      class CascadeClassifier
      {
      public:
        struct DTreeNode
        {
          s32 featureIdx;
          f32 threshold; // for ordered features only
          s32 left;
          s32 right;
        };

        struct DTree
        {
          s32 nodeCount;
        };

        struct Stage
        {
          s32 first;
          s32 ntrees;
          f32 threshold;
        };

        CascadeClassifier();

        // WARNING: Some of these options aren't supported. They are here to match the OpenCV API
        // WARNING: All memory from FixedLengthLists must be available at query time.
        CascadeClassifier(
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
          const FixedLengthList<s32> &subsets);

        bool IsValid() const;

      protected:
        class Data
        {
        public:
          //bool read(const FileNode &node);

          bool isStumpBased;

          s32 stageType;
          s32 featureType;
          s32 ncategories;
          //Size origWinSize;
          s32 origWinHeight;
          s32 origWinWidth;

          FixedLengthList<Stage> stages;
          FixedLengthList<DTree> classifiers;
          FixedLengthList<DTreeNode> nodes;
          FixedLengthList<f32> leaves;
          FixedLengthList<s32> subsets;
        }; // class CascadeClassifier::Data

        bool isValid;

        Data data;
        //Ptr<FeatureEvaluator> featureEvaluator;
        //Ptr<CvHaarClassifierCascade> oldCascade;
      }; // class CascadeClassifier

      class CascadeClassifier_LBP : public CascadeClassifier
      {
      public:
        struct LBPFeature
        {
          LBPFeature();
          LBPFeature(const s32 left, const s32 right, const s32 top, const s32 bottom)
            : rect(left, right, top, bottom) {}

          int calc(const int _offset) const;
          void updatePtrs(const ScrollingIntegralImage_u8_s32 &sum);

          void Print() const;

          Rectangle<s32> rect; //< weight and height for block
          const s32* p[16]; //< direct pointer for fast access to integral images
        };

        // See CascadeClassifier
        CascadeClassifier_LBP();

        CascadeClassifier_LBP(
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
          MemoryStack &memory);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
        // Use OpenCV to load the XML file, and convert it to the native format
        // NOTE: You must modify opencv to add "friend class Anki::Embedded::Classifier::CascadeClassifier;" to cv::CascadeClassifier
        CascadeClassifier_LBP(const char * filename, MemoryStack &memory);

        Result SaveAsHeader(const char * filename, const char * objectName);
#endif

        Result DetectMultiScale(
          const Array<u8> &image,
          const f32 scaleFactor,
          const s32 minNeighbors,
          const s32 minObjectHeight,
          const s32 minObjectWidth,
          const s32 maxObjectHeight,
          const s32 maxObjectWidth,
          FixedLengthList<Rectangle<s32> > &objects,
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        Result DetectSingleScale(
          const Array<u8> &image,
          const s32 processingRectHeight,
          const s32 processingRectWidth,
          const s32 xyIncrement, //< Same as openCV yStep
          const f32 scaleFactor,
          FixedLengthList<Rectangle<s32> > &candidates,
          MemoryStack scratch);

        bool IsValid() const;

      protected:
        FixedLengthList<LBPFeature> features;

        s32 PredictCategoricalStump(const ScrollingIntegralImage_u8_s32 &integralImage, const Point<s16> &location, f32& sum) const;
      }; // class CascadeClassifier_LBP : public CascadeClassifier

      // This function splits the input sequence or set into one or more equivalence classes and
      // returns the vector of labels - 0-based class indexes for each element.
      // predicate(a,b) returns true if the two sequence elements certainly belong to the same class.
      //
      // The algorithm is described in "Introduction to Algorithms"
      // by Cormen, Leiserson and Rivest, the chapter "Data structures for disjoint sets"
      template<typename _Tp, class _EqPredicate> int
        Partition(const FixedLengthList<_Tp>& _vec, FixedLengthList<int>& labels, _EqPredicate predicate, MemoryStack scratch)
      {
        int i, j, N = (int)_vec.get_size();
        const _Tp* vec = &_vec[0];

        const int PARENT=0;
        const int RANK=1;

        FixedLengthList<int> _nodes(N*2, scratch);
        int (*nodes)[2] = (int(*)[2])&_nodes[0];

        // The first O(N) pass: create N single-vertex trees
        for(i = 0; i < N; i++)
        {
          nodes[i][PARENT]=-1;
          nodes[i][RANK] = 0;
        }

        // The main O(N^2) pass: merge connected components
        for( i = 0; i < N; i++ )
        {
          int root = i;

          // find root
          while( nodes[root][PARENT] >= 0 )
            root = nodes[root][PARENT];

          for( j = 0; j < N; j++ )
          {
            if( i == j || !predicate(vec[i], vec[j]))
              continue;
            int root2 = j;

            while( nodes[root2][PARENT] >= 0 )
              root2 = nodes[root2][PARENT];

            if( root2 != root )
            {
              // unite both trees
              int rank = nodes[root][RANK], rank2 = nodes[root2][RANK];
              if( rank > rank2 )
                nodes[root2][PARENT] = root;
              else
              {
                nodes[root][PARENT] = root2;
                nodes[root2][RANK] += rank == rank2;
                root = root2;
              }
              assert( nodes[root][PARENT] < 0 );

              int k = j, parent;

              // compress the path from node2 to root
              while( (parent = nodes[k][PARENT]) >= 0 )
              {
                nodes[k][PARENT] = root;
                k = parent;
              }

              // compress the path from node to root
              k = i;
              while( (parent = nodes[k][PARENT]) >= 0 )
              {
                nodes[k][PARENT] = root;
                k = parent;
              }
            }
          }
        }

        // Final O(N) pass: enumerate classes
        labels.set_size(N);
        int nclasses = 0;

        for( i = 0; i < N; i++ )
        {
          int root = i;
          while( nodes[root][PARENT] >= 0 )
            root = nodes[root][PARENT];
          // re-use the rank as the class label
          if( nodes[root][RANK] >= 0 )
            nodes[root][RANK] = ~nclasses++;
          labels[i] = ~nodes[root][RANK];
        }

        return nclasses;
      }
    } // namespace Classifier
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_
