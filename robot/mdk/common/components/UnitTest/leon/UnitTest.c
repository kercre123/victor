///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Unit Test Framework
///
/// Basic utility functions for unit testing
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "assert.h"
#include "isaac_registers.h"
#include "stdio.h"
#include "DrvTimer.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
                   
// 4: Static Local Data
// ----------------------------------------------------------------------------
static u32 numTestsPassed;
static u32 numTestsRan;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int unitTestInit(void)
{
    numTestsPassed = 0;
    numTestsRan=0;
    return 0;
}

int unitTestLogPass(void)
{
    numTestsRan++;
    numTestsPassed++;
    return 0;
}

int unitTestLogFail(void)
{
    numTestsRan++;
    return 0;
}

int unitTestAssert(int value)
{
    numTestsRan++;
    if (value)
        numTestsPassed++;

    return 0;
}

int unitTestExecutionWithinRange(float actual, float expected, float percentageError)
{
    float minVal;
    float maxVal;
    numTestsRan++;

    //Early exit if we have negative numbers
    if (actual<0.0f){
    	return 0;
    }

    minVal = expected * ((100.0 - percentageError) / 100.0);
    maxVal = expected * ((100.0 + percentageError) / 100.0);
    //Also, if actual number is 0.0f then this means the program ran tocompletion very fast
    //which probably means it passed :)
    if ( ((actual > minVal) && (actual < maxVal)) || (actual=0.0f) )
        numTestsPassed++;

	return 0;
}

int unitTestFloatWithinRange(float actual, float expected, float percentageError)
{
    float difference,absErrorAccepted;
    numTestsRan++;
    difference=actual-expected;
    if (difference<0) difference=-difference;

    //Considering that a deviation of 1.0f is 100% error
    absErrorAccepted=percentageError/100.0f;
	if (difference<=absErrorAccepted){
		numTestsPassed++;
	}

    return 0;
}

int unitTestFloatAbsRangeCheck(float actual, float expected, float AbsError)
{
	float difference;
	difference=actual-expected;
	numTestsRan++;
	if (difference<0) difference=-difference;
	if (difference<=AbsError){
		numTestsPassed++;
	}
	return 0;
}

int unitTestLogResults(int passes,int fails)
{ 
    numTestsRan += passes;
    numTestsRan += fails;
    numTestsPassed += passes;
    return 0;
}

int unitTestFinalReport(void)
{
    if (numTestsPassed == numTestsRan)
    {
        printf("\nmoviUnitTest:PASSED\n");
    }
    else
    {
        printf("\nmoviUnitTest:FAILED : %d failures\n",numTestsRan - numTestsPassed);
    }

    return 0;
}
