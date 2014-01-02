///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Shave debugging applications
///

#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvL2Cache.h"
#include <DrvSvuDefines.h>
#include <DrvSvuDebug.h>
#include <DrvSvu.h>
#include <stdio.h> // Note: Printfs need to be removed from this driver

static SW_BP_TYPE  breakpointArray[MAX_SW_BPS];
static u32 numSWBreakpoints=0;
static u32 lastOperationSingleStep=0;
static u32 inSWBPStage1=0;
static u32 lastCauseOfBreak=0;

//Local functions only. Not exported anywhere but used by the functions in this file
u32 ShaveToLeonAddress(u32 shaveNumber,u32 address)
{
	u32 window;
	u32 windowBase;
	u32 leonAddress;
	//Calculate address of the WindowA register
	u32 base = SHAVE_0_BASE_ADR + SLC_TOP_OFFSET_WIN_A + SVU_SLICE_OFFSET * shaveNumber;

	window = (address >> 24) & 0xFF;

	switch (window) {
	case 0x1c:
		windowBase  = GET_REG_WORD_VAL(base + 0x0);
		leonAddress = windowBase + (address & 0x00FFFFFF);
		break;
	case 0x1d:
		windowBase  = GET_REG_WORD_VAL(base + 0x4);
		leonAddress = windowBase + (address & 0x00FFFFFF);
		break;
	case 0x1e:
		windowBase  = GET_REG_WORD_VAL(base + 0x8);
		leonAddress = windowBase + (address & 0x00FFFFFF);
		break;
	case 0x1f:
		windowBase  = GET_REG_WORD_VAL(base + 0xC);
		leonAddress = windowBase + (address & 0x00FFFFFF);
		break;
	default:
		// If we aren't in a window the address isn't modified
		leonAddress = address;
		break;
	}
/*
	DEBUG_INFO("ShaveToLeonAddress; Shave = ");
	ACTUAL_DATA(address);
	DEBUG_INFO("ShaveToLeonAddress; Leon  = ");
	ACTUAL_DATA(leonAddress);
*/
	return leonAddress;
}

// In order to do byte access using Leon we must use word accesses
// to avoid endian problems
u8 getShaveMemByte(u32 shaveNumber,u32 address)
{
	u32 leonAddress;
	u32 word;
	u32 offset=

	// First convert address to a leon address
	leonAddress = ShaveToLeonAddress(shaveNumber,address);

	word = GET_REG_WORD_VAL(leonAddress & 0xFFFFFFFC);

	offset = (leonAddress - (leonAddress & 0xFFFFFFFC)) * 8;

	return(word >> offset) & 0xFF;
}

// In order to do byte access using Leon we must use word accesses
// to avoid endian problems
void setShaveMemByte(u32 shaveNumber,u32 address,u8 val)
{
	u32 leonAddress;
	u32 word;
	u32 offset;
	u32 mask;

	// First convert address to a leon address
	leonAddress = ShaveToLeonAddress(shaveNumber,address);

	word = GET_REG_WORD_VAL(leonAddress & 0xFFFFFFFC);

	offset = (leonAddress - (leonAddress & 0xFFFFFFFC)) * 8;
	mask   = ~(0xFF << offset);
	word  &= mask;
	word  |= (val << offset);

	// Write back the modified word
	SET_REG_WORD(leonAddress & 0xFFFFFFFC,word);

	return;
}

// Common init function for DCU, only called once
void dcuInit(void)
{
	int shaveNumber;

	// Enable DCU for all shaves
	for (shaveNumber=0;shaveNumber<=7;shaveNumber++) {
		u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
		SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	}

	// Reset the software breakpoint array
	numSWBreakpoints=0;
	// Reset last operation
	lastOperationSingleStep=0;
	inSWBPStage1=0;
	lastCauseOfBreak=0;
}

void dcuAssertAsyncHalt(u32 shaveNumber)
{
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_HALT);
}

void dcuDeassertAsyncHalt(u32 shaveNumber)
{
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	CLEAR_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_HALT);
}

