///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief.    This is the implementation of horizontal resize library.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <DrvGpioDefines.h>
#include <DrvCpr.h>
#include <DrvGpio.h>
#include <DrvIcbDefines.h>
#include "HorizontalLineResizeApi.h"
#include <swcShaveLoader.h>
#include "swcFrameTypes.h"
#include <DrvSvu.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//#define USE_ABSOLUTE_ADR -> define in Makefile if needed

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
//NOTE: SVEX_* addresses are actually relative addresses
// so we don't need to export all 4 addresses even if we use 4 shaves, the symbols have the same value
extern unsigned int horiz0_cif_dma_done_adr;
extern unsigned int horiz0_CMXFirstInputLine;
extern unsigned int horiz0_CMXSecondInputLine;
extern unsigned int horiz0_CMXThirdInputLine;
extern unsigned int horiz0_current_cif_buffer_adr;
extern unsigned int horiz0_force_shave_graceful_halt;
extern unsigned int horiz0_OneLineResize;
extern unsigned int horiz0_OneLineResizeSBS;

extern unsigned int horiz1_cif_dma_done_adr;
extern unsigned int horiz1_CMXFirstInputLine;
extern unsigned int horiz1_CMXSecondInputLine;
extern unsigned int horiz1_CMXThirdInputLine;
extern unsigned int horiz1_current_cif_buffer_adr;
extern unsigned int horiz1_force_shave_graceful_halt;
extern unsigned int horiz1_OneLineResize;
extern unsigned int horiz1_OneLineResizeSBS;

extern unsigned int horiz2_cif_dma_done_adr;
extern unsigned int horiz2_CMXFirstInputLine;
extern unsigned int horiz2_CMXSecondInputLine;
extern unsigned int horiz2_CMXThirdInputLine;
extern unsigned int horiz2_current_cif_buffer_adr;
extern unsigned int horiz2_force_shave_graceful_halt;
extern unsigned int horiz2_OneLineResize;
extern unsigned int horiz2_OneLineResizeSBS;

extern unsigned int horiz3_cif_dma_done_adr;
extern unsigned int horiz3_CMXFirstInputLine;
extern unsigned int horiz3_CMXSecondInputLine;
extern unsigned int horiz3_CMXThirdInputLine;
extern unsigned int horiz3_current_cif_buffer_adr;
extern unsigned int horiz3_force_shave_graceful_halt;
extern unsigned int horiz3_OneLineResize;
extern unsigned int horiz3_OneLineResizeSBS;

extern unsigned int horiz6_cif_dma_done_adr;
extern unsigned int horiz6_CMXFirstInputLine;
extern unsigned int horiz6_CMXSecondInputLine;
extern unsigned int horiz6_CMXThirdInputLine;
extern unsigned int horiz6_current_cif_buffer_adr;
extern unsigned int horiz6_force_shave_graceful_halt;
extern unsigned int horiz6_OneLineResize;
extern unsigned int horiz6_OneLineResizeSBS;

unsigned int* horiz_cif_dma_done_adr[HRES_SHAVE_NO]={
		(unsigned int*)&horiz0_cif_dma_done_adr,
		(unsigned int*)&horiz1_cif_dma_done_adr,
		(unsigned int*)&horiz2_cif_dma_done_adr,
		(unsigned int*)&horiz3_cif_dma_done_adr,
		(unsigned int*)&horiz6_cif_dma_done_adr
};
unsigned int* horiz_CMXFirstInputLine[HRES_SHAVE_NO]={
		(unsigned int*)&horiz0_CMXFirstInputLine,
		(unsigned int*)&horiz1_CMXFirstInputLine,
		(unsigned int*)&horiz2_CMXFirstInputLine,
		(unsigned int*)&horiz3_CMXFirstInputLine,
		(unsigned int*)&horiz6_CMXFirstInputLine
};

unsigned int* horiz_CMXSecondInputLine[HRES_SHAVE_NO]={
	(unsigned int*)&horiz0_CMXSecondInputLine,
	(unsigned int*)&horiz1_CMXSecondInputLine,
	(unsigned int*)&horiz2_CMXSecondInputLine,
	(unsigned int*)&horiz3_CMXSecondInputLine,
	(unsigned int*)&horiz6_CMXSecondInputLine
};

unsigned int* horiz_CMXThirdInputLine[HRES_SHAVE_NO]={
		(unsigned int*)&horiz0_CMXThirdInputLine,
		(unsigned int*)&horiz1_CMXThirdInputLine,
		(unsigned int*)&horiz2_CMXThirdInputLine,
		(unsigned int*)&horiz3_CMXThirdInputLine,
		(unsigned int*)&horiz6_CMXThirdInputLine
};

