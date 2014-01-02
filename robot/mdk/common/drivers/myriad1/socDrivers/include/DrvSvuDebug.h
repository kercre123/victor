///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Shave debugging applications
///

#ifndef _SVU_DEBUG_
#define _SVU_DEBUG_
#include <mv_types.h>

/// Common init function for DCU, only called once
void dcuInit(void);

/// Assert an asynchronous halt
/// @param shaveNumber a particular Shave on which is applied the action 
void dcuAssertAsyncHalt(u32 shaveNumber);

/// Dessert an asynchronous halt
/// @param shaveNumber a particular Shave on which is applied the action 
void dcuDeassertAsyncHalt(u32 shaveNumber);

/// Clear a breakpoint condition
/// @param shaveNumber a particular Shave on which is applied the action 
/// @param conditionMask a bitmask with desired condition bits set
void dcuClearBreakCondition(u32 shaveNumber,u32 conditionMask);

///continue execution with specified number of steps
/// @param shaveNumber a particular Shave on which is applied the action 
/// @param numSteps number of steps to execute
void dcuContinue(u32 shaveNumber,int numSteps);

///Wait for execution to halt
/// @param shaveNumber a particular Shave on which is applied the action 
/// @return 0 in case of success
u32 dcuWaitForHalt(u32 shaveNumber);

///Wait for execution to stop stalling
/// @param shaveNumber a particular Shave on which is applied the action 
/// @return stall count
u32 dcuWaitForNoStalls(u32 shaveNumber);

/// This function reports the cause of break and takes any necessary action
///@param shaveNumber a particular Shave on which is applied the action 
///@return the last cause of break
u32 dcuHandleHalt(u32 shaveNumber);

/// Set a software BP for a particular Shave at the specified address
///@param shaveNumber a particular Shave on which is applied the action 
///@param address address at which the breakpoint is inserted
///@return number of software breakpoints that are currently set
u32 dcuSetSoftwareBP(u32 shaveNumber,u32 address);

/// Clear a software breakpoint
///@param shaveNumber a particular Shave on which is applied the action 
///@param address the address where the breakpoint resides
///@return 	
///			-  0 on success
///			- -1 on error
///			.
u32 dcuClearSoftwareBP(u32 shaveNumber,u32 address);

///Clear all software breakpoints for a specified Shave
///@param shaveNumber a particular Shave on which is applied the action 
///@return 0
u32 dcuClearAllSoftwareBPs(u32 shaveNumber);

///Clear instruction breakpoints for a particular Shave
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakpointNumber breakpoint number
void dcuClearInstructionBP(u32 shaveNumber, u32 breakpointNumber);

///Clear data breakpoints for a particular Shave
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakpointNumber breakpoint number
void dcuClearDataBP(u32 shaveNumber, u32 breakpointNumber);

/// This function moves a software breakpoint from stage 1 to stage 2
/// It first finds the byte which needs to be replaced and then asserts
/// async halt and replaces the byte. This should restart execution for
/// one instruction
///@param shaveNumber a particular Shave on which is applied the action 
void dcuSoftwareBPMoveToStage2(u32 shaveNumber);

///Dump IDC Fifo contents
///@param shaveNumber a particular Shave on which is applied the action
void dcuDumpIDCFifoContents(u32 shaveNumber);

///Check if specified instruction address is present in IDC Fifo queue
///@param shaveNumber a particular Shave on which is applied the action
///@param address address of the instruction to be checked
///@return 0
int dcuIsAddressInIDCFifo(u32 shaveNumber,u32 address);

///Dump 15 most recent instruction pointers
///@param shaveNumber a particular Shave on which is applied the action
void dcuDumpInstructionHistory(u32 shaveNumber);

///get the value of Instruction Pointer Overwrite(Program Counter) register.
///@param shaveNumber a particular Shave on which is applied the action
///@return the current value of the register
u32 dcuGetPCVal(u32 shaveNumber);

///Get the SVU_INEXT register value: pointer to the next instruction that will be executed.
///@param shaveNumber a particular Shave on which is applied the action
///@return the current value of the register
u32 dcuGetINEXT(u32 shaveNumber);

///Set DBRK_BYTE.  Write to first byte in instruction to fix software instruction breakpoint.
///@param shaveNumber a particular Shave on which is applied the action 
///@param dbrkByte a new SVU_DBK_BYTE register value
void SetSVU_DBRK_BYTE(u32 shaveNumber, u32 dbrkByte);

///Set a data read hardware breakpoint at a specific address
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakAddress address at which the breakpoint is inserted
void SetDRBreak(u32 shaveNumber, u32 breakAddress);

///Set a data write hardware breakpoint at a specific address
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakAddress address at which the breakpoint is inserted
void SetDWBreak(u32 shaveNumber, u32 breakAddress);

///Set a data access hardware breakpoint at a specific address
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakAddress address at which the breakpoint is inserted
void SetDABreak(u32 shaveNumber, u32 breakAddress);

///Set a data read hardware breakpoint at a specific address
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakNumber breakpoint number
///@param breakAddress address at which the breakpoint is inserted
///@param data value for Data Breakpoint Data Register
///@param ctrl value for Data Breakpoint Control Register
void SetDBreak(u32 shaveNumber, u32 breakNumber, u32 breakAddress, u32 data, u32 ctrl);

///Set an instruction hardware breakpoint at a specific address
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakAddress1
///@param breakAddress2
///@param bpNumber breakpoint number
void SetInstBreak(u32 shaveNumber, u32 breakAddress1, u32 breakAddress2, u32 bpNumber);

///Disable an instruction breakpoint for a specified shave
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakpointNumber breakpoint number
void DisableInstBreak(u32 shaveNumber, u32 breakpointNumber);

///Continue after breakpoint is hit
///@param shaveNumber a particular Shave on which is applied the action 
void ContinueSHAVE(u32 shaveNumber);

/// Display IP and I_NEXT
/// Note: PC should be read from SVU_PTR rather than the equivalent TRF register
///@param shaveNumber a particular Shave on which is applied the action 
void OutputIP(u32 shaveNumber);

///Set a software break
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakAddress address at which the breakpoint is inserted
void SetSBreak(u32 shaveNumber, u32 breakAddress);

///Disable a data breakpoint for a specified shave
///@param shaveNumber a particular Shave on which is applied the action 
///@param breakpointNumber breakpoint number
void RemoveDBreak(u32 shaveNumber, u32 breakpointNumber);
#endif