void dcuClearBreakCondition(u32 shaveNumber,u32 conditionMask)
{
	u32 dsrValue = GET_REG_WORD_VAL(DCU_DSR(shaveNumber));
	CLEAR_BITS(DCU_DSR(shaveNumber), dsrValue, conditionMask);
}

void dcuContinue(u32 shaveNumber,int numSteps)
{
	int needToStep=0;
	u32 temp;
	u32 localCauseOfBreak;
	u32 continueExecution = 0;
	u32 dcrValue;

	//DEBUG_INFO("dcuContinue");

	if (numSteps == ASYNC_CONTINUE) {
		continueExecution = 1;
	}

	// At its simplest this is just a deassert of async halt
	// However there are multiple scenarios to be covered

	// Backup the cause of break as it will change when we step
	localCauseOfBreak = lastCauseOfBreak;

	// If we are in SWBP Stage1
	if (inSWBPStage1) {
		needToStep=1;
	}

//	Breakpoints are now disabled immediately on halt in handleHalt
	if (localCauseOfBreak & COB_IBP0) {
//		DEBUG_INFO("temp disable IBP0");
		// Temp disable IBP0, step once
//		temp = GET_REG_WORD_VAL(DCU_IBC0(shaveNumber));
//		CLEAR_BITS(DCU_IBC0(shaveNumber), temp, DCU_IBP_EN);
		needToStep=1;
	}

	if (localCauseOfBreak & COB_IBP1) {
//		DEBUG_INFO("temp disable IBP1");
		// Temp disable IBP1, step once
//		temp = GET_REG_WORD_VAL(DCU_IBC1(shaveNumber));
//		CLEAR_BITS(DCU_IBC1(shaveNumber), temp, DCU_IBP_EN);
		needToStep=1;
	}

	if (needToStep) {
		if (inSWBPStage1) {
			//DEBUG_INFO("SWBP1->2");
			// Step using SWBP Move to stage 2
			dcuSoftwareBPMoveToStage2(shaveNumber);
			// This will restart execution for one step.
			// We must wait for halt as we may need to re-enable breakpoints afterwards
			dcuWaitForHalt(shaveNumber);
			continueExecution = 0;
			numSteps--;
		}
		else{
			//DEBUG_INFO("SS first inst.");
			// Step using Single Step
			dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
			SET_BITS(DCU_DCR(shaveNumber), ( dcrValue & ~DCR_STEP_NUM_MASK), DCR_STEP(1));

			lastOperationSingleStep=1;
			// We must wait for halt as we may need to re-enable breakpoints afterwards
			dcuWaitForHalt(shaveNumber);
			// Check the cause of break
			dcuHandleHalt(shaveNumber);
			// Reset the SS flag as handle halt will be called again by the user
			lastOperationSingleStep=1;
			// Potentially continue if there is no other cause of break
			if (lastCauseOfBreak & COB_SS)
				numSteps--;
			else
				continueExecution = 0;
		}
		needToStep=0;
	}

	if (localCauseOfBreak & COB_IBP0) {
		// Re-enable the breakpoint
		//DEBUG_INFO("re-enable IBP0");
		temp = GET_REG_WORD_VAL(DCU_IBC0(shaveNumber));
		SET_BITS(DCU_IBC0(shaveNumber), temp, DCU_IBP_EN);
	}

	if (localCauseOfBreak & COB_IBP1) {
		// Re-enable the breakpoint
		//DEBUG_INFO("re-enable IBP1");
		temp = GET_REG_WORD_VAL(DCU_IBC1(shaveNumber));
		SET_BITS(DCU_IBC1(shaveNumber), temp, DCU_IBP_EN);
	}


	if (continueExecution) {
		//DEBUG_INFO("deassertAsyncHalt");
		dcuDeassertAsyncHalt(shaveNumber);
	} else{
		if (numSteps>0) {
			//DEBUG_INFO("Stepping:");
			//ACTUAL_DATA(numSteps);

			dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
			SET_BITS(DCU_DCR(shaveNumber), ( dcrValue & ~DCR_STEP_NUM_MASK), DCR_STEP(numSteps));
			lastOperationSingleStep=1;
		}
	}
}

