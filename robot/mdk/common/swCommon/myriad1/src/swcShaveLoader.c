/**
 * This is simple module to manage shave loading and windowed addresses
 * 
 * @File                                        
 * @Author    Cormac Brick                     
 * @Brief     Simple Shave loader and window manager    
 * @copyright All code copyright Movidius Ltd 2012, all rights reserved 
 *            For License Warranty see: common/license.txt   
 * 
 * @TODO(Cormac):  Some svu support stuff is duplicated here which should find a new home
 */

#include "swcShaveLoader.h"
#include "swcShaveLoaderPrivate.h"
#include "swcMemoryTransfer.h"
#include <swcLeonUtils.h>
#include "isaac_registers.h" 
#include "DrvSvu.h"
#include "assert.h" 
#include "mv_types.h"
#include <DrvIcb.h>
#include <stdarg.h>

// declare a set of const pointers to linker defined symbols
extern void*  const __WinRegShave0_winC, __WinRegShave0_winD, __WinRegShave0_winE, __WinRegShave0_winF;
extern void*  const __WinRegShave1_winC, __WinRegShave1_winD, __WinRegShave1_winE, __WinRegShave1_winF;
extern void*  const __WinRegShave2_winC, __WinRegShave2_winD, __WinRegShave2_winE, __WinRegShave2_winF;
extern void*  const __WinRegShave3_winC, __WinRegShave3_winD, __WinRegShave3_winE, __WinRegShave3_winF;
extern void*  const __WinRegShave4_winC, __WinRegShave4_winD, __WinRegShave4_winE, __WinRegShave4_winF;
extern void*  const __WinRegShave5_winC, __WinRegShave5_winD, __WinRegShave5_winE, __WinRegShave5_winF;
extern void*  const __WinRegShave6_winC, __WinRegShave6_winD, __WinRegShave6_winE, __WinRegShave6_winF;
extern void*  const __WinRegShave7_winC, __WinRegShave7_winD, __WinRegShave7_winE, __WinRegShave7_winF;

// intialise arrays that will contain the correct window pointers.  
// The linker will locate these arrays where window registers really are in CMX address space
u32 __wins_shave0[4] __attribute__((section(".winregs.S0")))  = {(u32) &__WinRegShave0_winC, (u32) &__WinRegShave0_winD, (u32) &__WinRegShave0_winE, (u32) &__WinRegShave0_winF};
u32 __wins_shave1[4] __attribute__((section(".winregs.S1")))  = {(u32) &__WinRegShave1_winC, (u32) &__WinRegShave1_winD, (u32) &__WinRegShave1_winE, (u32) &__WinRegShave1_winF};
u32 __wins_shave2[4] __attribute__((section(".winregs.S2")))  = {(u32) &__WinRegShave2_winC, (u32) &__WinRegShave2_winD, (u32) &__WinRegShave2_winE, (u32) &__WinRegShave2_winF};
u32 __wins_shave3[4] __attribute__((section(".winregs.S3")))  = {(u32) &__WinRegShave3_winC, (u32) &__WinRegShave3_winD, (u32) &__WinRegShave3_winE, (u32) &__WinRegShave3_winF};
u32 __wins_shave4[4] __attribute__((section(".winregs.S4")))  = {(u32) &__WinRegShave4_winC, (u32) &__WinRegShave4_winD, (u32) &__WinRegShave4_winE, (u32) &__WinRegShave4_winF};
u32 __wins_shave5[4] __attribute__((section(".winregs.S5")))  = {(u32) &__WinRegShave5_winC, (u32) &__WinRegShave5_winD, (u32) &__WinRegShave5_winE, (u32) &__WinRegShave5_winF};
u32 __wins_shave6[4] __attribute__((section(".winregs.S6")))  = {(u32) &__WinRegShave6_winC, (u32) &__WinRegShave6_winD, (u32) &__WinRegShave6_winE, (u32) &__WinRegShave6_winF};
u32 __wins_shave7[4] __attribute__((section(".winregs.S7")))  = {(u32) &__WinRegShave7_winC, (u32) &__WinRegShave7_winD, (u32) &__WinRegShave7_winE, (u32) &__WinRegShave7_winF};

// Define an array of pointers to window registers
// this is for programmer convenience when writing code to iterate over shaves
u32* const windowRegs[8] = {
        ((u32* const) __wins_shave0),
        ((u32* const) __wins_shave1),
        ((u32* const) __wins_shave2),
        ((u32* const) __wins_shave3),
        ((u32* const) __wins_shave4),
        ((u32* const) __wins_shave5),
        ((u32* const) __wins_shave6),
        ((u32* const) __wins_shave7)
};