unsigned int* horiz_current_cif_buffer_adr[HRES_SHAVE_NO]={
		(unsigned int*)&horiz0_current_cif_buffer_adr,
		(unsigned int*)&horiz1_current_cif_buffer_adr,
		(unsigned int*)&horiz2_current_cif_buffer_adr,
		(unsigned int*)&horiz3_current_cif_buffer_adr,
		(unsigned int*)&horiz6_current_cif_buffer_adr
};
unsigned int* horiz_force_shave_graceful_halt[HRES_SHAVE_NO]={
		(unsigned int*)&horiz0_force_shave_graceful_halt,
		(unsigned int*)&horiz1_force_shave_graceful_halt,
		(unsigned int*)&horiz2_force_shave_graceful_halt,
		(unsigned int*)&horiz3_force_shave_graceful_halt,
		(unsigned int*)&horiz6_force_shave_graceful_halt
};

unsigned int horiz_OneLineResize[HRES_SHAVE_NO]={
		(unsigned int)&horiz0_OneLineResize,
		(unsigned int)&horiz1_OneLineResize,
		(unsigned int)&horiz2_OneLineResize,
		(unsigned int)&horiz3_OneLineResize,
		(unsigned int)&horiz6_OneLineResize
};
unsigned int horiz_OneLineResizeSBS[HRES_SHAVE_NO]={
		(unsigned int)&horiz0_OneLineResizeSBS,
		(unsigned int)&horiz1_OneLineResizeSBS,
		(unsigned int)&horiz2_OneLineResizeSBS,
		(unsigned int)&horiz3_OneLineResizeSBS,
		(unsigned int)&horiz6_OneLineResizeSBS
};

// 4: Static Local Data 
// ----------------------------------------------------------------------------
frameBuffer* horizresz_buffers;
unsigned int horizBufferPos = 0;
volatile unsigned int HRES_DATA_SECTION(cif_dma_done)[5] = {0, 0, 0, 0, 0};
volatile unsigned int HRES_DATA_SECTION(graceful_halt)[5] = {0, 0, 0, 0, 0};
unsigned char* frame_start_addresses[5];
unsigned int prevhorizBuffer = 0;
unsigned int ConfiguredHeight;
//Initialize some safe values
unsigned char* last_address[HRES_SHAVE_NO] = {(unsigned char*)&horiz0_CMXFirstInputLine,
        (unsigned char*)&horiz0_CMXFirstInputLine,
        (unsigned char*)&horiz0_CMXFirstInputLine,
        (unsigned char*)&horiz0_CMXFirstInputLine};

const unsigned int hres_shaves[HRES_SHAVE_NO] = {0, 1, 2, 3, 6};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
 
// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Computes the sum of two numbers
void HRES_CODE_SECTION(HRESShaveConf)()
{
    unsigned int *aux_ptr, *cif_done, *graceful;
    unsigned int VarAdr;
    unsigned int i;
    for (i = 0; i < HRES_SHAVE_NO; i++){
        cif_done = (unsigned int*) &cif_dma_done[i];
#ifndef USE_ABSOLUTE_ADR
        aux_ptr = (unsigned int*)(swcSolveShaveRelAddr((unsigned int)(horiz_cif_dma_done_adr[i]), hres_shaves[i]));
#else
        aux_ptr = horiz_cif_dma_done_adr[i];
#endif
        VarAdr = (unsigned int)cif_done;
        //We try an optimization here: SHAVES access CMX through fewer buses in case it acceses 0x1xxxx addresses
        //So we'll check if the variable used as flag is in CMX accessed by Leon or not.
        //Basically it could be in either:
        //CMX accessed through AHB: 0xAxxxxxx
        //CMX accessed through MXI(Shave direct access): 0x1xxxxxx
        //DDR: 0x4xxxxxx
        //LRAM: 0x9xxxxx
        //So in case we have 0xAxxxx we change to 0x1xxxx. We can't do anything for the rest of cases
        VarAdr = (VarAdr & 0xF0000000) == 0xA0000000 ? (VarAdr & 0x0FFFFFFF)| 0x10000000 : VarAdr;
        *aux_ptr = VarAdr;
        //Same story with graceful halt:
        graceful = (unsigned int*) &graceful_halt[i];
#ifndef USE_ABSOLUTE_ADR
        aux_ptr = (unsigned int*)(swcSolveShaveRelAddr((unsigned int)(horiz_force_shave_graceful_halt[i]), hres_shaves[i]));
#else
        aux_ptr = horiz_force_shave_graceful_halt[i];
#endif
        VarAdr = (unsigned int)graceful;
        VarAdr = (VarAdr & 0xF0000000) == 0xA0000000 ? (VarAdr & 0x0FFFFFFF)| 0x10000000 : VarAdr;
        *aux_ptr = VarAdr;
    }

    return;
}

