#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitLut(SippFilter *fptr)
{
   LutParam    *param    = (LutParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //Patch for channel mode: set number of planes = 1
 //even if processes multiple logical planes; the single plane = multiple channels
   if(param->cfg & (1<<1))
   {
      SippHwBuf  *iBuf = fptr->iBuf[0];
      iBuf->cfg &= ~(0xF << SIPP_NP_OFFSET); //clear old value, thus set nPlanes = 0+1
   }

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadLut(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI  = fptr->iBuf[0];
   SippHwBuf *srcO  = fptr->oBuf;
   LutParam  *param = (LutParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_LUT_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_LUT_ID);
       paramSet = SIPP_LUT_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_LUT_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_LUT_ID);
       paramSet = SIPP_LUT_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);
 
 //The LUT input buffer is not DB, so can't program in advance

 //LUT specific params
   SET_REG_WORD(paramSet + 0x00,  param->frmDim);   //SIPP_LUT_FRM_DIM_ADR  
   SET_REG_WORD(paramSet + 0x04,  param->cfg);      //SIPP_LUT_CFG_ADR      
   SET_REG_WORD(paramSet + 0x08,  param->sizeA);    //SIPP_LUT_SIZES7_0_ADR 
   SET_REG_WORD(paramSet + 0x0C,  param->sizeB);    //SIPP_LUT_SIZES15_8_ADR

 //Not double buffered:
   // SIPP_LUT_APB_ACCESS_ADR
   // SIPP_LUT_APB_RDATA_ADR 
   // SIPP_LUT_RAM_CTRL_ADR  
}

//########################################################################################
void ctxSwitchLut(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
    LutParam *paramNew = (LutParam*)newF->params;
    LutParam *paramOld = (LutParam*)oldF->params;
    UInt32    data;

  //Save old filter context
    oldF->iBuf[0]->ctx = GET_REG_WORD_VAL(pl->ictxAddr[unitID][0]);
    oldF->oBuf->ctx    = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

  //If the LUT used by new filter is different than the old lut, 
  //or if the filter is just loading, then:
  // - program new base
  // - retrigger LUT load
    if ( (paramNew->lut != paramOld->lut) || 
         (newF->exeNo == 0) )
    {
       //Program the new base (cfgs are programmed in advance as they're DB-ed)
         SET_REG_WORD(I_BASE(SIPP_LUT_LOAD_ID), (UInt32)paramNew->lut);
       //Retrigger load: the Lut Load Enable bit is not double buffered,
       //                so need to work with SIPP_LUT_CFG_ADR
         data = GET_REG_WORD_VAL(SIPP_LUT_CFG_ADR);
         SET_REG_WORD(SIPP_LUT_CFG_ADR, data & (~(1<<14)));
         SET_REG_WORD(SIPP_LUT_CFG_ADR, data | (  1<<14) );
    }

  //Load new filter context
    SET_REG_WORD(pl->ictxAddr[unitID][0], (1<<31) | newF->iBuf[0]->ctx);
    SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}

#endif