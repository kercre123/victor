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
std::vector<GrayvalueBool> LoadIntoList_grayvalueBool(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2);
template<typename Type> FixedLengthList<Type> LoadIntoList_permanentBuffer(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch, MemoryStack &memory);

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

std::vector<GrayvalueBool> LoadIntoList_grayvalueBool(const char * filenamePrefix, const char * filenameSuffix, MemoryStack scratch1, MemoryStack scratch2)
{
  const s32 filenameBufferLength = 1024;
  char filenameBuffer[filenameBufferLength];

  std::vector<GrayvalueBool> out;

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

void PrintUsage()
{
  printf(
    "Usage: run_trainDecisionTree <filenamePrefix> <numProbes> <leafNodeFraction> <leafNodeNumItems> <minGrayvalueDistance>\n"
    "Example: run_trainDecisionTree c:/tmp/treeTraining_ 900 1.0 1 20");
}

int main(int argc, const char* argv[])
{
  if(argc != 6) {
    PrintUsage();
  }

  const char * filenamePrefix = argv[1];
  const s32 numProbes = atol(argv[2]);
  const f32 leafNodeFraction = static_cast<f32>(atof(argv[3]));
  const s32 leafNodeNumItems = atol(argv[4]);
  const s32 minGrayvalueDistance = atol(argv[5]);

  const s32 bufferSize = 50000000;
  MemoryStack memory(malloc(bufferSize), bufferSize);

  std::vector<GrayvalueBool> probesUsed;
  FixedLengthList<const char *> labelNames;
  FixedLengthList<s32> labels;
  FixedLengthList<FixedLengthList<u8> > probeValues(numProbes, memory, Flags::Buffer(true, false, true));
  FixedLengthList<f32> probeLocationsXGrid;
  FixedLengthList<f32> probeLocationsYGrid;
  FixedLengthList<u8> grayvalueThresholdsToUse;

  AnkiConditionalErrorAndReturnValue(AreValid(probeValues),
    -5, "run_trainDecisionTree", "Invalid input");

  // Load all inputs
  {
    MemoryStack scratch1(malloc(bufferSize), bufferSize);
    MemoryStack scratch2(malloc(bufferSize), bufferSize);

    probesUsed = LoadIntoList_grayvalueBool(filenamePrefix, "probesUsed.array", scratch1, scratch2);
    labelNames = LoadIntoList_permanentBuffer<const char *>(filenamePrefix, "labelNames.array", scratch1, memory);
    labels = LoadIntoList_temporaryBuffer<s32>(filenamePrefix, "labels.array", scratch1, scratch2, memory);
    probeLocationsXGrid = LoadIntoList_temporaryBuffer<f32>(filenamePrefix, "probeLocationsXGrid.array", scratch1, scratch2, memory);
    probeLocationsYGrid = LoadIntoList_temporaryBuffer<f32>(filenamePrefix, "probeLocationsYGrid.array", scratch1, scratch2, memory);
    grayvalueThresholdsToUse = LoadIntoList_temporaryBuffer<u8>(filenamePrefix, "grayvalueThresholdsToUse.array", scratch1, scratch2, memory);

    for(s32 iProbe=0; iProbe<numProbes; iProbe++) {
      const s32 filenameBufferLength = 1024;
      char filenameBuffer[filenameBufferLength];
      snprintf(filenameBuffer, filenameBufferLength, "probeValues%d.array", iProbe);
      probeValues[iProbe] = LoadIntoList_temporaryBuffer<u8>(filenamePrefix, filenameBuffer, scratch1, scratch2, memory);
    }

    free(scratch1.get_buffer());
    free(scratch2.get_buffer());
  } // Load all inputs

  AnkiConditionalErrorAndReturnValue(
    AreValid(labelNames, labels, probeLocationsXGrid, probeLocationsYGrid, grayvalueThresholdsToUse)  &&
    probesUsed.size() == numProbes &&
    probeValues.get_size() == numProbes &&
    probeLocationsXGrid.get_size() == numProbes &&
    probeLocationsYGrid.get_size() == numProbes,
    -1, "run_trainDecisionTree", "Invalid input");

  for(s32 i=0; i<numProbes; i++) {
    AnkiConditionalErrorAndReturnValue(labels.get_size() == probeValues[i].get_size(),
      -2, "run_trainDecisionTree", "Invalid input");
  }

  // Add a const qualifier
  FixedLengthList<const FixedLengthList<u8> > probeValuesConst = *reinterpret_cast<FixedLengthList<const FixedLengthList<u8> >* >(&probeValues);

  std::vector<DecisionTreeNode> decisionTree;
  const Result result = BuildTree(
    probesUsed,
    labelNames, labels,
    probeValuesConst,
    probeLocationsXGrid, probeLocationsYGrid,
    leafNodeFraction, leafNodeNumItems, minGrayvalueDistance,
    grayvalueThresholdsToUse,
    decisionTree);

  free(memory.get_buffer());

  return 0;
}