//Returns pointers to the line buffers
unsigned char* HRES_CODE_SECTION(HRESGetInBuffer)(unsigned int no_buf, unsigned int shave_no)
{
    unsigned char* aux_ptr = 0x0;
    switch (no_buf){
    case 0:
#ifndef USE_ABSOLUTE_ADR
        aux_ptr = (unsigned char*)(swcSolveShaveRelAddr(((unsigned int)(horiz_CMXFirstInputLine[shave_no])) + 4 * 2, hres_shaves[shave_no]));
#else
        aux_ptr = (unsigned char*)((unsigned int)((horiz_CMXFirstInputLine[shave_no])) + 4 * 2);
#endif
        break;
    case 1:
#ifndef USE_ABSOLUTE_ADR
        aux_ptr = (unsigned char*)(swcSolveShaveRelAddr(((unsigned int)(horiz_CMXSecondInputLine[shave_no])) + 4 * 2, hres_shaves[shave_no]));
#else
        aux_ptr = (unsigned char*)((unsigned int)((horiz_CMXSecondInputLine[shave_no])) + 4 * 2);
#endif
        break;
    case 2:
#ifndef USE_ABSOLUTE_ADR
        aux_ptr = (unsigned char*)(swcSolveShaveRelAddr(((unsigned int)(horiz_CMXThirdInputLine[shave_no])) + 4 * 2,hres_shaves[shave_no]));
#else
        aux_ptr = (unsigned char*)((unsigned int)((horiz_CMXThirdInputLine[shave_no])) + 4 * 2);
#endif
        break;
    }
    return aux_ptr;
}

void HRES_CODE_SECTION(HRESShaveDone)(unsigned int unused)
{
    prevhorizBuffer = horizBufferPos;
    horizBufferPos++;
    if (horizBufferPos == HORIZRESZ_BUFFERS)
    {
        horizBufferPos = 0;
    }
    HRESASYNCDone(0);
    return;
}

void HRES_CODE_SECTION(HRESSetFinishedBuffer)(unsigned char* buf, unsigned int index, unsigned int line_no)
{
    unsigned int* aux_ptr;
    unsigned int crt_shave;
    //Detect which shave we are talking to
    //crt_shave=index % HRES_SHAVE_NO;
    //Bit handling the above operation (for speed concerns)
    crt_shave = index % HRES_SHAVE_NO;

    //And set the CMX variable for the current shave
    cif_dma_done[crt_shave] = 1;

#ifndef USE_ABSOLUTE_ADR
    aux_ptr = (unsigned int*)(swcSolveShaveRelAddrAHB((unsigned int)(horiz_current_cif_buffer_adr[crt_shave]), hres_shaves[crt_shave]));
#else
    aux_ptr = horiz_current_cif_buffer_adr[crt_shave];
#endif
    *aux_ptr = ((unsigned int)buf) - 4 * 2;

    //Signal as quickly as possible to make sure this works even in the hardest conditions
    if (line_no == ConfiguredHeight / 2)
    {
        HRESShaveDone(4);
    }

    return;
}

