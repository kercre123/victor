/**
File: cascadeClassifier.cpp
Author: Peter Barnum
Created: 2014-04-11

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/classifier.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/vision/robot/imageProcessing.h"

#if !defined(__EDG__)
#include <time.h>
#endif

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

#define CALC_SUM_(p0, p1, p2, p3, offset) \
  ((p0)[offset] - (p1)[offset] - (p2)[offset] + (p3)[offset])

#define ANKI_SUM_PTRS( p0, p1, p2, p3, sum, rect, stride )               \
  /* (x, y) */                                                         \
  (p0) = sum + (rect).left  + (stride) * (rect).top,                   \
  /* (x + w, y) */                                                     \
  (p1) = sum + (rect).right + (stride) * (rect).top,                   \
  /* (x, y + h) */                                                     \
  (p2) = sum + (rect).left  + (stride) * ((rect).top + (rect).height), \
  /* (x + w, y + h) */                                                 \
  (p3) = sum + (rect).right + (stride) * ((rect).top + (rect).height)

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

        this->isValid = true;
      } // CascadeClassifier::CascadeClassifier()

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
      } // bool CascadeClassifier::IsValid() const

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
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
          "#include \"anki/common/robot/config.h\"\n\n", headerDefineString, headerDefineString);

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
          "#define %s_subsets_length %d\n\n",
          objectName, this->data.stages.get_size(),
          objectName, this->data.classifiers.get_size(),
          objectName, this->data.nodes.get_size(),
          objectName, this->data.leaves.get_size(),
          objectName, this->data.subsets.get_size());

        // FixedLengthList<Stage> stages;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const Stage %s_stages_data[%s_stages_length]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.stages.get_size(); i++) {
          fprintf(file,
            "{%d,%d,%f},", this->data.stages[i].first, this->data.stages[i].ntrees, this->data.stages[i].threshold);
        }

        fprintf(file,
          "}; // %s_stages_data\n\n\n",
          objectName);

        // FixedLengthList<DTree> classifiers;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const DTree %s_classifiers_data[%s_classifiers_length]\n"
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
          "const DTreeNode %s_nodes_data[%s_nodes_length]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.nodes.get_size(); i++) {
          fprintf(file,
            "{%d,%f,%d,%d},", this->data.nodes[i].featureIdx, this->data.nodes[i].threshold, this->data.nodes[i].left, this->data.nodes[i].right);
        }

        fprintf(file,
          "}; // %s_nodes_data\n\n\n",
          objectName);

        // FixedLengthList<f32> leaves;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const f32 %s_leaves_data[%s_leaves_length]\n"
          "#if defined(__EDG__)  // ARM-MDK\n"
          "__attribute__ ((aligned (MEMORY_ALIGNMENT_RAW)))\n"
          "#endif\n"
          "= {\n",
          objectName, objectName);

        for(s32 i=0; i<this->data.leaves.get_size(); i++) {
          fprintf(file,
            "%f,", this->data.leaves[i]);
        }

        fprintf(file,
          "}; // %s_leaves_data\n\n\n",
          objectName);

        // FixedLengthList<s32> subsets;
        fprintf(file,
          "#if defined(_MSC_VER)\n"
          "__declspec(align(MEMORY_ALIGNMENT_RAW))\n"
          "#endif\n"
          "const Stage %s_subsets_data[%s_subsets_length]\n"
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

        fprintf(file,
          "#endif // %s\n", headerDefineString);

        fclose(file);

        return RESULT_OK;
      } // Result CascadeClassifier_LBP::SaveAsHeader(const char * filename, const char * objectName)
