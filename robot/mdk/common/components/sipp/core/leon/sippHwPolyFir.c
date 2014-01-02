#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#################################################################################
// Used because memcpy can not be used on PC model for set registers
static INLINE void memcpySetU32Registers(UInt32* dst, UInt32* src, UInt32 length)
{
    UInt32 dstAdr  = (UInt32)dst;
    UInt32 *srcAdr = (UInt32*)src;
    UInt32 x  = 0;
    for (x = 0; x < length; x++)
    {
        SET_REG_WORD(dstAdr, *srcAdr);
        dstAdr +=4;
        srcAdr++;
    }
}

//#######################################################################################
void sippInitPolyFir(SippFilter *fptr)
{
   PolyFirParam *param = (PolyFirParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //Pack Input/Output resolutions
   param->frmDimPar = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
   param->frmDimFlt = (fptr->outputH             << SIPP_IMGDIM_SIZE)|fptr->outputW;

 //Pack some internals 
   param->kerSz  = fptr->nLinesUsed[0];
   param->cfgReg = ((param->kerSz & 0x07) <<  0) |
                   ((param->clamp & 0x01) <<  3) |
                   ((param->horzD & 0x3F) <<  4) |
                   ((param->horzN & 0x1F) << 10) |
                   ((param->vertD & 0x3F) << 16) |
                   ((param->vertN & 0x1F) << 22) ;

  #if 0  // validate parameters 
    //printf("error - Polyphase invalid kernel size. Valid values 3, 5, 7");
    sippAssert((((3 == ker_sz) || (5 == ker_sz) || (7 == ker_sz))), E_INVALID_HW_PARAM);

    // if auto mode we will have separate branch
    if (POLY_MODE_ADVANCE == param->mode)
    {
        UInt32 expectedOutWidth  = (par->outputW * param->horzN - 1)/param->horzD + 1;
        UInt32 expectedOutHeight = (par->outputH * param->vertN - 1)/param->vertD + 1;
        sippAssert(((expectedOutWidth == fptr->outputW) && (expectedOutHeight == fptr->outputH)), E_INVALID_HW_PARAM);
    }
    else
    {
        //printf("Auto mode not implemnted yet\n");
    }
 #endif
}

//#################################################################################
void sippLoadPolyFir(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf    *targetI;
   SippHwBuf    *targetO;
   UInt32        paramSet;
   SippHwBuf    *srcI  = fptr->iBuf[0];
   SippHwBuf    *srcO  = fptr->oBuf;
   PolyFirParam *param = (PolyFirParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_UPFIRDN_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_UPFIRDN_ID);
       paramSet = SIPP_UPFIRDN_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_UPFIRDN_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_UPFIRDN_ID);
       paramSet = SIPP_UPFIRDN_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //PolyFIR specific params
   SET_REG_WORD(paramSet + 0x00,     param->frmDimPar);     //SIPP_UPFIRDN_FRM_IN_DIM_ADR
   SET_REG_WORD(paramSet + 0x04,     param->frmDimFlt);     //SIPP_UPFIRDN_FRM_OUT_DIM_ADR
   SET_REG_WORD(paramSet + 0x08,     param->cfgReg);        //SIPP_UPFIRDN_CFG_ADR

 //Set coeficients (the number of coefs is variable !)
   memcpySetU32Registers((UInt32*)(paramSet + 0x010), 
                         (UInt32*)param->vertCoefs, 
                         (param->vertN * 8 * 2)>>2); //SIPP_UPFIRDN_VCOEFF_P0_1_0_ADR

   memcpySetU32Registers((UInt32*)(paramSet + 0x110),
                         (UInt32*)param->horzCoefs, 
                         (param->horzN * 8 * 2)>>2); //SIPP_UPFIRDN_HCOEFF_P0_1_0_ADR, 
}

//#################################################################################
// set Poly-phase parameters
// "ctx" argument is unused, but need to keep consistency with "FnHwLnSetup"
void sippSetupPolyLine(SippFilter *fptr, UInt32 ctx)
{
    SippFilter      *par   = fptr->parents[0];
    UInt32          ker_sz = fptr->nLinesUsed[0];
    PolyFirParam    *param = (PolyFirParam*)fptr->params;
    UInt32          vphase;
    Int32           inputLineNr;
    Int32           inBufIdx;

  //Set vphase and dcount
    vphase = ((fptr->exeNo * param->vertD) % param->vertN);
    SET_REG_WORD(SIPP_UPFIRDN_VPHASE_ADR,     vphase | (vphase <<8) | SIPP_CTXUP_BIT_MASK);
        
  //Set input buffer icbl corectly
    inputLineNr = (par->outputH * fptr->exeNo)/fptr->outputH;
    inBufIdx = inputLineNr - (int)(param->kerSz>>1); 
    if (inBufIdx  < 0)
    {
        inBufIdx = 0;
    }
    SET_REG_WORD(I_CTX(SIPP_UPFIRDN_ID), inputLineNr | ((inBufIdx % par->nLines)<<16) | SIPP_CTXUP_BIT_MASK); 
}

#endif
