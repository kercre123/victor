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

typedef struct {
  s32 counts[256];
} GrayvalueCounts;

static void CountValues_u8(const u8 * restrict pFeatureValues, const RawBuffer<s32> &remaining, GrayvalueCounts &counts)
{
  memset(&counts.counts[0], 0, 256*sizeof(s32));

  const s32 numRemaining = remaining.size;
  const s32 * restrict pRemaining = remaining.buffer;

  for(s32 i=0; i<numRemaining; i++) {
    counts.counts[pFeatureValues[pRemaining[i]]]++;
  }
} // CountUnique_u8()

static void ComputeNumAboveThreshold(
  const u8 * restrict pFeatureValues,
  const s32 * restrict pLabels,
  const s32 maxLabel,
  const s32 * restrict pRemaining,
  const s32 numRemaining,
  const u8 curGrayvalueThreshold,
  const s32 u8MinDistanceFromThreshold,
  s32 * restrict pNumLT,
  s32 * restrict pNumGE,
  s32 &totalNumLT,
  s32 &totalNumGE,
  s32 &totalNumBoth,
  u8 &meanDistanceFromThreshold)
{
  memset(pNumLT, 0, (maxLabel+1)*sizeof(s32));
  memset(pNumGE, 0, (maxLabel+1)*sizeof(s32));

  totalNumLT = 0;
  totalNumGE = 0;
  totalNumBoth = 0;
  meanDistanceFromThreshold = 0;

  s32 totalDifferenceFromThreshold = 0;

  const s32 curGrayvalueThresholdS32 = curGrayvalueThreshold;

  for(s32 iRemain=0; iRemain<numRemaining; iRemain++) {
    const s32 iImage = pRemaining[iRemain];

    const s32 curLabel = pLabels[iImage];
    const s32 curFeatureValue = pFeatureValues[iImage];

    totalDifferenceFromThreshold += ABS(curFeatureValue - curGrayvalueThresholdS32);

    // NOTE: One item may increment both LT and GE

    s32 numAssigned = 0;

    if(curFeatureValue < (curGrayvalueThresholdS32 + u8MinDistanceFromThreshold)) {
      totalNumLT++;
      pNumLT[curLabel]++;
      numAssigned++;
    }

    if(curFeatureValue >= (curGrayvalueThresholdS32 - u8MinDistanceFromThreshold)) {
      totalNumGE++;
      pNumGE[curLabel]++;
      numAssigned++;
    }

    if(numAssigned == 2) {
      totalNumBoth++;
    }
  }

  meanDistanceFromThreshold = saturate_cast<u8>(totalDifferenceFromThreshold / numRemaining);
}

// Type should be f32 or f64
template<typename Type> static Type WeightedAverageEntropy(
  const s32 * restrict pNumLT,
  const s32 * restrict pNumGE,
  const s32 totalNumLT,
  const s32 totalNumGE,
  const s32 maxLabel)
{
  Type entropyLessThan = 0;
  Type entropyGreaterThan = 0;

  const Type inverseTotalNumLessThan = static_cast<Type>(1) / static_cast<Type>(totalNumLT);
  const Type inverseTotalNumGreaterThan = static_cast<Type>(1) / static_cast<Type>(totalNumGE);

  for(s32 iLabel=0; iLabel<=maxLabel; iLabel++) {
    //probabilitiesLessThan = allValuesLessThan / sum(allValuesLessThan);
    //entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
    if(pNumLT[iLabel] > 0) {
      const Type probability = static_cast<Type>(pNumLT[iLabel]) * inverseTotalNumLessThan;
      entropyLessThan -= probability * log2(probability);
    }

    //probabilitiesGreaterThan = allValuesGreaterThan / sum(allValuesGreaterThan);
    //entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
    if(pNumGE[iLabel] > 0) {
      const Type probability = static_cast<Type>(pNumGE[iLabel]) * inverseTotalNumGreaterThan;
      entropyGreaterThan -= probability * log2(probability);
    }
  } // for(s32 iLabel=0; iLabel<=maxLabel; iLabel++)

  const Type percent_lessThan    = static_cast<Type>(totalNumLT)    / static_cast<Type>(totalNumLT + totalNumGE);
  const Type percent_greaterThan = static_cast<Type>(totalNumGE) / static_cast<Type>(totalNumLT + totalNumGE);

  const Type weightedAverageEntropy = percent_lessThan * entropyLessThan + percent_greaterThan * entropyGreaterThan;

  return weightedAverageEntropy;
}

