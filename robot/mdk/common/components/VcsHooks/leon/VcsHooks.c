///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Set of functions to allow test case interaction with the VCS Test Environment
///
/// This module allows the use of optimised routines which speed up simulation of test cases.
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <VcsHooksApi.h>
#include <registersMyriad.h>
#include <DrvRegUtils.h>
#include <swcLeonUtils.h>
#include <stdio.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
                   
// 4: Static Local Data
// ----------------------------------------------------------------------------
// This is a local copy of the debug_test_state hardware register
static cache_debug_test_state = 0;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void printInt(u32 value)
{
//  if (GET_CURRENT_PLATFORM() == PLAT_VCS_SIM)
//  {
      SET_REG_WORD((u32)(ACT_DATA_ADR),value);
//  }
//  else
//  {
//      printf("\n%08X",value);
//  }

  return;
}

void printMsgInt(const char * msg,u32 value)
{
//  unsigned char nextChar;
//  if (GET_CURRENT_PLATFORM() == PLAT_VCS_SIM)
//  {
      puts(msg);
      SET_REG_WORD((u32)(DEBUG_MSG_INT),value);
//  }
//  else
//  {
//      printf("\n%s 0x%08X",msg,value);
//  }
  return;
}

void displayRawMemory(void * address, u32 length)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_MEM_DISPLAY,(u32)address, length, length, 0);

//    SET_REG_WORD(LEON_MEM_DISP_START,address);
//    SET_REG_WORD(LEON_MEM_DISP_LEN  ,length);
    return;
}

void vcsHookFastMemCpy(void * dst, void * src, u32 length)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_MEM_CPY,(u32) dst, (u32) src, length, 0);
    return;
}

void dumpMemoryToFile(u32 address, u32 length)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_MEM_FILE_DUMP,address, length, length, 0);
//    SET_REG_WORD(LEON_MEM_DUMP_START,address);
//    SET_REG_WORD(LEON_MEM_DUMP_LEN  ,length);
    return;
}

void loadMemFromFile(char * pFileNameOpt, u32 optIndex, u32 fileOffset, u32 bytesToLoad, void * targetLoadAddress)
{
    vcsHookFunctionCallParam6(VCS_HOOK_FUNC_MEM_FILE_LOAD,(u32)pFileNameOpt, optIndex, fileOffset, bytesToLoad, (u32) targetLoadAddress,0);
    return;
}

void vcsFastPuts(char * pString)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_FAST_PUTS,(u32)pString,0,0,0);
    return;
}

void testStateSet(u32 value)
{
    cache_debug_test_state = value;
    SET_REG_WORD(DEBUG_TEST_STATE,cache_debug_test_state);
}

void vcsHookVerilogEventTrigger(u32 eventCode)
{
    vcsHookFunctionCallParam4(VCS_HOOK_FUNC_VERILOG_TRIGGER,eventCode, 0 , 0 , 0);
}
void testStateInc(void)
{
    cache_debug_test_state++;
    SET_REG_WORD(DEBUG_TEST_STATE,cache_debug_test_state);
}

void testStateAdd(u32 value)
{
    cache_debug_test_state += value;
    SET_REG_WORD(DEBUG_TEST_STATE,cache_debug_test_state);
}


