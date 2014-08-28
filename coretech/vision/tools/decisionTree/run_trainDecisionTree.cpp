/**
File: run_trainDecisionTree.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "trainDecisionTree.h"

#include "anki/common/robot/serialize.h"

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> FixedLengthList<Type> LoadIntoList_temporaryBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2, MemoryStack &memory);
std::vector<U8Bool> LoadIntoList_grayvalueBool(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2);
template<typename Type> FixedLengthList<Type> LoadIntoList_permanentBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch, MemoryStack &memory);
template<typename Type> Result SaveList(const FixedLengthList<Type> &in, const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch);

template<typename Type> FixedLengthList<Type> LoadIntoList_temporaryBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2, MemoryStack &memory)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  snprintf(filenameBuffer, filenameBufferLength, "%s%s", filenamePrefix, filenameSuffix);

  Array<Type> raw = Array<Type>::LoadBinary(filenameBuffer, scratch1, scratch2);

  const s32 numElements = raw.get_size(0) * raw.get_size(1);

  FixedLengthList<Type> out = FixedLengthList<Type>(numElements, memory, Flags::Buffer(true, false, true));

  AnkiConditionalErrorAndReturnValue(AreValid(raw, out),
    FixedLengthList<Type>(), "LoadIntoList_permanentBuffer", "Could not allocate");

  for(s32 i=0; i<numElements; i++) {
    out[i] = raw.Element(i);
  }

  return out;
}

std::vector<U8Bool> LoadIntoList_grayvalueBool(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  std::vector<U8Bool> out;

  snprintf(filenameBuffer, filenameBufferLength, "%s%s", filenamePrefix, filenameSuffix);

  Array<u8> raw = Array<u8>::LoadBinary(filenameBuffer, scratch1, scratch2);

  const s32 rawHeight = raw.get_size(0);
  const s32 rawWidth = raw.get_size(1);

  AnkiConditionalErrorAndReturnValue(raw.IsValid(),
    out, "LoadIntoList_grayvalueBool", "Could not allocate");

  AnkiAssert(rawWidth == 256);

  out.resize(raw.get_size(0));

  for(s32 i=0; i<rawHeight; i++) {
    for(s32 j=0; j<256; j++) {
      out[i].values[j] = *raw.Pointer(i, j);
    }
  }

  return out;
}

template<typename Type> FixedLengthList<Type> LoadIntoList_permanentBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch, MemoryStack &memory)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  snprintf(filenameBuffer, filenameBufferLength, "%s%s", filenamePrefix, filenameSuffix);

  Array<Type> raw = Array<Type>::LoadBinary(filenameBuffer, scratch, memory);

  const s32 numElements = raw.get_size(0) * raw.get_size(1);

  FixedLengthList<Type> out = FixedLengthList<Type>(numElements, memory, Flags::Buffer(true, false, true));

  AnkiConditionalErrorAndReturnValue(AreValid(raw, out),
    FixedLengthList<Type>(), "LoadIntoList_permanentBuffer", "Could not allocate");

  for(s32 i=0; i<numElements; i++) {
    out[i] = raw.Element(i);
  }

  return out;
}

template<typename Type> Result SaveList(const FixedLengthList<Type> &in, const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  snprintf(filenameBuffer, filenameBufferLength, "%s%s", filenamePrefix, filenameSuffix);

  const ConstArraySlice<Type> arr = in;

  return arr.get_array().SaveBinary(filenameBuffer, scratch);
}

void PrintUsage()
{
  printf(
    "Usage: run_trainDecisionTree <filenamePrefix> <numFeatures> <leafNodeFraction> <leafNodeNumItems> <u8MinDistance> <maxPrimaryThreads> <maxSecondaryThreads>\n"
    "Example: run_trainDecisionTree c:/tmp/treeTraining_ 900 1.0 1 20 8 8");
}

int main(int argc, const char* argv[])
{
  if(argc != 8) {
    PrintUsage();
    return -10;
  }

  const char * filenamePrefix = argv[1];
  const s32 numFeatures = atol(argv[2]);
  const f32 leafNodeFraction = static_cast<f32>(atof(argv[3]));
  const s32 leafNodeNumItems = atol(argv[4]);
  const s32 u8MinDistance = atol(argv[5]);
  const s32 maxPrimaryThreads = atol(argv[6]);
  const s32 maxSecondaryThreads = atol(argv[7]);

  const s32 bufferSize = 50000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);

  std::vector<U8Bool> featuresUsed;
  FixedLengthList<const char *> labelNames;
  FixedLengthList<s32> labels;
  FixedLengthList<FixedLengthList<u8> > featureValues(numFeatures, memory, Flags::Buffer(true, false, true));
  FixedLengthList<u8> u8ThresholdsToUse;

  AnkiConditionalErrorAndReturnValue(AreValid(featureValues),
    -5, "run_trainDecisionTree", "Invalid input");

  // Load all inputs
  {
    MemoryStack scratch1(malloc(bufferSize), bufferSize);
    MemoryStack scratch2(malloc(bufferSize), bufferSize);

    featuresUsed = LoadIntoList_grayvalueBool(filenamePrefix, "featuresUsed.array", scratch1, scratch2);
    labelNames = LoadIntoList_permanentBuffer<const char *>(filenamePrefix, "labelNames.array", scratch1, memory);
    labels = LoadIntoList_temporaryBuffer<s32>(filenamePrefix, "labels.array", scratch1, scratch2, memory);
    u8ThresholdsToUse = LoadIntoList_temporaryBuffer<u8>(filenamePrefix, "u8ThresholdsToUse.array", scratch1, scratch2, memory);

    for(s32 iFeature=0; iFeature<numFeatures; iFeature++) {
      const s32 filenameBufferLength = 1024;
      char filenameBuffer[filenameBufferLength];
      snprintf(filenameBuffer, filenameBufferLength, "featureValues%d.array", iFeature);
      featureValues[iFeature] = LoadIntoList_temporaryBuffer<u8>(filenamePrefix, filenameBuffer, scratch1, scratch2, memory);
    }

    free(scratch1.get_buffer());
    free(scratch2.get_buffer());
  } // Load all inputs

  AnkiConditionalErrorAndReturnValue(
    AreValid(labelNames, labels, u8ThresholdsToUse)  &&
    featuresUsed.size() == numFeatures &&
    featureValues.get_size() == numFeatures,
    -1, "run_trainDecisionTree", "Invalid input");

  for(s32 i=0; i<numFeatures; i++) {
    AnkiConditionalErrorAndReturnValue(labels.get_size() == featureValues[i].get_size(),
      -2, "run_trainDecisionTree", "Invalid input");
  }

  // Add a const qualifier
  FixedLengthList<const FixedLengthList<u8> > featureValuesConst = *reinterpret_cast<FixedLengthList<const FixedLengthList<u8> >* >(&featureValues);

  std::vector<DecisionTreeNode> decisionTree;
  BuildTree(
    featuresUsed,
    labelNames, labels,
    featureValuesConst,
    leafNodeFraction, leafNodeNumItems, u8MinDistance,
    u8ThresholdsToUse,
    maxPrimaryThreads, maxSecondaryThreads,
    decisionTree);

  // We're done with this memory once BuildTree returns
  free(memory.get_buffer());

  // Save the output
  {
    const s32 numNodes = decisionTree.size();
    const s32 saveBufferSize = 10000000 + 3 * numNodes * sizeof(DecisionTreeNode);

    MemoryStack scratch(malloc(saveBufferSize), saveBufferSize);

    FixedLengthList<s32> depths(numNodes, scratch);
    FixedLengthList<f32> bestEntropys(numNodes, scratch);
    FixedLengthList<s32> whichFeatures(numNodes, scratch);
    FixedLengthList<u8>  u8Thresholds(numNodes, scratch);
    FixedLengthList<s32> leftChildIndexs(numNodes, scratch);

    AnkiConditionalErrorAndReturnValue(AreValid(depths, bestEntropys, whichFeatures, u8Thresholds, leftChildIndexs),
      -7, "run_trainDecisionTree", "Out of memory for saving");

    for(s32 iNode=0; iNode<numNodes; iNode++) {
      const DecisionTreeNode &curNode = decisionTree[iNode];

      depths[iNode] = curNode.depth;
      bestEntropys[iNode] = curNode.bestEntropy;
      whichFeatures[iNode] = curNode.whichFeature;
      u8Thresholds[iNode] = curNode.u8Threshold;
      leftChildIndexs[iNode] = curNode.leftChildIndex;
    } // for(s32 iNode=0; iNode<numNodes; iNode++)

    SaveList(depths, filenamePrefix, "out_depths.array", scratch);
    SaveList(bestEntropys, filenamePrefix, "out_bestEntropys.array", scratch);
    SaveList(whichFeatures, filenamePrefix, "out_whichFeatures.array", scratch);
    SaveList(u8Thresholds, filenamePrefix, "out_u8Thresholds.array", scratch);
    SaveList(leftChildIndexs, filenamePrefix, "out_leftChildIndexs.array", scratch);

    free(scratch.get_buffer());
  } // Save the output

  return 0;
}