#endif

      Result CascadeClassifier_LBP::DetectMultiScale(
        const Array<u8> &image,
        const f32 scaleFactor,
        const s32 minNeighbors,
        const s32 minObjectHeight,
        const s32 minObjectWidth,
        const s32 maxObjectHeight,
        const s32 maxObjectWidth,
        FixedLengthList<Rectangle<s32> > &objects,
        MemoryStack scratch)
      {
        const s32 MAX_CANDIDATES = 5000;
        const f32 GROUP_EPS = 0.2f;

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(this->IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "CascadeClassifier::DetectMultiScale", "This object is invalid");

        objects.Clear();

        // TODO: do I need masks?

        //if (!maskGenerator.empty()) {
        //  maskGenerator->initializeMask(image);
        //}

        //if( maxObjectSize.height == 0 || maxObjectSize.width == 0 )
        //  maxObjectSize = image.size();

        //Mat grayImage = image;
        //if( grayImage.channels() > 1 )
        //{
        //  Mat temp;
        //  cvtColor(grayImage, temp, CV_BGR2GRAY);
        //  grayImage = temp;
        //}

        //Mat imageBuffer(image.rows + 1, image.cols + 1, CV_8U);
        FixedLengthList<Rectangle<s32> > candidates(MAX_CANDIDATES, scratch);

        for(f32 factor = 1; ; factor *= scaleFactor) {
          PUSH_MEMORY_STACK(scratch);

          const s32 windowHeight = RoundS32(this->data.origWinHeight*factor);
          const s32 windowWidth = RoundS32(this->data.origWinWidth*factor);

          const s32 scaledImageHeight = RoundS32(imageHeight / factor);
          const s32 scaledImageWidth = RoundS32(imageWidth / factor);

          const s32 processingRectHeight = scaledImageHeight - this->data.origWinHeight;
          const s32 processingRectWidth = scaledImageWidth - this->data.origWinWidth;

          if( processingRectWidth <= 0 || processingRectHeight <= 0 )
            break;

          if( windowWidth > maxObjectWidth || windowHeight > maxObjectHeight )
            break;

          if( windowWidth < minObjectWidth || windowHeight < minObjectHeight )
            continue;

          Array<u8> scaledImage(scaledImageHeight, scaledImageWidth, scratch, Flags::Buffer(false, false, false));

          ImageProcessing::Resize(image, scaledImage);

          const s32 xyIncrement = factor > 2. ? 1 : 2;

          int stripCount, stripSize;

          const int PTS_PER_THREAD = 1000;
          stripCount = ((processingRectWidth/xyIncrement)*(processingRectHeight + xyIncrement-1)/xyIncrement + PTS_PER_THREAD/2)/PTS_PER_THREAD;
          stripCount = std::min(std::max(stripCount, 1), 100);
          stripSize = (((processingRectHeight + stripCount - 1)/stripCount + xyIncrement-1)/xyIncrement)*xyIncrement;

          /*if( !detectSingleScale( scaledImage, stripCount, processingRectSize, stripSize, xyIncrement, factor, candidates,
          rejectLevels, levelWeights, outputRejectLevels ) )
          break;*/
        } // for(f32 factor = 1; ; factor *= scaleFactor)

        objects.set_size(candidates.get_size());
        /*std::copy(candidates.begin(), candidates.end(), objects.begin());

        groupRectangles( objects, minNeighbors, GROUP_EPS );*/

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
        //if( !featureEvaluator->setImage( image, data.origWinSize ) )
        //return false;

        //Ptr<FeatureEvaluator> evaluator = classifier->featureEvaluator->clone();

        const s32 winHeight = RoundS32(this->data.origWinHeight * scaleFactor);
        const s32 winWidth = RoundS32(this->data.origWinWidth * scaleFactor);

        for(s32 y = 0; y < processingRectHeight; y += xyIncrement ) {
          for(s32 x = 0; x < processingRectWidth; x += xyIncrement ) {
            double gypWeight;
            //int result = classifier->runAt(evaluator, Point(x, y), gypWeight);
            int result = 0;

            if( result > 0 ) {
              const s32 x0 = RoundS32(x*scaleFactor);
              const s32 y0 = RoundS32(y*scaleFactor);

              // TODO: should be width/height minus one?
              candidates.PushBack(Rectangle<s32>(
                x0, x0 + winWidth,
                y0, y0 + winHeight));
            }

            // TODO: should the xStep be twice the xyIncrement?
            if(result == 0)
              x += xyIncrement;
          }
        }

        return RESULT_OK;
      }

      s32 CascadeClassifier_LBP::PredictCategoricalStump(f32& sum) const
      {
        int nstages = (int)this->data.stages.get_size();
        int nodeOfs = 0, leafOfs = 0;
        //FEval& featureEvaluator = (FEval&)*_featureEvaluator;
        size_t subsetSize = (this->data.ncategories + 31)/32;
        const int* cascadeSubsets = &this->data.subsets[0];
        const float* cascadeLeaves = &this->data.leaves[0];
        const CascadeClassifier::DTreeNode* cascadeNodes = &this->data.nodes[0];
        const CascadeClassifier::Stage* cascadeStages = &this->data.stages[0];

        for( int si = 0; si < nstages; si++ )
        {
          const CascadeClassifier::Stage& stage = cascadeStages[si];
          int wi, ntrees = stage.ntrees;

          sum = 0;

          for( wi = 0; wi < ntrees; wi++ )
          {
            const CascadeClassifier::DTreeNode& node = cascadeNodes[nodeOfs];

            //int c = featureEvaluator();
            //        int c = featuresPtr[node.featureIdx].calc(offset);
            //            int cval = CALC_SUM_( p[5], p[6], p[9], p[10], _offset );

            //return (CALC_SUM_( p[0], p[1], p[4], p[5], _offset ) >= cval ? 128 : 0) |   // 0
            //       (CALC_SUM_( p[1], p[2], p[5], p[6], _offset ) >= cval ? 64 : 0) |    // 1
            //       (CALC_SUM_( p[2], p[3], p[6], p[7], _offset ) >= cval ? 32 : 0) |    // 2
            //       (CALC_SUM_( p[6], p[7], p[10], p[11], _offset ) >= cval ? 16 : 0) |  // 5
            //       (CALC_SUM_( p[10], p[11], p[14], p[15], _offset ) >= cval ? 8 : 0)|  // 8
            //       (CALC_SUM_( p[9], p[10], p[13], p[14], _offset ) >= cval ? 4 : 0)|   // 7
            //       (CALC_SUM_( p[8], p[9], p[12], p[13], _offset ) >= cval ? 2 : 0)|    // 6
            //       (CALC_SUM_( p[4], p[5], p[8], p[9], _offset ) >= cval ? 1 : 0);

            int c = 0;

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
    } // namespace Classifier
  } // namespace Embedded
} // namespace Anki