// Keep local copies of section headers... 
// @TODO: should these just be assigned on the stack instead of globally
static tMofFileHeader    swc_mbinH;
static tMofSectionHeader swc_secH;


u32* swcGetWinRegsShave(u32 shave_num)
{
    assert(shave_num < 8);
    return (windowRegs[shave_num]);
}

void swcSetWindowedDefaultStack(u32 shave_num)
{
    u32* win_x;
    u32 ram_code_stack = 0;
    u32 stack_pointer = 0;
    u32 addr_next_shave = 0;

    //Compute the default stack pointer based on default window configuration
    //This code takes into account how much bytes were allocated to the data window and sets the stack
    //to the maximum allowed data code.
    if (shave_num <= 7){
        //read window register array from the window variable
        win_x = swcGetWinRegsShave(shave_num);
        //Compute the address of the next shave as being
        //128Kb and the current shave data (first) window. Zeroing lower bits for window alignment
        addr_next_shave = ((*win_x + 0x1ffff) & (~0x1ffff));
        //Compute the stack as being the next shave's start address - the current shave data window
        ram_code_stack = addr_next_shave - *win_x;
        //Or with 0x1Cxxxxxx since this was lost in the previous subtraction
        stack_pointer = 0x1c000000 | ram_code_stack;
    }
    //Finally set the default stack
    DrvSvutIrfWrite(shave_num, 19, stack_pointer);
}

void swcSetAbsoluteDefaultStack(u32 shave_num)
{
    u32* win_x;
    u32 ram_code_stack = 0;
    u32 stack_pointer = 0;
    u32 addr_next_shave = 0;

    //Compute the default stack pointer based on default window configuration
    //This code takes into account how much bytes were allocated to the data window and sets the stack
    //to the maximum allowed data code.
    if (shave_num <= 7){
        //stack pointer set as the end of the current shave's slice
        stack_pointer = windowRegs[shave_num][1] + (128 * 1024);
    }
    //Finally set the default stack
    DrvSvutIrfWrite(shave_num, 19, stack_pointer);
}

void swcSetShaveWindow(u32 shave_num, u32 window_num, u32 window_addr)
{
    assert(shave_num < 8);
    assert(window_num < 4);

    (windowRegs[shave_num])[window_num] = window_addr;
}

void swcSetShaveWindows(u32 shaveNumber, u32 windowA, u32 windowB,
        u32 windowC, u32 windowD)
{
    //Calculate address of the WindowA register
    u32 address = SHAVE_0_BASE_ADR + SVU_SLICE_OFFSET * shaveNumber;
    //Set each register
    SET_REG_WORD(address + SLC_TOP_OFFSET_WIN_A, windowA);
    SET_REG_WORD(address + SLC_TOP_OFFSET_WIN_B, windowB);
    SET_REG_WORD(address + SLC_TOP_OFFSET_WIN_C, windowC);
    SET_REG_WORD(address + SLC_TOP_OFFSET_WIN_D, windowD);
    return;
}

void swcSetShaveWindowsToDefault(u32 shaveNumber)
{
    swcSetShaveWindows(shaveNumber, windowRegs[shaveNumber][0], windowRegs[shaveNumber][1],
            windowRegs[shaveNumber][2], windowRegs[shaveNumber][3]);
}

u32 swcShaveRunning(u32 svu)
{
    //If stopped, return 0
    if (((GET_REG_WORD_VAL(SVU_CTRL_ADDR[svu] + SLC_OFFSET_SVU+SVU_OCR)) & 0x4) || DrvSvuIsStopped(svu))
        return 0;

    //else, it's running
    return 1;
}

// Start shave shave_nr from entry_point
void swcRunShave(u32 shave_nr, u32 entry_point)
{
    // Run the program. Note the first 2 steps are optional
    // Set STOP bit in control register
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_OCR, OCR_STOP_GO | OCR_IDC_128);
    // Clear any interrupts from previous test
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_IRR, 0xFFFFFFFF);
    // Enable SWI interrupt
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_ICR, 0x20);

    //Start Shave
    DrvSvuStart(shave_nr, entry_point);

    //Wait for completion (wait for program to halt)
    while (!DrvSvuIsStopped(shave_nr))
        NOP;
}

void swcSetRounding(u32 shave_no, u32 roundingBits)
{

    SET_REG_WORD(DCU_SVU_TRF(shave_no, P_CFG), roundingBits); // Float rounding mode
    return;
}