u32 dcuWaitForHalt(u32 shaveNumber)
{
	u32 dsrValue;
	u32 osrValue;
	do
	{

		dsrValue = GET_REG_WORD_VAL(DCU_DSR(shaveNumber));
		osrValue = GET_REG_WORD_VAL(DCU_OSR(shaveNumber));
	}   while (  (
				  !(dsrValue & DCU_DSR_DM)
				  )  &&
				 (
				  !(osrValue & OSR_SWI_HALT)
				 )
			  ); // Wait for execution to halt

	if (dsrValue & DCU_DSR_DM) {
		//DEBUG_INFO("..DM..");
	}
/*
	if (osrValue & OSR_SWI_HALT) {
		DEBUG_INFO("SWIH");
	}
*/
//	ACTUAL_DATA(osrValue);
	return 0;
}

u32 dcuWaitForNoStalls(u32 shaveNumber)
{
	u32 osrValue;
	int stallCount=0;

	do
	{
		osrValue = GET_REG_WORD_VAL(DCU_OSR(shaveNumber));
		stallCount++;
	} while((osrValue & OSR_SVU_STALL) && (stallCount < 500));
	return stallCount;
}

u32 dcuHandleHalt(u32 shaveNumber)
{
	u32 dsrValue;
	u32 dcrValue;
	u32 osrValue;
	u32 temp;

	u32 handleAnotherHalt=0;

	//DEBUG_INFO("dcuHH()");

	// This function reports the cause of break and takes any necessary action
	do
	{
		handleAnotherHalt=0;
		dsrValue = GET_REG_WORD_VAL(DCU_DSR(shaveNumber));
		osrValue = GET_REG_WORD_VAL(DCU_OSR(shaveNumber));
//		ACTUAL_DATA(dsrValue);
		lastCauseOfBreak = 0;

		if (osrValue & OSR_SWI_HALT) {
			//DEBUG_INFO("COB_SWIH");
			lastCauseOfBreak |= COB_SWIH;
		}

		if (dsrValue & DCU_DSR_DM) {
			// Handle Cause of break

			// This must be first as we set the bit later on
			if (dsrValue & DCU_DSR_SG) {
				// The first thing we check with an Async halt is if the the PC==0
				// This signifies that we have attempted to halt the application in debug mode
				// immediately after starting.
				// However in order to support this we must first single step so that the IDC FIFO gets
				// loaded
				if (GET_REG_WORD_VAL(DCU_SVU_PTR(shaveNumber)) == 0) {
					// Single step once to move us to the first instruction
					dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
					SET_BITS(DCU_DCR(shaveNumber), ( dcrValue & ~DCR_STEP_NUM_MASK), DCR_STEP(1));
					dcuWaitForHalt(shaveNumber);
					handleAnotherHalt=1;
					continue;
				}

				// Annoying that we have to remember our last operation was single step
				// There should be a bit in the DSR to tell us that
				if (lastOperationSingleStep) {
					//DEBUG_INFO("COB_SS");
					lastOperationSingleStep=0;
					lastCauseOfBreak |= COB_SS;
				}
				else if (inSWBPStage1) {
					//DEBUG_INFO("COB_SWBP");
					inSWBPStage1=0;
					handleAnotherHalt=0;
					lastCauseOfBreak |= COB_SWBP;
				}
				else
				{
					//DEBUG_INFO("COB_ASYNC");
					lastCauseOfBreak |= COB_ASYNC;
				}
			}

			if (dsrValue & DCU_DSR_IBP0) {
				//DEBUG_INFO("COB_IBP0");
				lastCauseOfBreak |= COB_IBP0;
				dcuAssertAsyncHalt(shaveNumber);
				// There is no point in clearing the break condition until we temp disable
				// the breakpoint. This prevents it from automatically re-triggering
				//DEBUG_INFO("temp dis IBP0");
				temp = GET_REG_WORD_VAL(DCU_IBC0(shaveNumber));
				CLEAR_BITS(DCU_IBC0(shaveNumber), temp, DCU_IBP_EN);

				dcuClearBreakCondition(shaveNumber,DCU_DSR_IBP0);
			}

			if (dsrValue & DCU_DSR_IBP1) {
				//DEBUG_INFO("COB_IBP1");
				lastCauseOfBreak |= COB_IBP1;
				dcuAssertAsyncHalt(shaveNumber);
				// There is no point in clearing the break condition until we temp disable
				// the breakpoint. This prevents it from automatically re-triggering
				//DEBUG_INFO("temp dis IBP1");
				temp = GET_REG_WORD_VAL(DCU_IBC1(shaveNumber));
				CLEAR_BITS(DCU_IBC1(shaveNumber), temp, DCU_IBP_EN);

				dcuClearBreakCondition(shaveNumber,DCU_DSR_IBP1);
			}

			if (dsrValue & DCU_DSR_DBP0) {
				//DEBUG_INFO("COB_DBP0");
				lastCauseOfBreak |= COB_DBP0;

				dcuAssertAsyncHalt(shaveNumber);
				// Note: Unlike instruction breakpoints the status bit is not sticky.
				// Once we clear it, the breakpoint will not re-trigger automatically unless
				// we re-execute the load/store instruction from scratch
				dcuClearBreakCondition(shaveNumber,DCU_DSR_DBP0);
			}

			if (dsrValue & DCU_DSR_DBP1) {
				//DEBUG_INFO("COB_DBP1");
				lastCauseOfBreak |= COB_DBP1;

				dcuAssertAsyncHalt(shaveNumber);
				// Note: Unlike instruction breakpoints the status bit is not sticky.
				// Once we clear it, the breakpoint will not re-trigger automatically unless
				// we re-execute the load/store instruction from scratch
				dcuClearBreakCondition(shaveNumber,DCU_DSR_DBP1);
			}

			if (dsrValue & DCU_DSR_EXT) {
				//DEBUG_INFO("COB_EXT");
				lastCauseOfBreak |= COB_EXT;
			}

			if (dsrValue & DCU_DSR_BT) {
				//DEBUG_INFO("COB_BT");
				lastCauseOfBreak |= COB_BT;
			}

			if (dsrValue & DCU_DSR_SBP) {
				//DEBUG_INFO("SWBP_S1");
				inSWBPStage1 = 1; // Needs to be handled @ some stage
				// If the only cause of break is a SWBP in stage 1 then we can automatically
				// move on one step until we reach the breakpoint
				if (dsrValue == (DCU_DSR_SBP | DCU_DSR_DM)) {
					//DEBUG_INFO("SWBP1->2");
					dcuSoftwareBPMoveToStage2(shaveNumber);
					dcuWaitForHalt(shaveNumber);
					handleAnotherHalt=1;
				}
			}

		}
	} while (handleAnotherHalt);

	return lastCauseOfBreak;
}

