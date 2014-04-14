/**
File: cascadeClassifier.cpp
Author: Peter Barnum
Created: 2014-04-11

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/classifier.h"
#include "anki/common/robot/matlabInterface.h"

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
      } // CascadeClassifier::CascadeClassifier(const char * filename, MemoryStack &memory)

      Result CascadeClassifier::SaveAsHeader(const char * filename, const char * objectName)
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
      } // Result CascadeClassifier::SaveAsHeader(const char * filename, const char * objectName)
#endif
    } // namespace Classifier
  } // namespace Embedded
} // namespace Anki