void swcStartShave(u32 shave_nr, u32 entry_point)
{
    // Run the program. Note the first 2 steps are optional
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_OCR, OCR_STOP_GO | OCR_IDC_128);       // Set STOP bit in control register
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_IRR, 0xFFFFFFFF); // Clear any interrupts from previous test
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_ICR, 0x20);       // Enable SWI interrupt

    //Start Shave
    DrvSvuStart(shave_nr, entry_point);
}

void swcStartShaveAsync(u32 shave_nr, u32 entry_point, irq_handler function)
{
    //Clear all interrupts so we can have the next interrupt happening
    SET_REG_WORD(DCU_IRR(shave_nr), 0xFFFFFFFF);
    //Disable ICB (Interrupt Control Block) while setting new interrupt
    DrvIcbDisableIrq(IRQ_SVE_0 + shave_nr);
    DrvIcbIrqClear(IRQ_SVE_0 + shave_nr);
    //Configure Leon to accept traps on any level
    swcLeonSetPIL(0);
    //Configure interrupt handlers
    DrvIcbSetIrqHandler(IRQ_SVE_0 + shave_nr, function);
    //Enable interrupts on SHAVE done
    DrvIcbConfigureIrq(IRQ_SVE_0 + shave_nr, SHAVE_INTERRUPT_LEVEL, POS_EDGE_INT);
    DrvIcbEnableIrq(IRQ_SVE_0 + shave_nr);
    //Enable SWIH IRQ sources
    SET_REG_WORD(DCU_ICR(shave_nr), ICR_SWI_ENABLE);
    //Can enable the interrupt now
    swcLeonEnableTraps();
    // Run the program. Note the first 2 steps are optional
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_OCR, OCR_STOP_GO | OCR_IDC_128);       // Set STOP bit in control register
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_IRR, 0xFFFFFFFF); // Clear any interrupts from previous test
    SET_REG_WORD(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_ICR, 0x20);       // Enable SWI interrupt

    //Start Shave
    DrvSvuStart(shave_nr, entry_point);
}

void swcSetRegsCC(u32 shave_num, const char *fmt, va_list a_list)
{
    u32 x, *v;
    int i = 0;
    int vrf = 23,irf = 18, srf = 23;
    while (fmt[i] != '\0')
    {

        if ((fmt[i] == 'i') || (fmt[i] == 'I'))
        {

            assert(irf != 10 && "Number of irf registers more than 8");
            x = va_arg(a_list, u32);
            DrvSvutIrfWrite(shave_num, irf, x);

            irf--;
        }

        if ((fmt[i] == 's') || (fmt[i] == 'S'))
        {

            assert(srf != 15 && "Number of srf registers more than 8");
            x = va_arg(a_list, u32);
            DrvSvutSrfWrite(shave_num, srf, x);

            srf--;
        }

        if ((fmt[i] == 'v') || (fmt[i] == 'V'))
        {
            assert(vrf != 15 && "Number of vrf registers more than 8");
            v = (u32 *)va_arg(a_list, u32);
            assert(v != NULL && "Number of vrf registers more than 8");
            DrvSvutVrfWrite(shave_num, vrf, 0, * (v + 0));
            DrvSvutVrfWrite(shave_num, vrf, 1, * (v + 1));
            DrvSvutVrfWrite(shave_num, vrf, 2, * (v + 2));
            DrvSvutVrfWrite(shave_num, vrf, 3, * (v + 3));
            vrf--;
        }

        i++;
    }
    return;
}

void swcStartShaveCC(u32 shave_num, u32 pc, const char *fmt, ...)
{
    va_list a_list;
    va_start(a_list, fmt);

    swcSetRegsCC(shave_num, fmt, a_list);

    va_end(a_list);
    swcStartShave(shave_num, (u32)pc);

}

void swcStartShaveAsyncCC(u32 shave_num, u32 pc, irq_handler function, const char *fmt, ...)
{
    va_list a_list;
    va_start(a_list, fmt);

    swcSetRegsCC(shave_num, fmt, a_list);

    va_end(a_list);
    swcStartShaveAsync(shave_num, (u32)pc, function);

}

void swcSetupShaveCC(u32 shave_num, const char *fmt, ...)
{
    va_list a_list;
    va_start(a_list, fmt);

    swcSetRegsCC(shave_num, fmt, a_list);

    va_end(a_list);

}