u32 dcuSetSoftwareBP(u32 shaveNumber,u32 address)
{

	if (numSWBreakpoints >= MAX_SW_BPS) {
		//DEBUG_INFO("Error: Max SW BPS reached");
		return -1;
	}

	// First read the byte we are going to replace
	breakpointArray[numSWBreakpoints].address     = address;
	breakpointArray[numSWBreakpoints].shaveNumber = shaveNumber;
	breakpointArray[numSWBreakpoints].value       = getShaveMemByte(shaveNumber,address);
	numSWBreakpoints++;

	// Then overwrite the value with a SWBP instruction
	setShaveMemByte(shaveNumber,address,SW_BP_INST);

	return numSWBreakpoints;
}

u32 dcuClearSoftwareBP(u32 shaveNumber,u32 address)
{
	u32 bpNum=0;
	u32 bFound=0;

	for (bpNum=0;bpNum<numSWBreakpoints;bpNum++) {
		if ( (address     == breakpointArray[bpNum].address    ) &&
			 (shaveNumber == breakpointArray[bpNum].shaveNumber)    ) {
			// Found the breakpoint, clear it in memory
			setShaveMemByte(shaveNumber,address,breakpointArray[bpNum].value);
			bFound=1;
		}
	}

	if (!bFound) {
		//DEBUG_INFO("Error: No SW BP @ Address:");
		//ACTUAL_DATA(address);
		return -1;
	}

   return 0;
}

