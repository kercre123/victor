///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup UnitTest Unit Test API
/// @{
/// @brief     Unit Test API
///
/// @details Used for test reporting
///

#ifndef _UNIT_TEST_API_H
#define _UNIT_TEST_API_H

// 1: Includes
// ----------------------------------------------------------------------------
#include "UnitTestDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Initiate a new unit test
int unitTestInit(void);

/// checks if a floating point value is within a certain percentage of 
/// expected value
/// actual+1 is considered 100% deviation, actual is 0% deviation
/// @param[in] actual           actual floating point value
/// @param[in] expected         expected floating point value
/// @param[in] percentageError  acceptable percent deviation as explained above
/// @return
int unitTestFloatWithinRange(float actual, float expected, float percentageError);

/// checks if a floating point value is within an acceptable margin
/// expected value
/// unitTestExecutionWithinRange(actual,100, 10) passes if actual >90 and less than 110
/// @param[in] actual           actual execution time
/// @param[in] expected         expected execution time
/// @param[in] percentageError  Accepted execution time error
/// @return
int unitTestExecutionWithinRange(float actual, float expected, float percentageError);

/// checks if a floating point value is within an acceptable margin
/// expected value
/// @param[in] actual    existing float
/// @param[in] expected  expected float
/// @param[in] AbsError  accepted absolute error
/// @return
int unitTestFloatAbsRangeCheck(float actual, float expected, float AbsError);

/// check if a logical condition is true or not
/// @param value the value to be checked
int unitTestAssert(int value);

///Log a passed test
int unitTestLogPass(void);

///Log a failed test
int unitTestLogFail(void);

///Set up the passed and failed tests results
///@param passes number of passed tests
///@param fails number of failed tests
int unitTestLogResults(int passes,int fails);

///Print the final results of the unit testing
int unitTestFinalReport(void);
/// @}
#endif // _BOARD_API_H_