namespace Anki
{
  namespace Embedded
  {
    ThreadResult ComputeInfoGain(void *computeInfoGainParameters)
    {
      ComputeInfoGainParameters * restrict parameters = reinterpret_cast<ComputeInfoGainParameters*>(computeInfoGainParameters);

      parameters->bestEntropy = FLT_MAX;
      parameters->bestFeatureIndex = -1;
      parameters->bestU8Threshold = -1;
      parameters->totalNumLT = -1;
      parameters->totalNumGE = -1;
      parameters->meanDistanceFromThreshold = 255;

      u8 u8Thresholds[256];
      s32 numGrayvalueThresholds;

      // If u8ThresholdsToUse has been passed in, don't compute the u8Thresholds
      if(parameters->u8ThresholdsToUse.get_size() != 0) {
        numGrayvalueThresholds = parameters->u8ThresholdsToUse.get_size();
        for(s32 i=0; i<numGrayvalueThresholds; i++) {
          u8Thresholds[i] = parameters->u8ThresholdsToUse[i];
        }
      }

      //const s32 numImages = parameters->featureValues[0].get_size();
      const s32 numRemaining = parameters->remaining.size;

      const s32 maxLabel = FindMaxLabel(parameters->labels, parameters->remaining);

      const s32 * restrict pLabels = parameters->labels.Pointer(0);
      const s32 * restrict pRemaining = parameters->remaining.buffer;

      //
      // For each feature location and grayvalue threshold, find the best entropy
      //

      for(s32 iFeature=parameters->minFeatureToCheck; iFeature<=parameters->maxFeatureToCheck; iFeature++) {
        const u8 * restrict pFeatureValues = parameters->featureValues[iFeature].Pointer(0);

        // If the u8ThresholdsToUse haven't been specified, compute them from the data
        if(parameters->u8ThresholdsToUse.get_size() == 0) {
          // What are the unique grayvalues for this feature?
          GrayvalueCounts counts;
          CountValues_u8(pFeatureValues, parameters->remaining, counts);

          // Compute the grayvalue thresholds (just halfway between each pair of detected grayvalues
          numGrayvalueThresholds = 0;
          s32 previousValue = -1;
          for(s32 i=0; i<256; i++) {
            if(counts.counts[i] > 0) {
              if(previousValue > 0) {
                u8Thresholds[numGrayvalueThresholds] = (i + previousValue + 1) / 2; // The +1 is so it rounds up, like matlab
                numGrayvalueThresholds++;
                previousValue = i;
              } else {
                previousValue = i;
              }
            }
          }
        } // if(parameters->u8ThresholdsToUse.empty())

        U8Bool &pFeaturesUsed = parameters->featuresUsed.buffer[iFeature];

        for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++) {
          const u8 curGrayvalueThreshold = u8Thresholds[iGrayvalueThreshold];

          if(pFeaturesUsed.values[curGrayvalueThreshold]) {
            continue;
          }

          s32 totalNumLT = 0;
          s32 totalNumGE = 0;
          s32 totalNumBoth = 0;
          u8 meanDistanceFromThreshold = 0;

          ComputeNumAboveThreshold(
            pFeatureValues,
            pLabels, maxLabel,
            pRemaining, numRemaining,
            curGrayvalueThreshold,
            parameters->u8MinDistanceFromThreshold,
            parameters->pNumLT, parameters->pNumGE,
            totalNumLT, totalNumGE, totalNumBoth,
            meanDistanceFromThreshold);

          AnkiAssert((totalNumLT-totalNumBoth) >= 0 && (totalNumGE-totalNumBoth) >= 0);

          if((totalNumLT-totalNumBoth) <= 0 || (totalNumGE-totalNumBoth) <= 0) {
            pFeaturesUsed.values[curGrayvalueThreshold] = true;
            continue;
          }

          const PRECISION entropy = WeightedAverageEntropy<PRECISION>(
            parameters->pNumLT, parameters->pNumGE,
            totalNumLT, totalNumGE,
            maxLabel);

          // If the entropy is less, or the the entropy is the same and the mean distance is more
          if((entropy < parameters->bestEntropy) ||
            (entropy <= parameters->bestEntropy && meanDistanceFromThreshold > parameters->meanDistanceFromThreshold)) {
              parameters->bestEntropy = static_cast<f32>(entropy);
              parameters->bestFeatureIndex = iFeature;
              parameters->totalNumLT = totalNumLT;
              parameters->totalNumGE = totalNumGE;
              parameters->totalNumBoth = totalNumBoth;
              parameters->bestU8Threshold = curGrayvalueThreshold;
              parameters->meanDistanceFromThreshold = meanDistanceFromThreshold;
          }
        } // for(s32 iGrayvalueThreshold=0; iGrayvalueThreshold<numGrayvalueThresholds; iGrayvalueThreshold++)
      } // for(s32 iFeature=0; iFeature<numFeaturesLocationsToCheck; iFeature++)

      return 0;
    } // ThreadResult ComputeInfoGain(void *computeInfoGainParameters)
  } // namespace Embedded
} // namespace Anki