void HRES_CODE_SECTION(HRESStartShave)(unsigned int stride, unsigned int in_width, unsigned int out_width, unsigned int in_out_height)
{
    int i;
    unsigned int old_frame_start_addr;
    ConfiguredHeight = in_out_height;
    for (i = 0; i < HRES_SHAVE_NO; i++){
        //Ask SHAVE to stop
        graceful_halt[i] = 1;
        //Wait for the shave to finish first, it might be handling a DMA transfer
        swcWaitShave(hres_shaves[i]);

        //Remove the graceful halt flag
        graceful_halt[i] = 0;

        swcResetShave(hres_shaves[i]);

        //Set the stack pointer, just in case we need extra space in there
#ifndef USE_ABSOLUTE_ADR
        DrvSvutIrfWrite(hres_shaves[i], 19, HRES_STACK);
#else
        swcSetAbsoluteDefaultStack(hres_shaves[i]);
#endif

        frame_start_addresses[i] = horizresz_buffers[horizBufferPos].p1 + horizresz_buffers[horizBufferPos].spec.width * 2 * i;

        DrvSvutIrfWrite(hres_shaves[i], 17, stride + horizresz_buffers[horizBufferPos].spec.width * 4);
        DrvSvutIrfWrite(hres_shaves[i], 16, in_width);
        DrvSvutIrfWrite(hres_shaves[i], 15, out_width);
        DrvSvutIrfWrite(hres_shaves[i], 14, (in_out_height / HRES_SHAVE_NO));
    }

    //The start addresses are custom because of the EOF handler problem

    //Cristian Olar: Let me explain what this is all about:
    //The "EOF handler problem" does not refer to any hardware issue but to the fact that
    //the shaves restarting subroutine (this one, HRESStartShave) is rather long
    //because it MUST wait for any pending DMA transfers
    //This means it will exceed most probably the time available
    //strictly at an end of line (DMA0_DONE) CIF interrupt.
    //The solution is not to run this start subroutine during a DMA0_DONE (End of line in our case
    //because the DMA is programmed to run for exactly one line) but on EOF (End of Frame)
    //This way, we get the whole camera frontporch available to run this instruction instead
    // of just a few cycles
    //This however creates the following dilemma: when restarting the shaves we will have a few lines
    //buffered. These are the last 3 lines of the previous frame.
    //The solution is to take care that these last lines are written in the previous buffer
    //and not in the current one. This explains the following instructions.
    //The shave code will write it's first output at the address stored in i18 but then, after finishing
    //it will load i13 into i18 therefore changing buffers.
    //I've marked the relevant lines in line_resize.asm with this change too.
    //Just search for "HRESStartShaves" in line_resize.asm

    //A question may arise: why not just starting later then, when the first lines start streaming in
    //Answer to that: the ONLY place where I can start is on EOF, because of the reason above
    //Other option: why not changing buffers such a way that we only treat the first coming buffers?
    //Problem is that in order to do that, I'd need to start shaves right before the first line
    //gets streamed in. And there is no interrupt available for that (EOF dos not happen at that time)
    //And even if it would, it would still only have the length of a DMA0_DONE interrupt and be too short

    old_frame_start_addr = (unsigned int)horizresz_buffers[prevhorizBuffer].p1 + out_width * 2;

    DrvSvutIrfWrite(0, 18, (old_frame_start_addr) + out_width * 2 * (in_out_height - 5));
    DrvSvutIrfWrite(1, 18, (old_frame_start_addr) + out_width * 2 * (in_out_height - 4));
    DrvSvutIrfWrite(2, 18, (old_frame_start_addr) + out_width * 2 * (in_out_height - 3));
    DrvSvutIrfWrite(3, 18, (old_frame_start_addr) + out_width * 2 * (in_out_height - 2));
    DrvSvutIrfWrite(6, 18, ((unsigned int)frame_start_addresses[0]));

    DrvSvutIrfWrite(0, 13, ((unsigned int)frame_start_addresses[0]) + out_width * 2);
    DrvSvutIrfWrite(1, 13, ((unsigned int)frame_start_addresses[1]) + out_width * 2);
    DrvSvutIrfWrite(2, 13, ((unsigned int)frame_start_addresses[2]) + out_width * 2);
    DrvSvutIrfWrite(3, 13, ((unsigned int)frame_start_addresses[3]) + out_width * 2);
    DrvSvutIrfWrite(6, 13, ((unsigned int)frame_start_addresses[4]) + out_width * 2);

    //Debugging markers. Only enable for debugging
    //DrvSvutIrfWrite(0, 30, 0xFF00FF00);
    //DrvSvutIrfWrite(1, 30, 0xFF80FF80);
    //DrvSvutIrfWrite(2, 30, 0x80008000);
    //DrvSvutIrfWrite(3, 30, 0x00FF00FF);
    //DrvSvutIrfWrite(6, 30, 0x80FF80FF);//white

    for (i = 0; i < HRES_SHAVE_NO; i++)
    {
        //Start the shave
        DrvSvuStart(hres_shaves[i],(unsigned int)(horiz_OneLineResize[i]));
    }

    //initialize sync values
    cif_dma_done[0] = 0;
    cif_dma_done[1] = 0;
    cif_dma_done[2] = 0;
    cif_dma_done[3] = 0;
    cif_dma_done[4] = 0;

    return;
}

//Gracefully stop shaves
void HRESStopShaves(void)
{
    int i;
    for (i = 0; i < HRES_SHAVE_NO; i++)
    {
        //Ask SHAVE to stop
        graceful_halt[i] = 1;
        //Wait for the shave to finish first, it might be handling a DMA transfer
        swcWaitShave(hres_shaves[i]);
    }
}