void swcResetShave(u32 shaveNumber)
{
    // Set STOP and 128_IDC bits in OCR to ensure application doesn't restart when we reset
    SET_REG_WORD(DCU_OCR(shaveNumber), OCR_STOP_GO | OCR_IDC_128);       // Set STOP bit in control register

    // Now we try to run it again after a reset
    DrvSvuSliceResetAll(shaveNumber); // reset every single bit
}

void swcResetShaveLite(u32 shaveNumber)
{
    // Set STOP and 128_IDC bits in OCR to ensure application doesn't restart when we reset
    SET_REG_WORD(SVU_CTRL_ADDR[shaveNumber] + SLC_OFFSET_SVU + SVU_OCR, 0x24);       // Set STOP bit in control register
    SET_REG_WORD(DCU_OCR(shaveNumber), GET_REG_WORD_VAL(DCU_OCR(shaveNumber)) & ~(1 << 5));          // Set instruction to 64 bit

    // Now we try to run it again after a reset
    DrvSvuSliceResetOnlySvu(shaveNumber); // Try just resetting the shave

    // First we give it a chance with these writes
    // Configure all CMX RAM layouts to config #6
    SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, 0x66666666 );

    //Configure Window registers
    swcSetShaveWindowsToDefault(shaveNumber);
}

//Wait for shaves form shave_list to finish execution; number of shaves within the list is given by no_of_shaves

void swcWaitShaves(u32 no_of_shaves, swcShaveUnit_t *shave_list)
{
    u32 svu;
    int done = 0;

    do
    {
        done = 1; // assumes all shaves are done
        for (svu = 0; svu < no_of_shaves; svu++)
        {
            // printf("checking shave %d\n", shave_list[svu]);
            if (!DrvSvuIsStopped(shave_list[svu])) // wait for shave to halt
            {
                done = 0;
            }
        }
    }while(!done);
}

void swcWaitShave(u32 shave_nr)
{
    //if STOPPED, return
    if ((GET_REG_WORD_VAL(SVU_CTRL_ADDR[shave_nr] + SLC_OFFSET_SVU + SVU_OCR)) & 0x4)
    {
        return;
    }

    //else, wait for HALT
    while (!DrvSvuIsStopped(shave_nr)) // Wait for program to halt
    {
        NOP;
    }
} 

u32 swcShavesRunning(u32 first, u32 last)
{
    u32 svu;

    for (svu = first; svu <= last; svu++)
    {
        if (swcShaveRunning(svu))
        {
            return 1;
        }
    }

    return 0;
}

u32 swcShavesRunningArr(u32 arr[], u32 N)
{
    u32 si;

    for (si = 0; si < N; si++)
    {
        if (swcShaveRunning(arr[si]))
        {
            //at least 1 shave from the group is running, so it's not over !
            return 1;
        }
    }

    return 0;
}

/// Function that translates Leon preferred CMX addresses to shave preferred ones
/// @param[in] LeonAddr Address to translate
/// @return - translated address
u32 swcTranslateLeonToShaveAddr(u32 LeonAddr)
{
    return ((LeonAddr & 0x0FFFFFFF) | 0x10000000);
}
/// Function that translates Shave preferred CMX addresses to Leon preferred ones
/// @param[in] ShaveAddr Address to translate
/// @return - translated address
u32 swcTranslateShaveToLeonAddr(u32 ShaveAddr)
{
    return ((ShaveAddr & 0x0FFFFFFF) | 0xA0000000);
}


//#######################################################################
// Converts a Shave -Relative address to a Systeme solved address (in 0x10000000 view), based on the [Target CMX Slice]
//              and current widnow it relates to
// Inputs : inAddr :shave relative address (can be code/data/absolute tyep of address)
// Return : the resolved addr 
//#######################################################################
u32 swcSolveShaveRelAddr(u32 inAddr, u32 targetS)
{
    u32 resolved;
    switch (inAddr >> 24)
    {
    case 0x1C: resolved = (inAddr & 0x00FFFFFF) + (windowRegs[targetS])[0]; break;
    case 0x1D: resolved = (inAddr & 0x00FFFFFF) + (windowRegs[targetS])[1]; break;
    case 0x1E: resolved = (inAddr & 0x00FFFFFF) + (windowRegs[targetS])[2]; break;
    case 0x1F: resolved = (inAddr & 0x00FFFFFF) + (windowRegs[targetS])[3]; break;
    default  : resolved =  inAddr;  break; //absolute address, no translation is to be done
    }

    return resolved;
}

