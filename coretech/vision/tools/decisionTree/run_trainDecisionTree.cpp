/**
File: run_trainDecisionTree.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "trainDecisionTree.h"

#include "anki/common/robot/serialize.h"

#define COMPRESSION_LEVEL 9
#define MAX_TREE_NODES 10000000

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> FixedLengthList<Type> LoadIntoList_temporaryBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2, void * allocatedBuffer, const s32 allocatedBufferLength);
std::vector<U8Bool> LoadIntoList_grayvalueBool(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2);
template<typename Type> FixedLengthList<Type> LoadIntoList_permanentBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch, MemoryStack &memory);
template<typename Type> Result SaveList(const FixedLengthList<Type> &in, const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch);

template<typename Type> FixedLengthList<Type> LoadIntoList_temporaryBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2, void * allocatedBuffer, const s32 allocatedBufferLength)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  snprintf(filenameBuffer, filenameBufferLength, "%s%s", filenamePrefix, filenameSuffix);

  Array<Type> raw = Array<Type>::LoadBinary(filenameBuffer, allocatedBuffer, allocatedBufferLength);

  AnkiConditionalErrorAndReturnValue(raw.get_size(0) == 1,
    FixedLengthList<Type>(), "LoadIntoList_permanentBuffer", "Input is the wrong size");

  const s32 numElements = raw.get_size(0) * raw.get_size(1);

  FixedLengthList<Type> out = FixedLengthList<Type>(numElements, raw.Pointer(0,0), allocatedBufferLength, Flags::Buffer(true, false, true));

  AnkiConditionalErrorAndReturnValue(AreValid(raw, out),
    FixedLengthList<Type>(), "LoadIntoList_permanentBuffer", "Could not allocate");

  //memcpy(out.Pointer(0), raw.Pointer(0,0), numElements*sizeof(Type));

  /*Type * restrict pOut = out.Pointer(0);
  const Type * restrict pRaw = raw.Pointer(0,0);

  for(s32 i=0; i<numElements; i++) {
  pOut[i] = pRaw[i];
  }*/

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

  return arr.get_array().SaveBinary(filenameBuffer, COMPRESSION_LEVEL, scratch);
}

void PrintUsage()
{
  CoreTechPrint(
    "Usage: run_trainDecisionTree <inFilenamePrefix> <numFeatures> <leafNodeFraction> <leafNodeNumItems> <u8MinDistanceForSplits> <u8MinDistanceFromThreshold> <maxThreads> <outFilenamePrefix>\n"
    "Example: run_trainDecisionTree c:/tmp/treeTraining_ 900 1.0 1 20 40 8 c:/tmp/treeTrainingOut_");
}

