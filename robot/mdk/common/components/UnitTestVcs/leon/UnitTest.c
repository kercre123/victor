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
#include "UnitTestApi.h"
#include "registersMyriad.h"
#include "stdio.h"
// UnitTestVcs is dependent on VcsHooks component
#include <VcsHooksApi.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ---------------------------------------------------------------------------


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
                   
// 4: Static Local Data
// ----------------------------------------------------------------------------
static u32 verbosity=VERBOSITY_QUIET; // Default to summary info including pass and fails
static u32 numPass;
static u32 numFail;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void unitTestInit(void)
{
    unitTestVerbosity(verbosity); // This signals the simulator to use our default verbosity level
    numPass=0;
    numFail=0;
    return;
}

void unitTestVerbosity(tyUnitVerbosity targetVerbosity)
{
    verbosity = targetVerbosity;
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_SET_VERBOSITY,(u32)targetVerbosity,0,0,0); // Signal the hardware that the verbosity is changed
    return;
}

void unitTestLogPass(void)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_ASSERT,1,0,0,0);  // Simulate pass with assert(1);
    numPass++;
    return;
}

void unitTestLogFail(void)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_ASSERT,0,0,0,0);  // Simulate fail with assert(0); 
    numFail++;
    return;
}

void unitTestAssert(int value)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_ASSERT,value,0,0,0);
    if (!IS_PLATFORM_VCS)
    {
        if (value)
            numPass++;
        else
        {
            numFail++;
            printf("\n unitTestAssert() -> Fail");
        }
    }
    return;
}

void unitTestCompare(u32 actualValue, u32 expectedValue)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_WORD_CMP,actualValue,expectedValue,0,0);
    if (!IS_PLATFORM_VCS)
    {
        if (actualValue == expectedValue)
        {
            numPass++;
            printf(" unitTestCompare() -> Pass\n");
        }
        else
        {
            numFail++;
            printf(" unitTestCompare() -> Fail  (%08X,%08X)\n",actualValue,expectedValue);
        }
    }
    return;
}

void unitTestCompare64(u64 actualValue, u64 expectedValue)
{
    u32 actualUpper;
    u32 actualLower;
    u32 expectedUpper;
    u32 expectedLower;                
    actualUpper   = (actualValue>>32)  & 0xFFFFFFFF ;
    actualLower   = (actualValue>>0)   & 0xFFFFFFFF ;
    expectedUpper = (expectedValue>>32)& 0xFFFFFFFF ;
    expectedLower = (expectedValue>>0) & 0xFFFFFFFF ;
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_DWORD_CMP,actualUpper,actualLower,expectedUpper,expectedLower,0,0); // Perform comparison in hardware
    if (!IS_PLATFORM_VCS)
    {
        if (actualValue == expectedValue)
        {
            numPass++;
            printf("\n unitTestCompare64() -> Pass");
        }
        else
        {
            numFail++;
            printf("\n unitTestCompare64() -> Fail  (0x%08X%08X,0x%08X%08X)",actualUpper,actualLower,expectedUpper,expectedLower);
        }
    }
    return;
}

void unitTestReadDWordCheck(void * dWordAddress, u64 expectedValue)
{
    u64 actualValue;
    u32 actualUpper;
    u32 actualLower;
    u32 expectedUpper;
    u32 expectedLower;                

    actualValue = GET_REG_DWORD_VAL(dWordAddress);
    actualUpper   = (actualValue>>32)  & 0xFFFFFFFF ;
    actualLower   = (actualValue>>0)   & 0xFFFFFFFF ;
    expectedUpper = (expectedValue>>32)& 0xFFFFFFFF ;
    expectedLower = (expectedValue>>0) & 0xFFFFFFFF ;

    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_WORD_CMP_ADR,VCS_HOOK_ID_UNIT_TEST_DWORD_CHK,actualUpper,actualLower,expectedUpper,expectedLower,(u32)dWordAddress); 

    return;
}

void unitTestReadWordCheck(void * wordAddress, u32 expectedValue)
{
    u32 actualValue;

    actualValue = GET_REG_WORD_VAL(wordAddress);
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_WORD_CMP_ADR,VCS_HOOK_ID_UNIT_TEST_WORD_CHK,0,actualValue,0,expectedValue,(u32)wordAddress); 
    return;
}

void unitTestReadHalfCheck(void * address, u16 expectedValue)
{
    u16 actualValue;
    actualValue = GET_REG_HALF_VAL(address);
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_WORD_CMP_ADR,VCS_HOOK_ID_UNIT_TEST_HALF_CHK,0,(u32)actualValue,0,(u32)expectedValue,(u32)address); 
    return;
}

void unitTestReadByteCheck(void * address, u8 expectedValue)
{
    u8 actualValue;
    actualValue = GET_REG_BYTE_VAL(address);
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_WORD_CMP_ADR,VCS_HOOK_ID_UNIT_TEST_BYTE_CHK,0,(u32)actualValue,0,(u32)expectedValue,(u32)address); 
    return;
}

void unitTestReadBitCheck(void * address, u32 startBit, u32 numBits, u32 expectedValue)
{
    u32 actualValue;
    u32 bitMask = ( numBits >= 32U ? 0xFFFFFFFF : ((1U << numBits) - 1U) ); 

    actualValue = GET_REG_WORD_VAL(address);
    actualValue = actualValue >> startBit;
    actualValue &= bitMask;
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_WORD_CMP_ADR,VCS_HOOK_ID_UNIT_TEST_BIT_CHK,0,(u32)actualValue,0,(u32)expectedValue,(u32)address); 
    return;
}

void unitTestCrcCheck(const void * pStart, u32 lengthBytes, u32 expectedCrc)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_MEM_CRC_VERIFY,(u32)pStart,lengthBytes,expectedCrc,0);
    return;
}

void unitTestMemCompare(const void * pActualStart,const void * pExpectedStart, u32 lengthBytes)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_MEM_COMPARE,(u32)pActualStart,(u32)pExpectedStart,lengthBytes,0);
    return;
}

void unitTestSetTestType(tyTestType testType)
{                              
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_SET_TEST_TYPE,(u32)testType,0,0,0);
}      

void unitTestFinalReport(void)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_DISPLAY_RESULT,0,0,0,0);
    if (!IS_PLATFORM_VCS)
    {
        if (numFail != 0)
        {
            printf("unitTest: Failed\n"); // TODO: Refine message
        }
        else
        {
            printf("unitTest: Pass\n"); // TODO: Refine message
        }
    }

    return;
}
