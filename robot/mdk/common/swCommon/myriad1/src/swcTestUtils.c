///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Software test library
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "swcTestUtils.h"
#include "isaac_registers.h"
#include "stdio.h"
#include "DrvSvuDefines.h"
#include "DrvSvu.h"
#include "DrvTimer.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
    tyTimeStamp execTime;
tyProcessorType swcGetProcessorType(void)
{
    u32 regValue;
    tyProcessorType processorType ;
    regValue = (GET_REG_WORD_VAL(CPR_RAM_PD_ADR) & 0x000F000) >> 12;

    if ((regValue == (regValue | 0xF)) || (regValue == (regValue | 0xE)))
    {
        processorType = MVI_IC;
    }
    else if ((regValue == (regValue | 0xD)) || (regValue == (regValue | 0xC)))
    {
        processorType = MVI_VCS;
    }
    else if ((regValue == (regValue | 0xB)) || (regValue == (regValue | 0xA)))
    {
        processorType = MVI_FPGA;
    }
    else if ((regValue == (regValue | 0x9)) || (regValue == (regValue | 0x8)))
    {
        processorType = MVI_SSIM;
    }
    else
    {
        processorType = MVI_UNKNOWN;
    }
    return processorType;
}

void swcShaveProfInit(performanceStruct *perfStruct)
{
    //resets the structure's elements

    perfStruct->perfCounterStall = -1;
    perfStruct->perfCounterExec  = -1;
    perfStruct->perfCounterClkCycles  = -1;
    perfStruct->perfCounterBranch = -1;
    perfStruct->perfCounterTimer  = 0;
    perfStruct->countShCodeRun = 0;
}


void swcShaveProfStartGathering(u32 shaveNumber, performanceStruct *perfStruct)
{
    perfStruct->countShCodeRun++;
    switch (perfStruct->countShCodeRun)
    {
    case 1:
        DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_EX_IN_EN);
        DrvSvuEnablePerformanceCounter(shaveNumber, 1, PC_STALL_EN);
        break;
    case 2:
        DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_BR_TAKEN_EN);
        DrvSvuEnablePerformanceCounter(shaveNumber, 1, PC_CLK_CYC_EN);
        break;
    case 3:
        DrvTimerStart(&perfStruct->executionTimer);
        break;
    default:
        break;
    }
    SET_REG_WORD(DCU_PC1(shaveNumber), 0);
    SET_REG_WORD(DCU_PC0(shaveNumber), 0);

}


int swcShaveProfGatheringDone(performanceStruct *perfStruct)
{

    int done = 0;
    if (perfStruct->countShCodeRun == 3)
        done = 1;
    else
        done = 0;

    return done;
}


void swcShaveProfStopGathering(u32 shaveNumber, performanceStruct *perfStruct)
{
    u32 count1, count2;
    count1 = GET_REG_WORD_VAL(DCU_PC0(shaveNumber));
    count2 = GET_REG_WORD_VAL(DCU_PC1(shaveNumber));
    if(perfStruct->countShCodeRun == 1)
    {
        perfStruct->perfCounterStall = count2;
        perfStruct->perfCounterExec = count1;
    }
    if(perfStruct->countShCodeRun == 2)
    {
        perfStruct->perfCounterBranch = DrvSvuGetPerformanceCounter0(shaveNumber);
        perfStruct->perfCounterClkCycles = DrvSvuGetPerformanceCounter1(shaveNumber);
    }
    if(perfStruct->countShCodeRun == 3)
        perfStruct->perfCounterTimer = DrvTimerElapsedTicks(&perfStruct->executionTimer);

}


