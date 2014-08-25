/**
File: computeInfoGain.cpp
Author: Peter Barnum
Created: 2014-08-22

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#define PRECISION f64

#include "trainDecisionTree.h"

#include <vector>
#include <queue>

using namespace std;
using namespace Anki;
using namespace Anki::Embedded;

// Visual c++ is missing log2
#if defined(_MSC_VER)
const f32 log2DivisorF32 = 1.0f / log(2.0f);
static inline f32 log2(const f32 x)
{
  return log(x) * log2DivisorF32;
}

const f64 log2DivisorF64 = 1.0 / log(2.0);
static inline f64 log2(const f64 x)
{
  return log(x) * log2DivisorF64;
}
#endif

typedef struct {
  s32 counts[256];
} GrayvalueCounts;

static void CountValues_u8(const u8 * restrict pProbeValues, const vector<s32> &remaining, GrayvalueCounts &counts)
{
  memset(&counts.counts[0], 0, 256*sizeof(s32));

  const s32 numRemaining = remaining.size();
  const s32 * restrict pRemaining = remaining.data();

  for(s32 i=0; i<numRemaining; i++) {
    counts.counts[pProbeValues[pRemaining[i]]]++;
  }
} // CountUnique_u8()

static void ComputeNumAboveThreshold(
  const u8 * restrict pProbeValues,
  const s32 * restrict pLabels,
  const s32 maxLabel,
  const s32 * restrict pRemaining,
  const s32 numRemaining,
  const u8 curGrayvalueThreshold,
  s32 * restrict pNumLessThan,
  s32 * restrict pNumGreaterThan,
  s32 &totalNumLessThan,
  s32 &totalNumGreaterThan)
{
  memset(pNumLessThan, 0, (maxLabel+1)*sizeof(s32));
  memset(pNumGreaterThan, 0, (maxLabel+1)*sizeof(s32));

  totalNumLessThan = 0;
  totalNumGreaterThan = 0;

  for(s32 iRemain=0; iRemain<numRemaining; iRemain++) {
    const s32 iImage = pRemaining[iRemain];

    const s32 curLabel = pLabels[iImage];
    const u8 curProbeValue = pProbeValues[iImage];

    if(curProbeValue < curGrayvalueThreshold) {
      totalNumLessThan++;
      pNumLessThan[curLabel]++;
    } else {
      totalNumGreaterThan++;
      pNumGreaterThan[curLabel]++;
    }
  }
}

// Type should be f32 or f64
template<typename Type> static Type WeightedAverageEntropy(
  const s32 * restrict pNumLessThan,
  const s32 * restrict pNumGreaterThan,
  const s32 totalNumLessThan,
  const s32 totalNumGreaterThan,
  const s32 maxLabel)
{
  Type entropyLessThan = 0;
  Type entropyGreaterThan = 0;

  const Type inverseTotalNumLessThan = static_cast<Type>(1) / static_cast<Type>(totalNumLessThan);
  const Type inverseTotalNumGreaterThan = static_cast<Type>(1) / static_cast<Type>(totalNumGreaterThan);

  for(s32 iLabel=0; iLabel<=maxLabel; iLabel++) {
    //probabilitiesLessThan = allValuesLessThan / sum(allValuesLessThan);
    //entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
    if(pNumLessThan[iLabel] > 0) {
      const Type probability = static_cast<Type>(pNumLessThan[iLabel]) * inverseTotalNumLessThan;
      entropyLessThan -= probability * log2(probability);
    }

    //probabilitiesGreaterThan = allValuesGreaterThan / sum(allValuesGreaterThan);
    //entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
    if(pNumGreaterThan[iLabel] > 0) {
      const Type probability = static_cast<Type>(pNumGreaterThan[iLabel]) * inverseTotalNumGreaterThan;
      entropyGreaterThan -= probability * log2(probability);
    }
  } // for(s32 iLabel=0; iLabel<=maxLabel; iLabel++)

  const Type percent_lessThan    = static_cast<Type>(totalNumLessThan)    / static_cast<Type>(totalNumLessThan + totalNumGreaterThan);
  const Type percent_greaterThan = static_cast<Type>(totalNumGreaterThan) / static_cast<Type>(totalNumLessThan + totalNumGreaterThan);

  const Type weightedAverageEntropy = percent_lessThan * entropyLessThan + percent_greaterThan * entropyGreaterThan;

  return weightedAverageEntropy;
}

namespace Anki
{
  namespace Embedded
  {
    ThreadResult ComputeInfoGain(void *computeInfoGainParameters)
    {
      const f64 time0 = GetTimeF64();

      ComputeInfoGainParameters * restrict parameters = reinterpret_cast<ComputeInfoGainParameters*>(computeInfoGainParameters);

      parameters->bestEntropy = FLT_MAX;
      parameters->bestProbeIndex = -1;
      parameters->bestGrayvalueThreshold = -1;

      u8 grayvalueThresholds[256];
      s32 numGrayvalueThresholds;

      // If grayvalueThresholdsToUse has been passed in, don't compute the grayvalueThresholds
      if(parameters->grayvalueThresholdsToUse.get_size() != 0) {
        numGrayvalueThresholds = parameters->grayvalueThresholdsToUse.get_size();
        for(s32 i=0; i<numGrayvalueThresholds; i++) {
          grayvalueThresholds[i] = parameters->grayvalueThresholdsToUse[i];
        }
      }

      const s32 numImages = parameters->probeValues[0].get_size();
      const s32 numRemaining = parameters->remaining.size();
      const s32 numProbesLocationsToCheck = parameters->probesLocationsToCheck.size();

      const s32 maxLabel = FindMaxLabel(parameters->labels, parameters->remaining);

      const s32 * restrict pLabels = parameters->labels.Pointer(0);
      const s32 * restrict pRemaining = parameters->remaining.data();
      const s32 * restrict pProbesLocationsToCheck = parameters->probesLocationsToCheck.data();

      //
      // For each probe location and grayvalue threshold, find the best entropy
      //

      for(s32 iProbeToCheck=0; iProbeToCheck<numProbesLocationsToCheck; iProbeToCheck++) {
        const s32 iProbe = pProbesLocationsToCheck[iProbeToCheck];
        const u8 * restrict pProbeValues = parameters->probeValues[iProbe].Pointer(0);

        // If the grayvalueThresholdsToUse haven't been specified, compute them from the data
        if(parameters->grayvalueThresholdsToUse.get_size() == 0) {
          // What are the unique grayvalues for this probe?
          GrayvalueCounts counts;
          CountValues_u8(pProbeValues, parameters->remaining, counts);

          // Compute the grayvalue thresholds (just halfway between each pair of detected grayvalues
          numGrayvalueThresholds = 0;
          s32 previousValue = -1;
          for(s32 i=0; i<256; i++) {
            if(counts.counts[i] > 0) {
              if(previousValue > 0) {
                grayvalueThresholds[numGrayvalueThresholds] = (i + previousValue) / 2;
                numGrayvalueThresholds++;
                previousValue = i;
              } else {
                previousValue = i;
              }
            }
          }
        } // if(parameters->grayvalueThresholdsToUse.empty())

        GrayvalueBool &pProbesUsed = parameters->probesUsed[iProbe];

        for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++) {
          const u8 curGrayvalueThreshold = grayvalueThresholds[iGrayvalueThreshold];

          if(pProbesUsed.values[curGrayvalueThreshold]) {
            continue;
          }

          s32 totalNumLessThan = 0;
          s32 totalNumGreaterThan = 0;

          ComputeNumAboveThreshold(
            pProbeValues,
            pLabels, maxLabel,
            pRemaining, numRemaining,
            curGrayvalueThreshold,
            parameters->pNumLessThan, parameters->pNumGreaterThan,
            totalNumLessThan, totalNumGreaterThan);

          if(totalNumLessThan == 0 || totalNumGreaterThan == 0) {
            pProbesUsed.values[curGrayvalueThreshold] = true;
            continue;
          }

          const PRECISION entropy = WeightedAverageEntropy<PRECISION>(
            parameters->pNumLessThan, parameters->pNumGreaterThan,
            totalNumLessThan, totalNumGreaterThan,
            maxLabel);

          if(entropy < parameters->bestEntropy) {
            parameters->bestEntropy = static_cast<f32>(entropy);
            parameters->bestProbeIndex = iProbe;
            parameters->bestGrayvalueThreshold = curGrayvalueThreshold;
          }
        } // for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++)
      } // for(s32 iProbeToCheck=0; iProbeToCheck<numProbesLocationsToCheck; iProbeToCheck++)

      // Mask out the grayvalues that are near to the chosen threshold
      const s32 minGray = MAX(0,   parameters->bestGrayvalueThreshold - parameters->minGrayvalueDistance);
      const s32 maxGray = MIN(255, parameters->bestGrayvalueThreshold + parameters->minGrayvalueDistance);
      for(s32 iGray=minGray; iGray<=maxGray; iGray++) {
        GrayvalueBool &pProbesUsed = parameters->probesUsed[parameters->bestProbeIndex];
        pProbesUsed.values[iGray] = true;
      }

      const f64 time1 = GetTimeF64();

      printf(" Best entropy is %f in %f seconds\n", parameters->bestEntropy, time1-time0);

      return 0;
    } // ThreadResult ComputeInfoGain(void *computeInfoGainParameters)
  } // namespace Embedded
} // namespace Anki