u32 dcuClearAllSoftwareBPs(u32 shaveNumber)
{
	u32 bpNum=0;

	for (bpNum=0;bpNum<numSWBreakpoints;bpNum++) {
		if ( (shaveNumber == breakpointArray[bpNum].shaveNumber)    ) {
			// Found a breakpoint, clear it in memory
			setShaveMemByte(shaveNumber,breakpointArray[bpNum].address,breakpointArray[bpNum].value);
		}
	}

   return 0;
}

// Dummy function just to keep a consistent naming style without breaking Alin's tests.
void dcuSetInstructionBP(u32 shaveNumber, u32 breakAddress1, u32 breakAddress2, u32 bpNumber)
{
	SetInstBreak(shaveNumber,breakAddress1,breakAddress2,bpNumber);
}

// Dummy function just to keep a consistent naming style without breaking Alin's tests.
void dcuClearInstructionBP(u32 shaveNumber, u32 breakpointNumber)
{
	DisableInstBreak(shaveNumber,breakpointNumber);
}

void dcuClearDataBP(u32 shaveNumber, u32 breakpointNumber)
{
	if(breakpointNumber == DBP_BP0)
    {
        //BP 0
		SET_REG_WORD(DCU_DBC0(shaveNumber), 0x00000000);
    }
	else if(breakpointNumber == DBP_BP1)
    {
        // BP 1
		SET_REG_WORD(DCU_DBC1(shaveNumber), 0x00000000);
    }
	else if ((breakpointNumber == DBP_INCLUSIVE_BP) || (breakpointNumber == DBP_EXCLUSIVE_BP))
    {
        // Interval BP
        SET_REG_WORD(DCU_DBC0(shaveNumber), 0x00000000);
        SET_REG_WORD(DCU_DBC1(shaveNumber), 0x00000000);
    }
    else
    {
        ;//DEBUG_INFO("ERROR: Invalid DBP number");
    }

	return;
}

// This function moves a software breakpoint from stage 1 to stage 2
// It first finds the byte which needs to be replaced and then asserts
// async halt and replaces the byte. This should restart execution for
// One instruction
void dcuSoftwareBPMoveToStage2(u32 shaveNumber)
{
	u32 bpNum=0;
	int bFound=0;
	u32 addressOfSWBP;
	int numWriteAttempts=0;
	u32 currentPC;

    dcuAssertAsyncHalt(0);

	addressOfSWBP = GET_REG_WORD_VAL(DCU_SVU_INEXT(shaveNumber));

	for (bpNum=0;bpNum<numSWBreakpoints;bpNum++) {
		if ( (addressOfSWBP  == breakpointArray[bpNum].address    ) &&
			 (shaveNumber    == breakpointArray[bpNum].shaveNumber)    ) {
			// Found the breakpoint, write to the head byte

			// Writing the DBRK byte may have to be done multiple times in the event of a stall
			do {
				numWriteAttempts++;
				SetSVU_DBRK_BYTE(shaveNumber,breakpointArray[bpNum].value );
				while ((GET_REG_WORD_VAL(DCU_DCR(shaveNumber))&1)==0)
				{
					// TODO: Add timeout
				}
				currentPC= GET_REG_WORD_VAL(DCU_SVU_PTR(shaveNumber));
			} while (currentPC != addressOfSWBP); // TODO: Add timeout

			if (numWriteAttempts>1) {
				//DEBUG_INFO("WARNING: Wrote DBRK BYTE more than once:");
				;//ACTUAL_DATA(numWriteAttempts);
			}
			// We should now move on one instruction and stop due to Async halt
			bFound=1;
		}
	}

	if (!bFound) {
		//DEBUG_INFO("Error: Unable to find SWBP");
	}

	return;
}

