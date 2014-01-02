#include "gtest/gtest.h"
#include <stdlib.h>
#include <math.h>

template<class T>
void CompareArrays(T* refData, T* actData, unsigned int refLineSize, unsigned int actLineSize, float minMatchesPercentage = 100.0f, T delta = T(0));

template<class T>
void CompareArrays(T* refData, T* actData, unsigned int refLineSize, unsigned int actLineSize, 
    float minMatchesPercentage, T delta)
{
    // If the number of results is not the same, the difference counts as mismatches
    unsigned int extraMismatches = abs(refLineSize - actLineSize);
    unsigned int mismatchesCount = actLineSize;
    unsigned int refIdx = 0;
    unsigned int actIdx = 0;
    T foundMarker = (T)0xFFFFFFFF; // Set all bits to 1
    
    for (actIdx = 0; actIdx < actLineSize; actIdx++)
    {
        for (refIdx = 0; refIdx < refLineSize; refIdx++)
        {
            if (fabs(actData[actIdx] - refData[refIdx]) <= delta)
            {
                refData[refIdx] = foundMarker;
                mismatchesCount--;
                break;
            }
        }
    }
    
    // Sanity checks
    assert(actIdx <= actLineSize);
    assert(refIdx <= refLineSize);
    assert(mismatchesCount >= 0);
    
    mismatchesCount += extraMismatches;
    
    // Percentage of passed values 
    float matchesPercentage = ((refLineSize - mismatchesCount) / (float)refLineSize) * 100.0f;
    
    EXPECT_GE(matchesPercentage, minMatchesPercentage);
}

template<class T>
void EXPECT_NEAR_PERCENT(T expected, T actual, float percentDelta)
{
    float diff = fabs((float)expected - (float)actual);
    float percentDiff = (diff / expected) * 100.0f;
    
    EXPECT_LE(percentDiff, percentDelta);
}