u32 swcSolveShaveRelAddrAHB(u32 inAddr, u32 targetS)
{
    u32 resolved;
    switch (inAddr >> 24)
    {
    case 0x1C: resolved = (((inAddr & 0x00FFFFFF) + (windowRegs[targetS])[0]) & 0x0FFFFFFF) | 0xA0000000; break;
    case 0x1D: resolved = (((inAddr & 0x00FFFFFF) + (windowRegs[targetS])[1]) & 0x0FFFFFFF) | 0xA0000000; break;
    case 0x1E: resolved = (((inAddr & 0x00FFFFFF) + (windowRegs[targetS])[2]) & 0x0FFFFFFF) | 0xA0000000; break;
    case 0x1F: resolved = (((inAddr & 0x00FFFFFF) + (windowRegs[targetS])[3]) & 0x0FFFFFFF) | 0xA0000000; break;
    default  : resolved =  inAddr;  break; //absolute address, no translation is to be done
    }

    return resolved;
} 


void swcLoadMbin(u8 *sAddr, u32 targetS)
{
    int sec;
    u32 srcA32;
    u32 dstA32;

    //get the program header
    swcU32memcpy((u32*)&swc_mbinH,     (u32*)sAddr, sizeof(tMofFileHeader));
    //dma_memcpy((u32*)&swc_mbinH,     (u32*)sAddr, sizeof(tMofFileHeader));

    //parse sections:
    for (sec = 0; sec < swc_mbinH.shCount; sec++)
    {
        //get current section
        swcU32memcpy((u32*)&swc_secH, ((u32*)sAddr) + ((swc_mbinH.shOffset + sec * swc_mbinH.shEntrySize) >> 2), sizeof(tMofSectionHeader));
        //dma_memcpy((u32*)&swc_secH, ((u32*)sAddr) + ((swc_mbinH.shOffset + sec * swc_mbinH.shEntrySize) >> 2), sizeof(tMofSectionHeader));

        //if this is a BSS type of section, just continue
        if (swc_secH.type == MSHT_NOBITS)
            continue;
        srcA32   = (u32)(sAddr + swc_secH.offset);
        dstA32   = swcSolveShaveRelAddr(swc_secH.address, targetS);

        swcU32memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
        //dma_memcpy((u32*)dstA32, (u32*)srcA32, swc_secH.size);
    }
    //     printf("Point %s on line %d\n",__FILE__,__LINE__);
}

/// Dynamically load shvdlib file - These are elf object files stripped of any symbols
///@param sAddr starting address where to load the shvdlib file
///@param targetS the target Shave
void swcLoadshvdlib(u8 *sAddr, u32 targetS)
{
    Elf32Ehdr ElfHeader;
    u32 SecHeaders, i;
    u32 phAddr;
    u32 *srcAddr;
    u32 *dstAddr;
    u32 SecSize;

    //get the section header
    swcU32memcpy((u32*)&ElfHeader, (u32*)sAddr, sizeof(Elf32Ehdr));

    //Reading section headers table offset
    phAddr = (u32)sAddr + ElfHeader.eShoff;

    //parse section headers:
    for (SecHeaders = 0; SecHeaders < ElfHeader.eShnum; SecHeaders++)
    {
        Elf32Section ElfSecHeader;
        u32 SecOffset;
        u32 SecDataOffset;
        SecOffset = phAddr + sizeof(Elf32Section) * SecHeaders;
        swcU32memcpy((u32*)&ElfSecHeader, (u32*)SecOffset, sizeof(Elf32Section));
        SecDataOffset = (u32)sAddr + ElfSecHeader.shOffset;

        srcAddr = (u32*)SecDataOffset;
        dstAddr = (u32*)swcSolveShaveRelAddrAHB(ElfSecHeader.shAddr, targetS);
        SecSize = ElfSecHeader.shSize;
        //swcU32memcpy((u32*)dstAddr, (u32*)srcAddr, SecSize);

        //Only load PROGBITS sections
        if (ElfSecHeader.shType == SHT_PROGBITS){

            // Assert aligned addresses
            assert((SecSize  & 0x3) == 0);
            assert(((u32)dstAddr  & 0x3) == 0);
            assert(((u32)srcAddr  & 0x3) == 0);

            SecSize = SecSize >> 2;
            for (i = 0; i < SecSize; i++)
                //Need to reverse endianess on the section data because of loading shave sections
                //moviDebug and moviConvert do the same thing on Elf data
                dstAddr[i] = ((srcAddr[i] & 0xFF) << 24) | ((srcAddr[i] & 0xFF00) << 8) |
                ((srcAddr[i] & 0xFF0000) >> 8) | ((srcAddr[i] & 0xFF000000) >> 24);
        }

    }
}
