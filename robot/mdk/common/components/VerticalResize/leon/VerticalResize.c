///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     This is the implementation of vertical resize library.
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <DrvGpioDefines.h>
#include <DrvCpr.h>
#include <DrvGpio.h>
#include <DrvIcbDefines.h>
#include <mv_types.h>
#include "VerticalResizeApi.h"
#include <swcShaveLoader.h>
#include <DrvSvu.h>
#include "swcFrameTypes.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
extern u32 VRES_sym(VRES_SH1,VertResize422i);
extern u32 VRES_sym(VRES_SH2,VertResize422i);

extern volatile u32 VRES_sym(VRES_SH1,gracefull_halt);
extern volatile u32 VRES_sym(VRES_SH2,gracefull_halt);

u32 vert_VertResize422i[VRES_SHAVES]={
		(u32)&VRES_sym(VRES_SH1,VertResize422i),
		(u32)&VRES_sym(VRES_SH2,VertResize422i)
};

u32* vert_gracefull_halt[VRES_SHAVES]={
		(u32*)&VRES_sym(VRES_SH1,gracefull_halt),
		(u32*)&VRES_sym(VRES_SH2,gracefull_halt)
};

// 4: Static Local Data 
// ----------------------------------------------------------------------------
frameBuffer* vertresz_buffers;
unsigned int vertBufferPos = 0;
swcShaveUnit_t vres_shaves[VRES_SHAVES] = {VRES_SH1, VRES_SH2};
static u32 AddressOptimized=0;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
 
// 6: Functions Implementation
// ----------------------------------------------------------------------------

void VRES_CODE_SECTION(VRESShaveDone)(unsigned int unused)
{
    VRESASYNCDone(0);
    vertBufferPos++;
    if (vertBufferPos == VERTRESZ_BUFFERS) vertBufferPos = 0;

    return;
}

void VRES_CODE_SECTION(VRESStartShave)(unsigned char* out_addr, unsigned char* in_addr,
        unsigned int in_out_width, unsigned int in_height, unsigned int out_height)
{
    int i;

    if (AddressOptimized==0){
    	for (i = 0; i < VRES_SHAVES; i++){
    		u32 VarAdr;
    		//Optimize access on first run
    		//Since it's the Leon that accesses them
    		VarAdr=(u32)vert_gracefull_halt[i];
    		VarAdr =(VarAdr & 0xF0000000) == 0x10000000 ? (VarAdr & 0x0FFFFFFF) | 0xA0000000 : VarAdr;
    		vert_gracefull_halt[i]=(u32*)VarAdr;
    	}
    	AddressOptimized=1;
    }

    for (i = 0; i < VRES_SHAVES; i++){
    	u32* aux_ptr;
        //Inform shave that it should gracefully halt
        aux_ptr=vert_gracefull_halt[i];
        *aux_ptr=1;

        //Wait for the shave to finish first, it might be handling a DMA transfer
        swcWaitShave(vres_shaves[i]);
        swcResetShave(vres_shaves[i]);

        //Inform shave that it should not gracefully halt
        *aux_ptr=0;

        //Set the stack pointer, just in case we need extra space in there
        swcSetAbsoluteDefaultStack(vres_shaves[i]);

        //Set the input address pointer to register i18 (according to calling convention,
        DrvSvutIrfWrite(vres_shaves[i], 18, ((unsigned int)(out_addr)) + (in_out_width * 2 * out_height) * i);
        DrvSvutIrfWrite(vres_shaves[i], 17, ((unsigned int)(in_addr)) + (in_out_width * in_height) * i);
        DrvSvutIrfWrite(vres_shaves[i], 16, in_out_width);
        DrvSvutIrfWrite(vres_shaves[i], 15, in_height / VRES_SHAVES);
        DrvSvutIrfWrite(vres_shaves[i], 14, out_height / VRES_SHAVES);
    }

    for (i = 0; i < VRES_SHAVES; i++)
    {
        //Start the shave
    	DrvSvuStart(vres_shaves[i], (u32)vert_VertResize422i[i]);
    }

    return;
}