void dcuDumpIDCFifoContents(u32 shaveNumber)
{
	int i;
	u32 readPointer;
	u32 writePointer;
	u32 byteOffset;
	u32 readWritePointer;

	readWritePointer = GET_REG_WORD_VAL(DCU_SVU_RWPTR(shaveNumber));

	readPointer  =  readWritePointer        & 0x1F;
	writePointer = (readWritePointer >>  8) & 0x1F;
	byteOffset   = (readWritePointer >> 16) & 0x07;

	/*DEBUG_INFO("SVU_RWPTR=");
	ACTUAL_DATA(readWritePointer);
	ACTUAL_DATA(readPointer);
	ACTUAL_DATA(writePointer);
	ACTUAL_DATA(byteOffset);

	DEBUG_INFO("SVU_IDCA0-15=");*/
	// Dump the address registers
	for (i=0;i<16;i++) {
		printf("%08X\n",(GET_REG_WORD_VAL(DCU_SVU_IDCA(shaveNumber,i))));
	}

	return;
}

int dcuIsAddressInIDCFifo(u32 shaveNumber,u32 address)
{
	u32 readPointer;
	u32 writePointer;
	u32 byteOffset;
	u32 readWritePointer;
	u32 fifoLevel;
	u32 readIndex;
	u32 addrInFifo;

	readWritePointer = GET_REG_WORD_VAL(DCU_SVU_RWPTR(shaveNumber));

	readPointer  =  readWritePointer        & 0x1F;
	writePointer = (readWritePointer >>  8) & 0x1F;
	byteOffset   = (readWritePointer >> 16) & 0x07;

	fifoLevel = (writePointer << 3) - ((readPointer << 3) + byteOffset);

	//DEBUG_INFO("FIFO_LVL=");
	//ACTUAL_DATA(fifoLevel);

	if (fifoLevel == 0) {
		//DEBUG_INFO("IDC Fifo Empty");
		return 0;
	}

	// Otherwise we check every address in the fifo
	do {
		readIndex = (readPointer & 0xF);

		// Note: The lower 3 bits of the address in the fifo relate to branching..
		// TODO: Get brendan to explain exactly wha they do
		addrInFifo = GET_REG_WORD_VAL(DCU_SVU_IDCA(shaveNumber,readIndex)) & IDC_FIFO_ADDR_MASK;

		if ((address >=  addrInFifo   ) &&
			(address <= (addrInFifo+7))     ) {
			//DEBUG_INFO("FOUND ADDR IN IDC FIFO:");
			//ACTUAL_DATA(address);
			;//ACTUAL_DATA(GET_REG_WORD_VAL(DCU_SVU_IDCA(shaveNumber,readIndex))); // We output the raw value out of interest
//			return 1;
		}
		else
		{
			//DEBUG_INFO("Not found:");
			;//ACTUAL_DATA(addrInFifo);
		}

		readPointer++;

		if (readPointer == 32)  // 32 is intended, we keep track of level using module math
			readPointer = 0;

	} while (readPointer != writePointer);
	return 0;
}

void dcuDumpInstructionHistory(u32 shaveNumber)
{
	int i;
	//DEBUG_INFO("SVU_IH0-15=");
	// Dump the address registers
	for (i=0;i<16;i++) {
		printf("%08X\n",(GET_REG_WORD_VAL(DCU_SVU_IH(shaveNumber,i))));
	}

	return;
}

// These functions provide the official standard way of checking the PC and INEXT
u32 dcuGetPCVal(u32 shaveNumber)
{
	// Note: PC should be read from SVU_PTR rather than the equivalent TRF register
	return GET_REG_WORD_VAL(DCU_SVU_PTR(shaveNumber));
}

u32 dcuGetINEXT(u32 shaveNumber)
{
    return GET_REG_WORD_VAL(DCU_SVU_INEXT(shaveNumber));
}

//set DBRK_BYTE
void SetSVU_DBRK_BYTE(u32 shaveNumber, u32 dbrkByte)
{
	//DEBUG_INFO("W:DBRK_BYTE");
	SET_REG_WORD(DCU_SVU_DBRK_BYTE(shaveNumber), dbrkByte);
}