void swcShaveProfPrint(u32 shaveNumber, performanceStruct *perfStruct)
{
    switch(perfStruct->countShCodeRun)
    {
    case 3:
        printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(perfStruct->perfCounterTimer)
                ,(u32)(DrvTimerTicksToMs(perfStruct->perfCounterTimer)*1000)
                ,(u32)(DrvTimerTicksToMs(perfStruct->perfCounterTimer)));
    case 2:
        printf("\nShave executed clock cycle count: %d\n", perfStruct->perfCounterClkCycles);
        printf("\nNumber of branches taken: %d\n", perfStruct->perfCounterBranch);
    case 1:
        printf("\nShave measured stall cycle count: %d\n", perfStruct->perfCounterStall);
        printf("\nShave measured instruction count: %d\n", perfStruct->perfCounterExec);
    default:{
        if (perfStruct->countShCodeRun>2){
            printf("\nMeasuring over multiple runs. Small disparities are expected.\n");
        }
    }
    }
}

void swcShaveProfStartGatheringFields(u32 shaveNumber, performanceCounterDef perfDefines)
{
    u32 count1;
    performanceCounterDef perfInfoType;
    perfInfoType = perfDefines;
    if (perfInfoType == PERF_INSTRUCT_COUNT)
    {
        DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_EX_IN_EN);
    }
    if (perfInfoType == PERF_STALL_COUNT)
    {
         DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_STALL_EN);
    }
    if (perfInfoType == PERF_CLK_CYCLE_COUNT)
    {
        DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_CLK_CYC_EN);
    }
    if (perfInfoType == PERF_BRANCH_COUNT)
    {
        DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_BR_TAKEN_EN);
    }
    if (perfInfoType == PERF_TIMER_COUNT)
    {
        DrvTimerStart(&execTime);
    }
    SET_REG_WORD(DCU_PC0(shaveNumber), 0);
}

void swcShaveProfStopFieldsGatehering(u32 shaveNumber, performanceCounterDef perfDefines)
{
    u32 count;
    u32 result;
    unsigned long long timeCount;
    int perfGathered = perfDefines;
    count = GET_REG_WORD_VAL(DCU_PC0(shaveNumber));
    if (perfGathered == PERF_STALL_COUNT)
    {
        result = count;
        printf("\nShave measured stall cycle count: %d\n", result);
    }
    if (perfGathered == PERF_INSTRUCT_COUNT)
    {
        result = count;
        printf("\nShave measured instruction count: %d\n", result);
    }
    if (perfGathered == PERF_CLK_CYCLE_COUNT)
    {
        result = count;
        printf("\nShave executed clock cycle count: %d\n", result);
    }
    if (perfGathered == PERF_BRANCH_COUNT)
    {
        result = count;
        printf("\nNumber of branches taken: %d\n", result);
    }
    if (perfGathered == PERF_TIMER_COUNT)
    {
        timeCount = DrvTimerElapsedTicks(&execTime);
        printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(timeCount)
            ,(u32)(DrvTimerTicksToMs(timeCount)*1000)
            ,(u32)(DrvTimerTicksToMs(timeCount)));
    }
}


void swcShaveProfileCyclesStart(u32 shaveNumber)
{

    DrvSvuEnablePerformanceCounter(shaveNumber, 0, PC_EX_IN_EN);
    DrvSvuEnablePerformanceCounter(shaveNumber, 1, PC_STALL_EN);

	DrvTimerStart(&execTime);

    SET_REG_WORD(DCU_PC0(shaveNumber), 0);
    SET_REG_WORD(DCU_PC1(shaveNumber), 0);
}

void swcShaveProfileCyclesStop(u32 shaveNumber)
{
    u32 count0, count1;
    u32 result;
    unsigned long long timeCount;

    count1 = GET_REG_WORD_VAL(DCU_PC1(shaveNumber));
    timeCount = DrvTimerElapsedTicks(&execTime);
    count0 = GET_REG_WORD_VAL(DCU_PC0(shaveNumber));

    printf("\nMeasuring at different offsets: \n");
    printf("\nShave measured instruction count: %d\n", count0);
    printf("\nShave measured stall cycle count: %d\n", count1);
    printf("\nLeon executed %d cycles in %06d micro seconds ([%d ms])\n",(u32)(timeCount)
        ,(u32)(DrvTimerTicksToMs(timeCount)*1000)
        ,(u32)(DrvTimerTicksToMs(timeCount)));
}

