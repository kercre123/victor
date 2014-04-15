/**
File: classifier.h
Author: Peter Barnum
Created: 2014-04-11

Classify a query with a classifier. For example, detect faces in an image, using a cascade classifier.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_
#define _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/integralImage.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Classifier
    {
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
          void updatePtrs(const IntegralImage_u8_s32 &sum);

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
          const FixedLengthList<LBPFeature> &features);

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
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
          MemoryStack scratch);

        Result DetectSingleScale(
          const Array<u8> &image,
          const s32 processingRectHeight,
          const s32 processingRectWidth,
          const s32 xyIncrement, //< Same as openCV yStep
          const f32 scaleFactor,
          FixedLengthList<Rectangle<s32> > &candidates,
          MemoryStack scratch);

      protected:
        FixedLengthList<LBPFeature> features;

        s32 PredictCategoricalStump(const IntegralImage_u8_s32 &integralImage, const Point<s16> &location, f32& sum) const;
      }; // class CascadeClassifier_LBP : public CascadeClassifier
    } // namespace Classifier
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_CLASSIFIER_H_
