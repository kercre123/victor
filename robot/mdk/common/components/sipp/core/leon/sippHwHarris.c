#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitHarris(SippFilter *fptr)
{
   HarrisParam *param = (HarrisParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadHarris(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf   *targetI;
   SippHwBuf   *targetO;
   UInt32       paramSet;
   SippHwBuf   *srcI  = fptr->iBuf[0];
   SippHwBuf   *srcO  = fptr->oBuf;
   HarrisParam *param = (HarrisParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_HARRIS_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_HARRIS_ID);
       paramSet = SIPP_HARRIS_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_HARRIS_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_HARRIS_ID);
       paramSet = SIPP_HARRIS_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //Harris specific params
   SET_REG_WORD(paramSet + 0x00,     param->frmDim);                   //SIPP_HARRIS_FRM_DIM_ADR
   SET_REG_WORD(paramSet + 0x04,     param->cfg);                      //SIPP_HARRIS_CFG_ADR
   SET_REG_WORD(paramSet + 0x08,     *((UInt32*)(&(param->kValue))) ); //SIPP_HARRIS_K_ADR
}
#endif