//Set a data read hardware breakpoint at a specific address
void SetDRBreak(u32 shaveNumber, u32 breakAddress)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	//Try to see if first data breakpoint is free
	u32 dbc0Value = GET_REG_WORD_VAL(DCU_DBC0(shaveNumber));
	if((dbc0Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA0(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC0(shaveNumber), 0x0000000B);
		return;
	}
	//Try to see if second data breakpoint is free
	u32 dbc1Value = GET_REG_WORD_VAL(DCU_DBC1(shaveNumber));
	if((dbc1Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA1(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC1(shaveNumber), 0x0000000B);
		return;
	}
	//No breakpoint is free, display an error message
	//DEBUG_INFO("ERROR: Data read breakpoint could not be set");
}
//Set a data write hardware breakpoint at a specific address
void SetDWBreak(u32 shaveNumber, u32 breakAddress)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	//Try to see if first data breakpoint is free
	u32 dbc0Value = GET_REG_WORD_VAL(DCU_DBC0(shaveNumber));
	if((dbc0Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA0(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC0(shaveNumber), 0x00000013);
		return;
	}
	//Try to see if second data breakpoint is free
	u32 dbc1Value = GET_REG_WORD_VAL(DCU_DBC1(shaveNumber));
	if((dbc1Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA1(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC1(shaveNumber), 0x00000013);
		return;
	}
	//No breakpoint is free, display an error message
	//DEBUG_INFO("ERROR: Data write breakpoint could not be set");
}
//Set a data access hardware breakpoint at a specific address
void SetDABreak(u32 shaveNumber, u32 breakAddress)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	//Try to see if first data breakpoint is free
	u32 dbc0Value = GET_REG_WORD_VAL(DCU_DBC0(shaveNumber));
	if((dbc0Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA0(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC0(shaveNumber), 0x00000003);
		return;
	}
	//Try to see if second data breakpoint is free
	u32 dbc1Value = GET_REG_WORD_VAL(DCU_DBC1(shaveNumber));
	if((dbc1Value & 0x1) == 0)
	{
		//Breakpoint is free, insert here the new one
		SET_REG_WORD(DCU_DBA1(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBC1(shaveNumber), 0x00000003);
		return;
	}
	//No breakpoint is free, display an error message
	//DEBUG_INFO("ERROR: Data write breakpoint could not be set");
}
//Set a data read hardware breakpoint at a specific address
void SetDBreak(u32 shaveNumber, u32 breakNumber, u32 breakAddress, u32 data, u32 ctrl)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	//If the first breakpoint is specified
	if(breakNumber == 0)
	{
		//Set registers for the breakpoint
		SET_REG_WORD(DCU_DBA0(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBD0(shaveNumber), data);
		SET_REG_WORD(DCU_DBC0(shaveNumber), ctrl);
		return;
	}
	//If the second breakpoint is specified
	if(breakNumber == 1)
	{
		//Set registers for the breakpoint
		SET_REG_WORD(DCU_DBA1(shaveNumber), breakAddress);
		SET_REG_WORD(DCU_DBD1(shaveNumber), data);
		SET_REG_WORD(DCU_DBC1(shaveNumber), ctrl);
		return;
	}//No breakpoint is free, display an error message
	//DEBUG_INFO("ERROR: Data read breakpoint could not be set");
}

//Set an instruction hardware breakpoint at a specific address
void SetInstBreak(u32 shaveNumber, u32 breakAddress1, u32 breakAddress2, u32 bpNumber)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	if (bpNumber == IBP_BP0)
    {
		//Set Breakpoint 0
		SET_REG_WORD(DCU_IBA0(shaveNumber), breakAddress1);
		SET_REG_WORD(DCU_IBC0(shaveNumber), 0x00000001);
	}
    else if (bpNumber == IBP_BP1)
    {
		//Set Breakpoint 1
		SET_REG_WORD(DCU_IBA1(shaveNumber), breakAddress1);
		SET_REG_WORD(DCU_IBC1(shaveNumber), 0x00000001);
	}
    else if (bpNumber == IBP_INCLUSIVE_BP)
    {
        //Set inclusive interval breakpoint
        SET_REG_WORD(DCU_IBA0(shaveNumber), breakAddress1);
		SET_REG_WORD(DCU_IBA1(shaveNumber), breakAddress2);
		SET_REG_WORD(DCU_IBC0(shaveNumber), 0x00000401);
        SET_REG_WORD(DCU_IBC1(shaveNumber), 0x00000401);
    }
    else if (bpNumber == IBP_EXCLUSIVE_BP)
    {
        //Set exclusive interval breakpoint
        SET_REG_WORD(DCU_IBA0(shaveNumber), breakAddress1);
        SET_REG_WORD(DCU_IBA1(shaveNumber), breakAddress2);
		SET_REG_WORD(DCU_IBC0(shaveNumber), 0x00000801);
        SET_REG_WORD(DCU_IBC1(shaveNumber), 0x00000801);
    }
    else
    {
	    //No breakpoint is free, display an error message
	    //DEBUG_INFO("ERROR: Instruction breakpoint could not be set");
    }
}
//Disable an instruction breakpoint for a specified shave
void DisableInstBreak(u32 shaveNumber, u32 breakpointNumber)
{
	if(breakpointNumber == IBP_BP0)
    {
        //BP 0
		lastCauseOfBreak &= ~COB_IBP0; // This stops us from auto-single stepping from the breakpoint
		SET_REG_WORD(DCU_IBC0(shaveNumber), 0x00000000);
    }
	else if(breakpointNumber == IBP_BP1)
    {
        // BP 1
		lastCauseOfBreak &= ~COB_IBP1; // This stops us from auto-single stepping from the breakpoint
		SET_REG_WORD(DCU_IBC1(shaveNumber), 0x00000000);
    }
	else if ((breakpointNumber == IBP_INCLUSIVE_BP) || (breakpointNumber == IBP_EXCLUSIVE_BP))
    {
        // Interval BP
		lastCauseOfBreak &= ~COB_IBP0; // This stops us from auto-single stepping from the breakpoint
        SET_REG_WORD(DCU_IBC0(shaveNumber), 0x00000000);
        SET_REG_WORD(DCU_IBC1(shaveNumber), 0x00000000);
    }
    else
    {
        ;//DEBUG_INFO("ERROR: Invalid breakpoint number");
    }
}

//Continue after breakpoint is hit
void ContinueSHAVE(u32 shaveNumber)
{
	//
	SET_REG_WORD(DCU_DSR(shaveNumber), 0x00000000);
}

// Display IP and I_NEXT
void OutputIP(u32 shaveNumber)
{
    u32 temp;

	// Note: PC should be read from SVU_PTR rather than the equivalent TRF register
    temp = GET_REG_WORD_VAL(DCU_SVU_PTR(shaveNumber));
    //DEBUG_INFO("SVU_PTR  = ");
    //ACTUAL_DATA(temp);
    temp = GET_REG_WORD_VAL(DCU_SVU_INEXT(shaveNumber));
    //DEBUG_INFO("I_NEXT  = ");
    //ACTUAL_DATA(temp);
}

//Set a software break
void SetSBreak(u32 shaveNumber, u32 breakAddress)
{
	//Make sure that DCR[0] = 1 (Debug enable)
	u32 dcrValue = GET_REG_WORD_VAL(DCU_DCR(shaveNumber));
	SET_BITS(DCU_DCR(shaveNumber), dcrValue, DCR_DBGE);
	//set SWBR
	SET_REG_BYTE(SVU_BASE_ADDR[shaveNumber] + (breakAddress & 0x00FFFFFF), 0xF9);
}

//Disable a data breakpoint for a specified shave
void RemoveDBreak(u32 shaveNumber, u32 breakpointNumber)
{
	if(breakpointNumber == 0)
		SET_REG_WORD(DCU_DBC0(shaveNumber), 0x00000000);
	else if(breakpointNumber == 1)
		SET_REG_WORD(DCU_DBC1(shaveNumber), 0x00000000);
	else
    {
		;//DEBUG_INFO("ERROR: Invalid breakpoint number");
    }
}
