///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Unit Test Framework
/// 
#ifndef _UNIT_TEST_DEFINES_H
#define _UNIT_TEST_DEFINES_H 

#include "mv_types.h"
// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef enum
{
  VERBOSITY_SILENT          = 0x10,     // Only display summary of errors 
  VERBOSITY_QUIET           = 0x20,     // Display summary of errors and passes but no buffer info 
  VERBOSITY_DIFFS           = 0x30,     // Display summary of errors and passes and also list differences within buffers 
  VERBOSITY_ALL             = 0x40,     // Display summary of errors and passes and also display full buffer contents during comparison  
} tyUnitVerbosity;

typedef enum
{
    EXPECT_TESTS_TO_PASS     = 1, // Positive testing (normal and default) Expect that the test will PASS
    EXPECT_TESTS_TO_FAIL     = 2, // Negative testing Expect that the test will FAIL (Can be used for example when testing the test API)
} tyTestType;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BOARD_DEF_H