int main(int argc, const char* argv[])
{
  f64 time0 = GetTimeF64();

  const f64 benchmarkSampleEveryNSeconds = 5.0;

  if(argc != 9) {
    PrintUsage();
    return -10;
  }

  const char * inFilenamePrefix = argv[1];
  const s32 numFeatures = atol(argv[2]);
  const f32 leafNodeFraction = static_cast<f32>(atof(argv[3]));
  const s32 leafNodeNumItems = atol(argv[4]);
  const s32 u8MinDistanceForSplits = atol(argv[5]);
  const s32 u8MinDistanceFromThreshold = atol(argv[6]);
  const s32 maxThreads = atol(argv[7]);
  const char * outFilenamePrefix = argv[8];

#ifdef _MSC_VER
  // 32-bit
  const s32 memorySize = s32(1e8);
  const s32 scratchSize = s32(1e8) / 2;
#else
  // 64-bit
  const s32 memorySize = s32(1e9);
  const s32 scratchSize = s32(1e9);
#endif

  MemoryStack memory(malloc(memorySize), memorySize);

  AnkiConditionalErrorAndReturnValue(
    AreValid(memory),
    -1, "run_trainDecisionTree", "Out of memory");

  std::vector<U8Bool> featuresUsed;
  FixedLengthList<const char *> labelNames;
  FixedLengthList<s32> labels;
  FixedLengthList<FixedLengthList<u8> > featureValues(numFeatures, memory, Flags::Buffer(true, false, true));
  FixedLengthList<u8> u8ThresholdsToUse;

  AnkiConditionalErrorAndReturnValue(AreValid(featureValues),
    -5, "run_trainDecisionTree", "Invalid input");

  std::vector<void*> bufferPointers;

  // Load all inputs
  {
    MemoryStack scratch1(malloc(scratchSize), scratchSize);
    MemoryStack scratch2(malloc(scratchSize), scratchSize);

    AnkiConditionalErrorAndReturnValue(
      AreValid(scratch1, scratch2),
      -1, "run_trainDecisionTree", "Out of memory");

    CoreTechPrint("Loading Inputs...\n");

    // Load this first, just to figure out the right buffer size
    s32 numImages;
    {
#ifdef _MSC_VER
      // 32-bit
      const s32 tmpBigBufferLength = s32(1e8);

#else
      // 64-bit
      const s32 tmpBigBufferLength = s32(1e9);
#endif

      void * tmpBigBuffer = calloc(tmpBigBufferLength + 2*MEMORY_ALIGNMENT, 1);

      AnkiAssert(tmpBigBuffer);

      labels = LoadIntoList_temporaryBuffer<s32>(inFilenamePrefix, "labels.array", scratch1, scratch2, tmpBigBuffer, tmpBigBufferLength);
      numImages = labels.get_size();
      free(tmpBigBuffer);
    }

    AnkiAssert(numImages > 0);
    //AnkiAssert((numImages * numFeatures) < 1e10);

    featuresUsed = LoadIntoList_grayvalueBool(inFilenamePrefix, "featuresUsed.array", scratch1, scratch2);

    labelNames = LoadIntoList_permanentBuffer<const char *>(inFilenamePrefix, "labelNames.array", scratch1, memory);

    const s32 labelsBufferSize = numImages*sizeof(s32) + s32(1e4) + 2*MEMORY_ALIGNMENT;
    bufferPointers.push_back(calloc(labelsBufferSize, 1));
    labels = LoadIntoList_temporaryBuffer<s32>(inFilenamePrefix, "labels.array", scratch1, scratch2, bufferPointers[bufferPointers.size()-1], labelsBufferSize);

    const s32 u8ThresholdsToUseBufferSize = s32(1e6);
    bufferPointers.push_back(calloc(u8ThresholdsToUseBufferSize, 1));
    u8ThresholdsToUse = LoadIntoList_temporaryBuffer<u8>(inFilenamePrefix, "u8ThresholdsToUse.array", scratch1, scratch2, bufferPointers[bufferPointers.size()-1], u8ThresholdsToUseBufferSize);

    f64 t0 = GetTimeF64();
    const s32 featureValuesBufferSize = numImages + s32(1e4) + 2*MEMORY_ALIGNMENT;
    for(s32 iFeature=0; iFeature<numFeatures; iFeature++) {
      bufferPointers.push_back(calloc(featureValuesBufferSize, 1));

      const s32 filenameBufferLength = 1024;
      char filenameBuffer[filenameBufferLength];
      snprintf(filenameBuffer, filenameBufferLength, "featureValues%d.array", iFeature);
      featureValues[iFeature] = LoadIntoList_temporaryBuffer<u8>(inFilenamePrefix, filenameBuffer, scratch1, scratch2, bufferPointers[bufferPointers.size()-1], featureValuesBufferSize);

      if(iFeature > 0 && iFeature % 200 == 0) {
        f64 t1 = GetTimeF64();
        CoreTechPrint("Loaded %d/%d in %f seconds\n", iFeature, numFeatures, t1-t0);
        t0 = t1;
      }
    }

    free(scratch1.get_buffer());
    free(scratch2.get_buffer());

    CoreTechPrint("Done loading\n");
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

  ThreadSafeFixedLengthList<DecisionTreeNode> decisionTree(MAX_TREE_NODES, memory);
  std::vector<f32> cpuUsage;

  AnkiConditionalErrorAndReturnValue(
    AreValid(decisionTree),
    -1, "run_trainDecisionTree", "Out of memory");

  const Result result = BuildTree(
    featuresUsed,
    labelNames, labels,
    featureValuesConst,
    leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold,
    u8ThresholdsToUse,
    maxThreads,
    benchmarkSampleEveryNSeconds,
    cpuUsage,
    decisionTree);

  // result could fail, but let's try to save anyway
  if(result != RESULT_OK) {
    CoreTechPrint("BuildTree failed, but trying to save anyway...\n");
  }

  for(u32 i=0; i<bufferPointers.size(); i++) {
    free(bufferPointers[i]);
  }

  // Save the output
  {
    const FixedLengthList<DecisionTreeNode> unsafeDecisionTree = decisionTree.get_buffer();
    const s32 numNodes = unsafeDecisionTree.get_size();
    const s32 saveBufferSize = 10000000 + 3 * numNodes * sizeof(DecisionTreeNode);

    MemoryStack scratch(malloc(saveBufferSize), saveBufferSize);

    AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
      -7, "run_trainDecisionTree", "Out of memory for saving");

    FixedLengthList<s32> depths(numNodes, scratch, Flags::Buffer(true, false, true));
    FixedLengthList<f32> bestEntropys(numNodes, scratch, Flags::Buffer(true, false, true));
    FixedLengthList<s32> whichFeatures(numNodes, scratch, Flags::Buffer(true, false, true));
    FixedLengthList<u8>  u8Thresholds(numNodes, scratch, Flags::Buffer(true, false, true));
    FixedLengthList<s32> leftChildIndexs(numNodes, scratch, Flags::Buffer(true, false, true));
    FixedLengthList<f32> cpuUsageSamples(cpuUsage.size(), scratch, Flags::Buffer(true, false, true));

    AnkiConditionalErrorAndReturnValue(AreValid(depths, bestEntropys, whichFeatures, u8Thresholds, leftChildIndexs, cpuUsageSamples),
      -7, "run_trainDecisionTree", "Out of memory for saving");

    for(s32 iNode=0; iNode<numNodes; iNode++) {
      const DecisionTreeNode &curNode = unsafeDecisionTree[iNode];

      depths[iNode] = curNode.depth;
      bestEntropys[iNode] = curNode.bestEntropy;
      whichFeatures[iNode] = curNode.whichFeature;
      u8Thresholds[iNode] = curNode.u8Threshold;
      leftChildIndexs[iNode] = curNode.leftChildIndex;
    } // for(s32 iNode=0; iNode<numNodes; iNode++)

    SaveList(depths, outFilenamePrefix, "depths.array", scratch);
    SaveList(bestEntropys, outFilenamePrefix, "bestEntropys.array", scratch);
    SaveList(whichFeatures, outFilenamePrefix, "whichFeatures.array", scratch);
    SaveList(u8Thresholds, outFilenamePrefix, "u8Thresholds.array", scratch);
    SaveList(leftChildIndexs, outFilenamePrefix, "leftChildIndexs.array", scratch);

    for(u32 iSample=0; iSample<cpuUsage.size(); iSample++) {
      cpuUsageSamples[iSample] = cpuUsage[iSample];
    } // for(s32 iNode=0; iNode<numNodes; iNode++)

    SaveList(cpuUsageSamples, outFilenamePrefix, "cpuUsageSamples.array", scratch);

    free(scratch.get_buffer());
  } // Save the output

  free(memory.get_buffer());

  f64 time1 = GetTimeF64();

  CoreTechPrint("Tree training took %f seconds. Tree is %d nodes.\n", time1-time0, static_cast<s32>(decisionTree.get_buffer().get_size()));

  return 0;
}
